#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

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
} Actor;

typedef struct {
    Vector2 position;
    Vector2 target;
} GameCamera;

typedef struct {

    Font mapFont;
    int mapFontSize;
    GameCamera camera;

    int map[MAP_HEIGHT][MAP_WIDTH];

    Actor player;
    Actor actors[1024];
    size_t actors_count;

} Game;

void generateMap(Game* game) {

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {
            game->map[y][x] = 0;
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

           for (size_t roomY = roomStartY; roomY <= roomEndY; ++roomY) {
               for (size_t roomX = roomStartX; roomX <= roomEndX; ++roomX) {
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

        // printf("generated room %zu: %dx%d\n", i, roomW, roomH);

    }

    size_t randomRoom = GetRandomValue(0, roomsCount);
    game->player.coord.x = GetRandomValue(rooms[randomRoom + 0], rooms[randomRoom + 2]);
    game->player.coord.y = GetRandomValue(rooms[randomRoom + 1], rooms[randomRoom + 3]);

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {
            if (y == 0 || y == MAP_HEIGHT - 1 || x == 0 || x == MAP_WIDTH - 1) game->map[y][x] = 0;
        }
    }

}

void renderMap(Game* game) {

    int cellSize = game->mapFontSize;

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {

            int glyphX = (x * cellSize - game->camera.position.x);
            int glyphY = (y * cellSize - game->camera.position.y);

            Vector2 renderPos = { glyphX, glyphY };

            if (game->map[y][x] == 1)
                DrawTextEx(game->mapFont, ".", renderPos, MAP_FONT_SIZE, 1, YELLOW);
            else if (game->map[y][x] == 2)
                DrawTextEx(game->mapFont, ".", renderPos, MAP_FONT_SIZE, 1, DARKGRAY);
            else
                DrawTextEx(game->mapFont, "#", renderPos, MAP_FONT_SIZE, 1, WHITE);

        }
    }

}

void updateActors(Game* game) {

}

void renderActor(Game* game, Actor* actor) {

    int cellSize = game->mapFontSize;

    Vector2 targetPosition = {actor->coord.x * cellSize, actor->coord.y * cellSize};
    actor->glyph.position = targetPosition;

    Vector2 bgRenderingPosition = Vector2Subtract(targetPosition, game->camera.position);
    DrawRectangle(bgRenderingPosition.x, bgRenderingPosition.y, cellSize, cellSize, actor->glyph.bgColor);

    Vector2 glyphRenderingPosition = Vector2Subtract(actor->glyph.position, game->camera.position);

    char renderingCharBuffer[2];
    sprintf(renderingCharBuffer, "%c", actor->glyph.ch);
    DrawTextEx(game->mapFont, renderingCharBuffer, glyphRenderingPosition, game->mapFontSize, 1, actor->glyph.fgColor);

}

void initPlayer(Actor* player) {

    player->glyph.ch = '@';
    player->glyph.fgColor = WHITE;
    player->glyph.bgColor = BLACK;
    player->glyph.speed = 10;

}

int main() {

    SetRandomSeed(time(NULL));

    Game game = {0};
    initPlayer(&game.player);
    generateMap(&game);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "hello world");
    SetTargetFPS(60);

    int codepoints[512] = { 0 };
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;   // Basic ASCII characters
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x0400 + i;   // Cyrillic characters
    game.mapFont = LoadFontEx("DejaVuSansMono.ttf", MAP_FONT_SIZE, codepoints, 512);
    game.mapFontSize = MAP_FONT_SIZE;

    printf("font size: %d\n", game.mapFontSize);
    printf("font height: %d\n", game.mapFont.baseSize);

    for (int i = 0; i < 512; ++i) {

        int ch = codepoints[i];
        char tempBuf[2];
        sprintf(tempBuf, "%c", ch);
        Vector2 charSize = MeasureTextEx(game.mapFont, tempBuf, game.mapFontSize, 1);
        printf("%c %dx%d\n", ch, (int) charSize.x, (int) charSize.y);

    }

    while (!WindowShouldClose()) {

        BeginDrawing();

        if (IsKeyPressed(KEY_R)) generateMap(&game);

        if (IsKeyPressed(KEY_W)) game.player.coord.y--;
        if (IsKeyPressed(KEY_S)) game.player.coord.y++;
        if (IsKeyPressed(KEY_A)) game.player.coord.x--;
        if (IsKeyPressed(KEY_D)) game.player.coord.x++;

        ClearBackground(BLACK);

        renderMap(&game);
        renderActor(&game, &game.player);

        Vector2 halfWindowSize = {(float) WINDOW_WIDTH / 2, (float) WINDOW_HEIGHT / 2};
        // game.camera.position = Vector2Subtract(game.player.glyph.position, halfWindowSize);

        game.camera.target = Vector2Subtract(game.player.glyph.position, halfWindowSize);
        game.camera.position = Vector2Lerp(game.camera.position, game.camera.target, 0.05f);

        DrawFPS(10, 10);

        EndDrawing();

    }
    CloseWindow();
    return 0;
}
