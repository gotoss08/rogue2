
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

#define dbg_num(x) printf("DEBUG: [%s] = %d\n", #x, x)
#define dbg_str(x) printf("DEBUG: [%s] = %s\n", #x, x)

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define NORMAL_FPS 60
#define TARGET_FPS 144
#define LERPING_FACTOR(x) x * ((float) NORMAL_FPS / (float) TARGET_FPS)

#define MAP_FONT_SIZE 32

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
    Vector2 velocity;
} Glyph;

typedef struct {
    Coord coord;
    Glyph glyph;
    int visionRadius;
} Actor;

typedef enum {
    Empty,
    Wall,
    Floor,
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

    int windowWidth;
    int windowHeight;

    Font mapFont;
    int mapFontSize;
    int cellSize;

    GameCamera camera;

    Map map;

    Actor player;
    Actor actors[1024];
    size_t actors_count;

    bool useLOS;

} Game;

Vector2 coord2vector(Game* game, Coord coord) {
    return (Vector2) {coord.x * game->cellSize, coord.y * game->cellSize};
}

Vector2 coord2screen(Game* game, Coord coord) {
    return Vector2Subtract(coord2vector(game, coord), game->camera.position);
}

Vector2 vector2screen(Game* game, Vector2 vector) {
    return Vector2Subtract(vector, game->camera.position);
}

// TODO: implement screen2vector and vector2coord functions
// TODO: implement current tile selection using mouse

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

    switch(type) {
    case Wall:
        t.glyph.ch = '#';
        break;
    case Floor:
        t.glyph.ch = '.';
        break;
    default:
        break;
    }

    return t;
}

Tile* getMapTile(Map* map, int x, int y) {
    return &(map->tiles[y][x]);
}

bool isTileBlocksLOS(Tile* tile) {
    switch (tile->type) {
    case Wall:
        return true;
    default:
        return false;
    }
}

bool isTileBlocksMovement(Tile* tile) {
    switch (tile->type) {
    case Wall:
        return true;
    default:
        return false;
    }
}

void clearLOS(Game* game) {
    for (int y = 0; y < game->map.height; ++y)
        for (int x = 0; x < game->map.width; ++x)
            getMapTile(&game->map, x, y)->isInLOS = false;
}

bool plot(Game* game, int x, int y) {

    if (x < 0 || x >= game->map.width || y < 0 || y >= game->map.height)
        return false;

    bool isVisible = !isTileBlocksLOS(getMapTile(&game->map, x, y));

    if (isVisible)
        for (int xx = -1; xx <= 1; xx++)
            for (int yy = -1; yy <= 1; yy++)
                if (x + xx >= 0 && x + xx < game->map.width
                    && y + yy >= 0 && y + yy < game->map.height) {

                    int tileX = x + xx;
                    int tileY = y + yy;

                    Tile* t = getMapTile(&game->map, tileX, tileY);
                    t->isInLOS = true;
                    t->isVisited = true;

                }

    return isVisible;

}

void bresenham(Game* game, int x1, int y1, int x2, int y2) {

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
            bresenham(game, x, y, tx, ty);
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
            *getMapTile(&game->map, x, y) = createTile(Wall);
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

        Tile* currentTile = getMapTile(&game->map, currentX, currentY);

        bool isInRoom = (directionY != 0 && currentX + 1 < game->map.width && getMapTile(&game->map, currentX + 1, currentY)->type != Wall)
                        || (directionY != 0 && currentX - 1 >= 0 && getMapTile(&game->map, currentX - 1, currentY)->type != Wall)
                        || (directionX != 0 && currentY + 1 < game->map.height && getMapTile(&game->map, currentX, currentY + 1)->type != Wall)
                        || (directionX != 0 && currentY - 1 >= 0 && getMapTile(&game->map, currentX, currentY - 1)->type != Wall);

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

                   Tile* t = getMapTile(&game->map, roomX, roomY);
                   *t = createTile(Floor);
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

        if (currentTile->type == Wall) {
            *currentTile = createTile(Floor);
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
                *getMapTile(&game->map, x, y) = createTile(Wall);
            }
        }
    }

    // place player at random room

    int px, py;
    while(1) {
        size_t randomRoom = GetRandomValue(0, roomsCount);
        px = GetRandomValue(rooms[randomRoom + 0], rooms[randomRoom + 2]);
        py = GetRandomValue(rooms[randomRoom + 1], rooms[randomRoom + 3]);
        if (getMapTile(&game->map, px, py)->type != Wall) break;
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

    // TODO: render chars in the middle of background rect

    int cellSize = game->cellSize;
    char chBuffer[2] = {glyph->ch}; // because DrawTextEx requires char*

    Vector2 targetPosition = coord2vector(game, coord);

    if (glyph->animateMovement)
        glyph->position = Vector2Lerp(glyph->position, targetPosition, LERPING_FACTOR(0.1f));
    else
        glyph->position = targetPosition;

    Vector2 chRenderingPosition = vector2screen(game, glyph->position);

    DrawRectangle(chRenderingPosition.x, chRenderingPosition.y, cellSize, cellSize, glyph->bgColor);
    DrawTextEx(game->mapFont, chBuffer, chRenderingPosition, game->mapFontSize, 1, glyph->fgColor);

}

void renderMap(Game* game) {
    for (int y = 0; y < game->map.height; ++y) {
        for (int x = 0; x < game->map.width; ++x) {

            float alpha = 1.0f;

            Tile* t = getMapTile(&game->map, x, y);

            if (game->useLOS && !t->isInLOS && !t->isVisited) continue;
            if (!t->isInLOS && t->isVisited) alpha = 0.05f; // TODO: factor out faded alpha const

            t->glyph.fgColor = Fade(t->glyph.fgColor, alpha);
            renderGlyph(game, (Coord) {x, y}, &t->glyph);

            // int glyphX = (x * game->cellSize - game->camera.position.x);
            // int glyphY = (y * game->cellSize - game->camera.position.y);

            // Vector2 renderPos = { glyphX, glyphY };

            // char chBuffer[2] = {t->glyph.ch};
            // DrawTextEx(game->mapFont, chBuffer, renderPos, game->mapFontSize, 1, Fade(t->glyph.fgColor, alpha));

        }
    }
}

void renderActor(Game* game, Actor* actor) {
    renderGlyph(game, actor->coord, &actor->glyph);
}

bool moveActor(Game* game, Actor* actor, int dx, int dy) {

    int tx = actor->coord.x + dx;
    int ty = actor->coord.y + dy;

    if (tx < 0 || tx >= game->map.width || ty < 0 || ty >= game->map.height) return false;

    Tile* tile = getMapTile(&game->map, tx, ty);

    if (isTileBlocksMovement(tile)) return false;

    actor->coord.x = tx;
    actor->coord.y = ty;

    return true;

}

void initPlayer(Actor* player) {

    player->glyph.ch = '@';
    player->glyph.fgColor = WHITE;
    player->glyph.bgColor = BLACK;
    player->glyph.animateMovement = true;
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

    while (dx > 0) {
        if (!movePlayer(game, 1, 0)) break;
    }

    while (dx < 0) {
        if (!movePlayer(game, -1, 0)) break;
    }

    while (dy > 0) {
        if (!movePlayer(game, 0, 1)) break;
    }

    while (dy < 0) {
        if (!movePlayer(game, 0, -1)) break;
    }

}

int main(int argc, char** argv) {

    (void) argc;
    (void) argv;

    SetRandomSeed(time(NULL));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "rogue v0.1");
    SetTargetFPS(TARGET_FPS);

    Game game = {0};
    game.useLOS = true;
    game.windowWidth = WINDOW_WIDTH;
    game.windowHeight = WINDOW_HEIGHT;

    int codepoints[512] = { 0 };
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;   // Basic ASCII characters
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x0400 + i;   // Cyrillic characters
    game.mapFont = LoadFontEx("DejaVuSansMono.ttf", MAP_FONT_SIZE, codepoints, 512);
    game.mapFontSize = MAP_FONT_SIZE;
    game.cellSize = MAP_FONT_SIZE;

    dbg_num(game.mapFontSize);
    dbg_num(game.mapFont.baseSize);
    dbg_num(game.cellSize);

    initPlayer(&game.player);
    generateMap(&game, MAP_WIDTH, MAP_HEIGHT);

    // for (int i = 0; i < 512; ++i) {
    //     int ch = codepoints[i];
    //     char tempBuf[2];
    //     sprintf(tempBuf, "%c", ch);
    //     Vector2 charSize = MeasureTextEx(game.mapFont, tempBuf, game.mapFontSize, 1);
    //     printf("%c %dx%d\n", ch, (int) charSize.x, (int) charSize.y);
    // }

    while (!WindowShouldClose()) {

        if (IsWindowResized()) {
            game.windowWidth = GetScreenWidth();
            game.windowHeight = GetScreenHeight();
        }

        BeginDrawing();

        // TODO: write custom keys handling and mapping function

        if (IsKeyPressed(KEY_R)) generateMap(&game, MAP_WIDTH, MAP_HEIGHT);
        if (IsKeyPressed(KEY_L)) game.useLOS = !game.useLOS;

        if (IsKeyPressed(KEY_W) || IsKeyPressedRepeat(KEY_W)) movePlayer(&game, 0, -1);
        if (IsKeyPressed(KEY_W) && IsKeyDown(KEY_LEFT_SHIFT)) longMovePlayer(&game, 0, -1);
        if (IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_S)) movePlayer(&game, 0, 1);
        if (IsKeyPressed(KEY_S) && IsKeyDown(KEY_LEFT_SHIFT)) longMovePlayer(&game, 0, 1);
        if (IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_A)) movePlayer(&game, -1, 0);
        if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_SHIFT)) longMovePlayer(&game, -1, 0);
        if (IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_D)) movePlayer(&game, 1, 0);
        if (IsKeyPressed(KEY_D) && IsKeyDown(KEY_LEFT_SHIFT)) longMovePlayer(&game, 1, 0);

        ClearBackground(BLACK);

        renderMap(&game);
        renderActor(&game, &game.player);

        cameraTarget(&game, game.player.glyph.position);
        cameraUpdate(&game);

        DrawFPS(10, 10);

        EndDrawing();

    }
    CloseWindow();
    return 0;
}

