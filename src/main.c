
// TODO: factor out rendering code
// TODO: factor out generation code
// TODO: factor out LOS code
// TODO: factor out Game structure and other common structures to .h file

#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

#define EASING_IMPLEMENTATION
#include "easing.h"

#define dbg_num(x) printf("DEBUG: [%s] = %d\n", #x, x)
#define dbg_str(x) printf("DEBUG: [%s] = %s\n", #x, x)

#define NORMAL_FPS 60
#define TARGET_FPS 60
#define LERPING_FACTOR(x) x * ((float) NORMAL_FPS / (float) TARGET_FPS)

#define MAP_WIDTH 128
#define MAP_HEIGHT 128
#define MAP_GENERATOR_BORDERS_PADDING 3
#define MAP_GENERATOR_ITERATIONS_COUNT MAP_WIDTH * MAP_HEIGHT * 0.5
#define MAP_GENERATOR_STEP_DIRECTION_CHANGE_CHANCE 3
#define MAP_GENERATOR_STEP_ROOM_CHANCE 5
#define MAP_GENERATOR_STEP_ROOM_COOLDOWN 25
#define ROOM_MIN_WIDTH 3
#define ROOM_MAX_WIDTH 10
#define ROOM_MIN_HEIGHT 3
#define ROOM_MAX_HEIGHT 10
#define MAX_ROOMS_COUNT(mapWidth, mapHeight) (int) floor(((double)(mapWidth*mapHeight)) / ((double)(ROOM_MIN_WIDTH*ROOM_MIN_HEIGHT)))

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

typedef struct {
    int x;
    int y;
} Coord;

typedef struct {
    char ch;
    Color fgColor;
    Color bgColor;

    bool animateMovement;
    Vector2 position;
    float animationTime;
} Glyph;

typedef struct {
    Coord coord;
    Glyph glyph;
    int visionRadius;
} Actor;

typedef enum {
    TileTypeEmpty = 0,
    TileTypeWall,
    TileTypeFloor,
} TileType;

typedef struct {
    TileType type;
    Glyph glyph;
    bool isInLOS;
    bool isVisited;
} Tile;

typedef struct {
    int width;
    int height;
    Tile** tiles;
} Map;

typedef struct {
    Vector2 position;
    Vector2 target;
} GameCamera;

typedef struct {
    Font font;
    int size;
    int spacing;
} GameFont;

typedef struct {
    char* text;
    Color color;
} DebugInfoLine;

typedef struct {

    bool visible;
    Vector2 offset;
    Color bgColor;
    DebugInfoLine lines[512];
    size_t linesCount;

} DebugInfo;

typedef struct {
    DebugInfo debugInfo;
} UI;

typedef struct {

    int windowWidth;
    int windowHeight;

    int cellSize;

    GameFont glyphFont;
    GameFont uiFont;
    GameFont debugFont;

    GameCamera camera;

    Map map;

    Actor player;
    Actor actors[1024];
    size_t actors_count;

    bool useLOS;

    float deltaTime;
    Vector2 mouse;
    Coord mouseCoord;

    UI ui;

    bool renderGlyphsCentered;

} Game;

GameFont createGameFont(char* filepath, int size) {

    int codepoints[512] = { 0 };
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;   // Basic ASCII characters
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x0400 + i;   // Cyrillic characters

    GameFont font = {0};

    font.font = LoadFontEx(filepath, size, codepoints, 512);
    font.size = size;
    font.spacing = 1;

    SetTextureFilter(font.font.texture, TEXTURE_FILTER_POINT);

    return font;

}

void renderText(GameFont* font, const char* text, Vector2 position, Color color) {
    DrawTextEx(font->font, text, position, font->size, font->spacing, color);
}

Vector2 renderTextBg(GameFont* font, const char* text, Vector2 position, Color fgColor, Color bgColor) {

    Vector2 textSize = MeasureTextEx(font->font, text, font->size, font->spacing);

    DrawRectangleV(position, textSize, bgColor);
    renderText(font, text, position, fgColor);

    return textSize;

}

Vector2 coord2vector(Game* game, Coord coord) {
    return (Vector2) {coord.x * game->cellSize, coord.y * game->cellSize};
}

Vector2 coord2screen(Game* game, Coord coord) {
    return Vector2Subtract(coord2vector(game, coord), game->camera.position);
}

Vector2 vector2screen(Game* game, Vector2 vector) {
    return Vector2Subtract(vector, game->camera.position);
}

Vector2 screen2vector(Game* game, Vector2 screen) {
    return Vector2Add(screen, game->camera.position);
}

Coord vector2coord(Game* game, Vector2 vector) {
    return (Coord) {vector.x / game->cellSize, vector.y / game->cellSize};
}

Coord screen2coord(Game* game, Vector2 screen) {
    return vector2coord(game, screen2vector(game, screen));
}

void cameraPosition(Game* game, Vector2 position) {
    Vector2 halfWindowSize = {(float) game->windowWidth / 2, (float) game->windowHeight / 2};
    game->camera.position = Vector2Subtract(position, halfWindowSize);
    game->camera.target = game->camera.position;
}

void cameraTarget(Game* game, Vector2 target) {
    Vector2 halfWindowSize = {(float) game->windowWidth / 2, (float) game->windowHeight / 2};
    game->camera.target = Vector2Subtract(target, halfWindowSize);
}

void cameraUpdate(Game* game) {
    game->camera.position = Vector2Lerp(game->camera.position, game->camera.target, LERPING_FACTOR(0.05f));
}

Tile createTile(TileType type) {
    Tile t = {0};
    t.type = type;
    t.glyph.fgColor = WHITE;
    t.glyph.bgColor = BLACK;
    t.glyph.animateMovement = false;

    switch(type) {
    case TileTypeWall:
        t.glyph.ch = '#';
        break;
    case TileTypeFloor:
        t.glyph.ch = '.';
        break;
    default:
        break;
    }

    return t;
}

bool checkMapBounds(Map* map, int x, int y) {
    return x >= 0 && x < map->width && y >= 0 && y < map->height;
}

Tile* mapGetTile(Map* map, int x, int y) {
    return &(map->tiles[y][x]);
}

bool isTileBlocksLOS(Tile* tile) {
    switch (tile->type) {
    case TileTypeWall:
        return true;
    default:
        return false;
    }
}

bool isTileBlocksMovement(Tile* tile) {
    switch (tile->type) {
    case TileTypeWall:
        return true;
    default:
        return false;
    }
}

void clearLOS(Game* game) {
    for (int y = 0; y < game->map.height; ++y)
        for (int x = 0; x < game->map.width; ++x)
            mapGetTile(&game->map, x, y)->isInLOS = false;
}

bool plot(Game* game, int x, int y) {

    if (x < 0 || x >= game->map.width || y < 0 || y >= game->map.height)
        return false;

    bool isVisible = !isTileBlocksLOS(mapGetTile(&game->map, x, y));

    if (isVisible)
        for (int xx = -1; xx <= 1; xx++)
            for (int yy = -1; yy <= 1; yy++)
                if (x + xx >= 0 && x + xx < game->map.width
                    && y + yy >= 0 && y + yy < game->map.height) {

                    int tileX = x + xx;
                    int tileY = y + yy;

                    Tile* t = mapGetTile(&game->map, tileX, tileY);
                    t->isInLOS = true;
                    t->isVisited = true;

                }

    return isVisible;

}

bool alwaysTruePlot(Game* game, int x, int y) {

    Tile* t = mapGetTile(&game->map, x, y);
    t->isInLOS = true;
    t->isVisited = true;

    return true;

}

void bresenham(Game* game, int x1, int y1, int x2, int y2, bool (*plot) (Game* game, int x, int y)) {

    int dx = x2 - x1;
    int ix = dx > 0 ? 1 : -1;
    dx = 2 * abs(dx);

    int dy = y2 - y1;
    int iy = dy > 0 ? 1 : -1;
    dy = 2 * abs(dy);

    if (!plot(game, x1, y1)) return;

    if (dx >= dy) {
        int error = dy - dx / 2;

        while (x1 != x2) {
            if (error > 0 || (error == 0 && ix > 0)) {
                error = error - dx;
                y1 = y1 + iy;
            }

            error = error + dy;
            x1 = x1 + ix;

            if (!plot(game, x1, y1)) return;
        }
    } else {
        int error = dx - dy / 2;

        while (y1 != y2) {
            if (error > 0 || (error == 0 && iy > 0)) {
                error = error - dy;
                x1 = x1 + ix;
            }

            error = error + dx;
            y1 = y1 + iy;

            if (!plot(game, x1, y1)) return;
        }
    }

}

void calcLOS(Game* game, const int x, const int y, const int boxRadius) {

    clearLOS(game);

    int hr = (int) floor((float) boxRadius / 2);

    int xx = x - hr;
    if (xx < 0) xx = 0;

    int yy = y - hr;
    if (yy < 0) yy = 0;

    for (int ty = yy; ty < y + hr; ty++) {
        for (int tx = xx; tx < x + hr; tx++) {
            bresenham(game, x, y, tx, ty, &plot);
        }
    }

}

void generateMap(Game* game, int width, int height) {

    if (game->map.width != 0) {
        for (int y = 0; y < game->map.height; ++y) {
            free(game->map.tiles[y]);
        }
        free(game->map.tiles);
    }

    game->map.width = width;
    game->map.height = height;

    game->map.tiles = malloc(height * sizeof(Tile*));
    for (int y = 0; y < height; ++y) {
        game->map.tiles[y] = malloc(width * sizeof(Tile));
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            *mapGetTile(&game->map, x, y) = createTile(TileTypeWall);
        }
    }

    int currentX = GetRandomValue(MAP_GENERATOR_BORDERS_PADDING, game->map.width - MAP_GENERATOR_BORDERS_PADDING);
    int currentY = GetRandomValue(MAP_GENERATOR_BORDERS_PADDING, game->map.height - MAP_GENERATOR_BORDERS_PADDING);

    bool updateDirection = true;

    int directionX = 0;
    int directionY = 0;

    int steps = 0;
    int roomCooldown = 0;

    int rooms[MAX_ROOMS_COUNT(game->map.width, game->map.height)];
    size_t roomsCount = 0;

    printf("map size: %dx%d\n", game->map.width, game->map.height);
    printf("min room size: %dx%d\n", ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT);
    printf("max possible rooms count: %d\n", MAX_ROOMS_COUNT(game->map.width, game->map.height));

    while(1) {

        if (steps >= MAP_GENERATOR_ITERATIONS_COUNT) break;

        if (updateDirection) {

            updateDirection = false;
            int randomDirection = GetRandomValue(0, 1) == 0 ? -1 : 1;

            if (directionX != 0) {
                directionX = 0;
                directionY = randomDirection;
            } else if (directionY != 0) {
                directionX = randomDirection;
                directionY = 0;
            } else {
                if (GetRandomValue(0, 1) == 0) directionX = randomDirection;
                else directionY = randomDirection;
            }
        }

        int nextX = currentX + directionX;
        int nextY = currentY + directionY;

        if (nextX >= game->map.width - MAP_GENERATOR_BORDERS_PADDING || nextX < MAP_GENERATOR_BORDERS_PADDING
            || nextY >= game->map.height - MAP_GENERATOR_BORDERS_PADDING || nextY < MAP_GENERATOR_BORDERS_PADDING) {
            updateDirection = true;
            continue;
        }

        if (GetRandomValue(0, 100) <= MAP_GENERATOR_STEP_DIRECTION_CHANGE_CHANCE) {
            updateDirection = true;
            // continue;
        }

        currentX = nextX;
        currentY = nextY;

        Tile* currentTile = mapGetTile(&game->map, currentX, currentY);

        bool isInRoom = (directionY != 0 && currentX + 1 < game->map.width && mapGetTile(&game->map, currentX + 1, currentY)->type != TileTypeWall)
                        || (directionY != 0 && currentX - 1 >= 0 && mapGetTile(&game->map, currentX - 1, currentY)->type != TileTypeWall)
                        || (directionX != 0 && currentY + 1 < game->map.height && mapGetTile(&game->map, currentX, currentY + 1)->type != TileTypeWall)
                        || (directionX != 0 && currentY - 1 >= 0 && mapGetTile(&game->map, currentX, currentY - 1)->type != TileTypeWall);

        if (!isInRoom && roomCooldown == 0 && GetRandomValue(0, 100) <= MAP_GENERATOR_STEP_ROOM_CHANCE) {

           int roomWidth = GetRandomValue(ROOM_MIN_WIDTH, ROOM_MAX_WIDTH);
           int roomHeight = GetRandomValue(ROOM_MIN_HEIGHT, ROOM_MAX_HEIGHT);
           int roomHalfWidth = floor((float) roomWidth / 2.0f);
           int roomHalfHeight = floor((float) roomHeight / 2.0f);

           int roomStartX = currentX - roomHalfWidth;
           if (roomStartX < 0) roomStartX = 0;

           int roomStartY = currentY - roomHalfHeight;
           if (roomStartY < 0) roomStartY = 0;

           int roomEndX = roomStartX + roomWidth;
           if (roomEndX >= game->map.width) roomEndX = game->map.width - 1;

           int roomEndY = roomStartY + roomHeight;
           if (roomEndY >= game->map.height) roomEndY = game->map.height - 1;

           for (int roomY = roomStartY; roomY <= roomEndY; ++roomY) {
               for (int roomX = roomStartX; roomX <= roomEndX; ++roomX) {

                   Tile* t = mapGetTile(&game->map, roomX, roomY);
                   *t = createTile(TileTypeFloor);
                   t->glyph.fgColor = DARKGRAY;

               }
           }

           rooms[roomsCount * 4 + 0] = roomStartX;
           rooms[roomsCount * 4 + 1] = roomStartY;
           rooms[roomsCount * 4 + 2] = roomEndX;
           rooms[roomsCount * 4 + 3] = roomEndY;
           ++roomsCount;

           roomCooldown = MAP_GENERATOR_STEP_ROOM_COOLDOWN;

        }

        if (currentTile->type == TileTypeWall) {
            *currentTile = createTile(TileTypeFloor);
            currentTile->glyph.fgColor = YELLOW;
        }

        steps++;
        if (roomCooldown > 0) roomCooldown--;

    }

    // for (size_t i = 0; i < roomsCount; ++i) {
    //     int roomX = rooms[i * 4 + 0];
    //     int roomY = rooms[i * 4 + 1];
    //     int roomW = rooms[i * 4 + 2] - roomX;
    //     int roomH = rooms[i * 4 + 3] - roomY;
    //     printf("generated room %zu: %dx%d\n", i, roomW, roomH);
    // }

    // place walls at map edges

    for (int y = 0; y < game->map.height; ++y) {
        for (int x = 0; x < game->map.width; ++x) {
            if (y == 0 || y == game->map.height - 1 || x == 0 || x == game->map.width - 1) {
                *mapGetTile(&game->map, x, y) = createTile(TileTypeWall);
            }
        }
    }

    // place player at random room

    int px, py;
    while(1) {
        size_t randomRoom = GetRandomValue(0, roomsCount);
        px = GetRandomValue(rooms[randomRoom + 0], rooms[randomRoom + 2]);
        py = GetRandomValue(rooms[randomRoom + 1], rooms[randomRoom + 3]);
        if (mapGetTile(&game->map, px, py)->type != TileTypeWall) break;
    }

    game->player.coord.x = px;
    game->player.coord.y = py;
    game->player.glyph.position = coord2vector(game, game->player.coord);

    cameraPosition(game, game->player.glyph.position);
    cameraTarget(game, game->player.glyph.position);

    calcLOS(game, game->player.coord.x, game->player.coord.y, game->player.visionRadius);

}

void updateActors(Game* game) {
    (void) game;
}

void renderGlyph(Game* game, Coord coord, Glyph* glyph) {

    int cellSize = game->cellSize;
    char chBuffer[2] = {glyph->ch}; // because DrawTextEx requires char*

    Vector2 chTargetPosition = coord2vector(game, coord);

    if (game->renderGlyphsCentered) {
        GlyphInfo glyphInfo = GetGlyphInfo(game->glyphFont.font, (int) glyph->ch);
        Vector2 textSize = MeasureTextEx(game->glyphFont.font, chBuffer, game->glyphFont.size, game->glyphFont.spacing);
        chTargetPosition = Vector2Add(chTargetPosition, (Vector2) {(float) cellSize / 2 - (float) glyphInfo.offsetX / 2, (float) cellSize / 2 - (float) glyphInfo.offsetY / 2});
        chTargetPosition = Vector2Subtract(chTargetPosition, Vector2Scale(textSize, 0.5));
    }

    if (glyph->animateMovement && !Vector2Equals(glyph->position, chTargetPosition)) {
        glyph->position = Vector2Lerp(glyph->position, chTargetPosition, easeOutBack(glyph->animationTime));
        glyph->animationTime += game->deltaTime;
    } else glyph->position = chTargetPosition;

    Vector2 bgTargetPosition = glyph->position;

    Vector2 chRenderingPosition = vector2screen(game, glyph->position);
    Vector2 bgRenderingPosition = vector2screen(game, bgTargetPosition);

    DrawRectangle(bgRenderingPosition.x, bgRenderingPosition.y, cellSize, cellSize, glyph->bgColor);
    DrawTextEx(game->glyphFont.font, chBuffer, chRenderingPosition, game->glyphFont.size, game->glyphFont.spacing, glyph->fgColor);

}

// TODO: add Map* as argument to renderMap()

void renderMap(Game* game) {
    for (int y = 0; y < game->map.height; ++y) {
        for (int x = 0; x < game->map.width; ++x) {

            float alpha = 1.0f;

            Tile* t = mapGetTile(&game->map, x, y);

            if (game->useLOS && !t->isInLOS && !t->isVisited) continue;
            if (!t->isInLOS && t->isVisited) alpha = 0.05f; // TODO: factor out faded alpha const

            t->glyph.fgColor = Fade(t->glyph.fgColor, alpha);
            renderGlyph(game, (Coord) {x, y}, &t->glyph);

        }
    }
}

void renderActor(Game* game, Actor* actor) {
    renderGlyph(game, actor->coord, &actor->glyph);
}

void highlightTile(Game* game, Coord coord, Color color) {
    Vector2 hoverPosition = coord2screen(game, coord);
    // Rectangle rect = {hoverPosition.x, hoverPosition.y, game->cellSize, game->cellSize};
    // DrawRectangleRoundedLines(rect, 1, 10, 1, color);
    DrawRectangleLines(hoverPosition.x, hoverPosition.y, game->cellSize, game->cellSize, color);
}

void renderCurrentTileInfo(Game* game) {

    if (checkMapBounds(&game->map, game->mouseCoord.x, game->mouseCoord.y)) {

        Tile* t = mapGetTile(&game->map, game->mouseCoord.x, game->mouseCoord.y);

        if (!t->isInLOS && !t->isVisited) return;

        highlightTile(game, game->mouseCoord, YELLOW);

        char* tileTypeText;
        switch(t->type) {
        case TileTypeEmpty:
            tileTypeText = "Empty";
            break;
        case TileTypeWall:
            tileTypeText = "Wall";
            break;
        case TileTypeFloor:
            tileTypeText = "Floor";
            break;
        default:
            tileTypeText = "<Unknown>";
            break;
        }

        const char* currentTileText = TextFormat("Tile [%c] - %s", t->glyph.ch, tileTypeText);
        Vector2 size = MeasureTextEx(game->uiFont.font, currentTileText, game->uiFont.size, game->uiFont.spacing);
        renderTextBg(&game->uiFont, currentTileText, (Vector2) {10, game->windowHeight - 10 - size.y}, YELLOW, Fade(BLACK, 0.85f));

    }

}

bool plotPathToMousePosition(Game* game, int x, int y) {
    highlightTile(game, (Coord) {x, y}, GREEN);
    return true;
}

void renderPathToMousePosition(Game* game) {

    bresenham(game, game->player.coord.x, game->player.coord.y, game->mouseCoord.x, game->mouseCoord.y, &plotPathToMousePosition);

}

void renderDebugInfo(Game* game, DebugInfo* debugInfo) {

    DebugInfo di = *debugInfo;

    if (!di.visible) return;

    float lineY = 0;

    for (size_t i = 0; i < di.linesCount; ++i) {

        DebugInfoLine line = di.lines[i];

        Vector2 position = {di.offset.x, di.offset.y + lineY};
        Vector2 textSize = renderTextBg(&game->debugFont, line.text, position, line.color, di.bgColor);

        lineY += textSize.y;

    }

}

void addDebugInfoLine(Game* game, const char* text, Color color) {

    DebugInfoLine line = {0};
    line.text = (char*) text;
    line.color = color;

    game->ui.debugInfo.lines[game->ui.debugInfo.linesCount] = line;
    game->ui.debugInfo.linesCount++;

}

void clearDebugInfo(Game* game) {
    game->ui.debugInfo.linesCount = 0;
}

void renderUI(Game* game) {

    // renderPathToMousePosition(game);
    renderCurrentTileInfo(game);

    renderDebugInfo(game, &game->ui.debugInfo);

}

bool moveActor(Game* game, Actor* actor, int dx, int dy) {

    int tx = actor->coord.x + dx;
    int ty = actor->coord.y + dy;

    if (tx < 0 || tx >= game->map.width || ty < 0 || ty >= game->map.height) return false;

    Tile* tile = mapGetTile(&game->map, tx, ty);

    if (isTileBlocksMovement(tile)) return false;

    actor->coord.x = tx;
    actor->coord.y = ty;
    actor->glyph.animationTime = 0;

    return true;

}

void initPlayer(Actor* player) {

    player->glyph.ch = '@';
    player->glyph.fgColor = WHITE;
    player->glyph.bgColor = BLACK;
    player->glyph.animateMovement = true;
    player->glyph.animationTime = 0;
    // player->glyph.t = LERPING_FACTOR(0.5f);
    // player->glyph.defaultT = player->glyph.t;
    player->visionRadius = 20;

}

bool movePlayer(Game* game, int dx, int dy) {

    if (moveActor(game, &game->player, dx, dy)) {
        calcLOS(game, game->player.coord.x, game->player.coord.y, game->player.visionRadius);
        return true;
    }

    return false;

}

void longMovePlayer(Game* game, int dx, int dy) {
    while (movePlayer(game, dx, dy));
}

int main(int argc, char** argv) {

    (void) argc;
    (void) argv;

    SetRandomSeed(time(NULL));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "rogue v0.1");
    SetTargetFPS(TARGET_FPS);

    Game game = {0};
    game.windowWidth = WINDOW_WIDTH;
    game.windowHeight = WINDOW_HEIGHT;
    game.cellSize = 32;

    game.glyphFont = createGameFont("assets/fonts/DejaVuSansMono.ttf", 32);
    // game.glyphFont = createGameFont("assets/fonts/FSEX302.ttf", 32);
    game.uiFont = createGameFont("assets/fonts/DejaVuSans.ttf", 26);
    game.debugFont = createGameFont("assets/fonts/Iosevka-Regular.ttf", 24);

    game.useLOS = true;
    game.renderGlyphsCentered = true;

    game.ui.debugInfo.offset = (Vector2) { 5, 5 };
    game.ui.debugInfo.bgColor = Fade(BLACK, 0.65f);

    dbg_num(game.glyphFont.size);
    dbg_num(game.glyphFont.font.baseSize);
    dbg_num(game.cellSize);

    initPlayer(&game.player);
    generateMap(&game, MAP_WIDTH, MAP_HEIGHT);

    // for (int i = 0; i < 512; ++i) {
    //     int ch = codepoints[i];
    //     char tempBuf[2];
    //     sprintf(tempBuf, "%c", ch);
    //     Vector2 charSize = MeasureTextEx(game.glyphFont, tempBuf, game.glyphFontSize, 1);
    //     printf("%c %dx%d\n", ch, (int) charSize.x, (int) charSize.y);
    // }

    while (!WindowShouldClose()) {

        clearDebugInfo(&game);

        addDebugInfoLine(&game, TextFormat("FPS: %d", GetFPS()), WHITE);

        game.deltaTime = GetFrameTime();
        addDebugInfoLine(&game, TextFormat("Frame time: %f", game.deltaTime), WHITE);

        if (IsWindowResized()) {
            game.windowWidth = GetScreenWidth();
            game.windowHeight = GetScreenHeight();
        }

        // TODO: write custom keys handling and mapping function

        if (IsKeyPressed(KEY_R)) generateMap(&game, MAP_WIDTH, MAP_HEIGHT);
        if (IsKeyPressed(KEY_L)) game.useLOS = !game.useLOS;
        if (IsKeyPressed(KEY_F1)) game.renderGlyphsCentered = !game.renderGlyphsCentered;
        if (IsKeyPressed(KEY_F3)) game.ui.debugInfo.visible = !game.ui.debugInfo.visible;

        // up movement

        if (IsKeyPressed(KEY_W) || IsKeyPressedRepeat(KEY_W))
            movePlayer(&game, 0, -1);
        if (!IsKeyPressed(KEY_W) && IsKeyPressedRepeat(KEY_W))
            game.player.glyph.position = coord2vector(&game, game.player.coord);
        if (IsKeyPressed(KEY_W) && IsKeyDown(KEY_LEFT_SHIFT))
            longMovePlayer(&game, 0, -1);

        // down movement

        if (IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_S))
            movePlayer(&game, 0, 1);
        if (!IsKeyPressed(KEY_S) && IsKeyPressedRepeat(KEY_S))
            game.player.glyph.position = coord2vector(&game, game.player.coord);
        if (IsKeyPressed(KEY_S) && IsKeyDown(KEY_LEFT_SHIFT))
            longMovePlayer(&game, 0, 1);

        // left movement

        if (IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_A))
            movePlayer(&game, -1, 0);
        if (!IsKeyPressed(KEY_A) && IsKeyPressedRepeat(KEY_A))
            game.player.glyph.position = coord2vector(&game, game.player.coord);
        if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_SHIFT))
            longMovePlayer(&game, -1, 0);

        // right movement

        if (IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_D))
            movePlayer(&game, 1, 0);
        if (!IsKeyPressed(KEY_D) && IsKeyPressedRepeat(KEY_D))
            game.player.glyph.position = coord2vector(&game, game.player.coord);
        if (IsKeyPressed(KEY_D) && IsKeyDown(KEY_LEFT_SHIFT))
            longMovePlayer(&game, 1, 0);

        Vector2 mouse = GetMousePosition();
        game.mouse = mouse;
        game.mouseCoord = screen2coord(&game, mouse);

        BeginDrawing();

        ClearBackground(BLACK);

        renderMap(&game);
        renderActor(&game, &game.player);
        renderUI(&game);

        cameraTarget(&game, game.player.glyph.position);
        cameraUpdate(&game);

        // DrawFPS(10, 10);

        EndDrawing();

    }
    CloseWindow();
    return 0;
}

