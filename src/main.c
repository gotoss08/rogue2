
// TODO: factor out rendering code
// TODO: factor out generation code
// TODO: factor out LOS code
// TODO: factor out Game structure and other common structures to .h file

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
#define MAX_ROOMS_COUNT (int)floor(((double)(MAP_WIDTH*MAP_HEIGHT)) / ((double)(ROOM_MIN_WIDTH*ROOM_MIN_HEIGHT)))

typedef struct {
    int x;
    int y;
} Coord;

typedef struct {
    char ch;
    Color fgColor;
    Color bgColor;
    Vector2 position;
    Vector2 velocity;
    float speed;
} Glyph;

typedef struct {
    Coord coord;
    Glyph glyph;
    int visionRadius;
} Actor;

typedef struct {
    Vector2 position;
    Vector2 target;
} GameCamera;

typedef struct {

    int windowWidth;
    int windowHeight;

    Font mapFont;
    int mapFontSize;
    GameCamera camera;

    int cellSize;
    int map[MAP_HEIGHT][MAP_WIDTH];
    char los[MAP_HEIGHT][MAP_WIDTH];
    char visited[MAP_HEIGHT][MAP_WIDTH];

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

void clearLOS(Game* game) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            game->los[y][x] = 0;
        }
    }
}

bool plot(Game* game, int x, int y) {
    bool isVisible = game->map[y][x] > 0;
    if (isVisible)
        for (int xx = -1; xx < 2; ++xx)
            for (int yy = -1; yy < 2; ++yy)
                if (x + xx >= 0 && x + xx < MAP_WIDTH
                    && y + yy >= 0 && y + yy < MAP_HEIGHT) {

                    int tileX = x + xx;
                    int tileY = y + yy;

                    game->los[tileY][tileX] = 1;
                    game->visited[tileY][tileX] = 1;

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

void generateMap(Game* game) {

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {
            game->map[y][x] = 0;
            game->los[y][x] = 0;
            game->visited[y][x] = 0;
        }
    }

    int currentX = GetRandomValue(MAP_GENERATOR_BORDERS_PADDING, MAP_WIDTH - MAP_GENERATOR_BORDERS_PADDING);
    int currentY = GetRandomValue(MAP_GENERATOR_BORDERS_PADDING, MAP_HEIGHT - MAP_GENERATOR_BORDERS_PADDING);

    bool updateDirection = true;

    int directionX = 0;
    int directionY = 0;

    int steps = 0;
    int roomCooldown = 0;

    int rooms[MAX_ROOMS_COUNT];
    size_t roomsCount = 0;

    printf("map size: %dx%d\n", MAP_WIDTH, MAP_HEIGHT);
    printf("min room size: %dx%d\n", ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT);
    printf("max possible rooms count: %d\n", MAX_ROOMS_COUNT);

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

        if (nextX >= MAP_WIDTH - MAP_GENERATOR_BORDERS_PADDING || nextX < MAP_GENERATOR_BORDERS_PADDING
            || nextY >= MAP_HEIGHT - MAP_GENERATOR_BORDERS_PADDING || nextY < MAP_GENERATOR_BORDERS_PADDING) {
            updateDirection = true;
            continue;
        }

        if (GetRandomValue(0, 100) <= MAP_GENERATOR_STEP_DIRECTION_CHANGE_CHANCE) {
            updateDirection = true;
            // continue;
        }

        currentX = nextX;
        currentY = nextY;

        bool isInRoom = (directionY != 0 && currentX + 1 < MAP_WIDTH && game->map[currentY][currentX + 1] != 0)
            || (directionY != 0 && currentX - 1 >= 0 && game->map[currentY][currentX - 1] != 0)
            || (directionX != 0 && currentY + 1 < MAP_HEIGHT && game->map[currentY + 1][currentX] != 0)
            || (directionX != 0 && currentY - 1 >= 0 && game->map[currentY - 1][currentX] != 0);

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
           if (roomEndX >= MAP_WIDTH) roomEndX = MAP_WIDTH - 1;

           int roomEndY = roomStartY + roomHeight;
           if (roomEndY >= MAP_HEIGHT) roomEndY = MAP_HEIGHT - 1;

           for (int roomY = roomStartY; roomY <= roomEndY; ++roomY) {
               for (int roomX = roomStartX; roomX <= roomEndX; ++roomX) {
                   game->map[roomY][roomX] = 2;
               }
           }

           rooms[roomsCount * 4 + 0] = roomStartX;
           rooms[roomsCount * 4 + 1] = roomStartY;
           rooms[roomsCount * 4 + 2] = roomEndX;
           rooms[roomsCount * 4 + 3] = roomEndY;
           ++roomsCount;

           roomCooldown = MAP_GENERATOR_STEP_ROOM_COOLDOWN;

        }

        if (game->map[currentY][currentX] == 0)
            game->map[currentY][currentX] = 1;

        steps++;
        if (roomCooldown > 0) roomCooldown--;

    }


    for (size_t i = 0; i < roomsCount; ++i) {

        int roomX = rooms[i * 4 + 0];
        int roomY = rooms[i * 4 + 1];
        int roomW = rooms[i * 4 + 2] - roomX;
        int roomH = rooms[i * 4 + 3] - roomY;

        printf("generated room %zu: %dx%d\n", i, roomW, roomH);

    }

    // place player at random room

    int px, py;
    while(1) {
        size_t randomRoom = GetRandomValue(0, roomsCount);
        px = GetRandomValue(rooms[randomRoom + 0], rooms[randomRoom + 2]);
        py = GetRandomValue(rooms[randomRoom + 1], rooms[randomRoom + 3]);
        if (game->map[py][px] > 0) break;
    }
    game->player.coord.x = px;
    game->player.coord.y = py;

    game->player.glyph.position = coord2vector(game, game->player.coord);

    cameraPosition(game, game->player.glyph.position);
    cameraTarget(game, game->player.glyph.position);

    calcLOS(game, game->player.coord.x, game->player.coord.y, game->player.visionRadius);

    // place walls at map edges

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {
            if (y == 0 || y == MAP_HEIGHT - 1 || x == 0 || x == MAP_WIDTH - 1) game->map[y][x] = 0;
        }
    }

}

void renderMap(Game* game) {

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {

            float alpha = 1.0f;

            if (game->useLOS && !game->los[y][x] && !game->visited[y][x]) continue;
            if (!game->los[y][x] && game->visited[y][x]) alpha = 0.05f;

            int glyphX = (x * game->cellSize - game->camera.position.x);
            int glyphY = (y * game->cellSize - game->camera.position.y);

            Vector2 renderPos = { glyphX, glyphY };

            if (game->map[y][x] == 1)
                DrawTextEx(game->mapFont, ".", renderPos, MAP_FONT_SIZE, 1, Fade(YELLOW, alpha));
            else if (game->map[y][x] == 2)
                DrawTextEx(game->mapFont, ".", renderPos, MAP_FONT_SIZE, 1, Fade(DARKGRAY, alpha));
            else
                DrawTextEx(game->mapFont, "#", renderPos, MAP_FONT_SIZE, 1, Fade(WHITE, alpha));

        }
    }

}

void updateActors(Game* game) {
    (void) game;
}

void renderActor(Game* game, Actor* actor) {

    int cellSize = game->cellSize;
    char chBuffer[2] = {actor->glyph.ch}; // because DrawTextEx requires char*

    Vector2 targetPosition = coord2vector(game, actor->coord);
    actor->glyph.position = Vector2Lerp(actor->glyph.position, targetPosition, LERPING_FACTOR(0.1f));

    Vector2 chRenderingPosition = vector2screen(game, actor->glyph.position);

    DrawRectangle(chRenderingPosition.x, chRenderingPosition.y, cellSize, cellSize, actor->glyph.bgColor);
    DrawTextEx(game->mapFont, chBuffer, chRenderingPosition, game->mapFontSize, 1, actor->glyph.fgColor);

}

void initPlayer(Actor* player) {

    player->glyph.ch = '@';
    player->glyph.fgColor = WHITE;
    player->glyph.bgColor = BLACK;
    player->glyph.speed = 10;
    player->visionRadius = 20;

}

void movePlayer(Game* game, int dx, int dy) {

    game->player.coord.x += dx;
    game->player.coord.y += dy;

    calcLOS(game, game->player.coord.x, game->player.coord.y, game->player.visionRadius);

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
    generateMap(&game);

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

        if (IsKeyPressed(KEY_R)) generateMap(&game);
        if (IsKeyPressed(KEY_L)) game.useLOS = !game.useLOS;

        if (IsKeyPressed(KEY_W) || IsKeyPressedRepeat(KEY_W)) movePlayer(&game, 0, -1);
        if (IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_S)) movePlayer(&game, 0, 1);
        if (IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_A)) movePlayer(&game, -1, 0);
        if (IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_D)) movePlayer(&game, 1, 0);

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

