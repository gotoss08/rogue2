#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define TILE_WIDTH 32
#define TILE_HEIGHT 32

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

#define FRICTION 0.35

float global_scale = 0.2;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float speed;
} Actor;

typedef struct {
    Vector2 position;
    Vector2 target;
} GameCamera;

typedef struct {

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
           int roomStartY = currentY - roomHalfHeight;
           if (roomStartX < 0) roomStartX = 0;
           if (roomStartY < 0) roomStartY = 0;

           for (size_t roomY = roomStartY; (roomY < roomStartY + roomHeight) && roomY < MAP_HEIGHT; ++roomY) {
               for (size_t roomX = roomStartX; (roomX < roomStartX + roomWidth) && roomX < MAP_WIDTH; ++roomX) {
                   game->map[roomY][roomX] = 2;
               }
           }

           roomCooldown = MAP_GENERATOR_STEP_ROOM_COOLDOWN;
        }

        if (game->map[currentY][currentX] == 0)
            game->map[currentY][currentX] = 1;

        steps++;
        if (roomCooldown > 0) roomCooldown--;

    }

}

void renderMap(Game* game) {

    int paddingX = 1;
    int paddingY = 1;

    for (size_t y = 0; y < MAP_HEIGHT; ++y) {
        for (size_t x = 0; x < MAP_WIDTH; ++x) {

            int renderX = (x * TILE_WIDTH + paddingX - game->camera.position.x - (float) WINDOW_WIDTH / 2) * global_scale;
            int renderY = (y * TILE_HEIGHT + paddingY - game->camera.position.y - (float) WINDOW_HEIGHT / 2) * global_scale;
            int renderW = (TILE_WIDTH - paddingX * 2) * global_scale;
            int renderH = (TILE_HEIGHT - paddingY * 2) * global_scale;

            if (game->map[y][x] == 1)
                DrawRectangle(renderX, renderY, renderW, renderH, YELLOW);
            else if (game->map[y][x] == 2)
                DrawRectangle(renderX, renderY, renderW, renderH, GREEN);
            // else
            //     DrawRectangle(renderX, renderY, renderW, renderH, YELLOW);
        }
    }

}

void updateActors(Game* game) {

}

int main() {

    SetRandomSeed(time(NULL));

    Game game = {0};

    game.player.speed = 10;

    generateMap(&game);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "hello world");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsKeyPressed(KEY_R)) generateMap(&game);

        Vector2 temp = {0};
        game.player.velocity = Vector2Lerp(game.player.velocity, temp, FRICTION);

        if (IsKeyDown(KEY_W)) game.player.velocity.y = -game.player.speed;
        if (IsKeyDown(KEY_S)) game.player.velocity.y = game.player.speed;
        if (IsKeyDown(KEY_A)) game.player.velocity.x = -game.player.speed;
        if (IsKeyDown(KEY_D)) game.player.velocity.x = game.player.speed;

        Vector2 newPosition = Vector2Add(game.player.position, game.player.velocity);
        game.player.position = newPosition;

        Vector2 halfWindowSize = { (float) WINDOW_WIDTH / 2, (float) WINDOW_HEIGHT / 2 };

        game.camera.target = Vector2Subtract(game.player.position, halfWindowSize);
        game.camera.position = Vector2Lerp(game.camera.position, game.camera.target, 0.05f);

        ClearBackground(BLACK);
        renderMap(&game);

        Vector2 renderingPosition = Vector2Subtract(game.player.position, game.camera.position);
        DrawCircleV(renderingPosition, 8, RED);

        DrawFPS(10, 10);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
