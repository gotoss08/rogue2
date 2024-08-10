#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include <raylib.h>

#define TILE_WIDTH 16
#define TILE_HEIGHT 16

#define MAP_WIDTH 64
#define MAP_HEIGHT 64
#define MAP_GENERATOR_BORDERS_PADDING 3
#define MAP_GENERATOR_ITERATIONS_COUNT 500
#define MAP_GENERATOR_STEP_DIRECTION_CHANGE_CHANCE 15
#define MAP_GENERATOR_STEP_ROOM_CHANCE 5

typedef struct {
    Vector2 position;
    Vector2 velocity;
} Actor;

typedef struct {

    int map[MAP_HEIGHT][MAP_WIDTH];

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

    while(1) {

        if (steps >= MAP_GENERATOR_ITERATIONS_COUNT) break;

        if (updateDirection) {
            directionX = 0;
            directionY = 0;
            if (GetRandomValue(0, 1) == 0) {
                directionX = GetRandomValue(0, 1) == 0 ? -1 : 1;
            } else directionY = GetRandomValue(0, 1) == 0 ? -1 : 1;
            updateDirection = false;
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
            continue;
        }

        if (GetRandomValue(0, 100) <= MAP_GENERATOR_STEP_ROOM_CHANCE) {
            // todo
        }

        printf("step\n");

        currentX = nextX;
        currentY = nextY;

        game->map[currentY][currentX] = 1;

        steps++;

    }

}

void renderMap(Game* game) {

    int paddingX = 1;
    int paddingY = 1;

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {

            int renderX = x * TILE_WIDTH + paddingX;
            int renderY = y * TILE_HEIGHT + paddingY;
            int renderW = TILE_WIDTH - paddingX * 2;
            int renderH = TILE_HEIGHT - paddingY * 2;

            if (game->map[y][x] == 1)
                DrawRectangle(renderX, renderY, renderW, renderH, GREEN);
            else 
                DrawRectangle(renderX, renderY, renderW, renderH, YELLOW);
        }
    }

}

void updateActors(Game* game) {

}

int main() {

    SetRandomSeed(time(NULL));

    Game game = {0};

    generateMap(&game);

    InitWindow(1920, 1080, "hello world");
    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsKeyPressed(KEY_R)) generateMap(&game);

        ClearBackground(BLACK);
        renderMap(&game);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
