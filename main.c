#include "raylib.h"
#include "gravity.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void DrawGame(void);

int main(void)
{
    int screenWidth = 1280;
    int screenHeight = 720;

    int targetFPS = 60;

    InitWindow(screenWidth, screenHeight, "Gravity Assist");
    SetTargetFPS(targetFPS);
    SetExitKey(0);

    int wMid = screenWidth / 2;
    int hMid = screenHeight / 2;

    GameState gameState = GAME_HOME;

    CameraSettings cameraSettings = {
        .defaultZoom = 3e-2f,
        .minZoom = 1e-6f,
        .maxZoom = 1.0f};

    Camera2D camera = {0};
    camera.rotation = 0.0f;
    camera.zoom = cameraSettings.defaultZoom;

    WarpController timeScale = {
        .val = 1.0f,
        .increment = 1.5f,
        .min = 1.0f,
        .max = 1024.0f};

    HUD playerHUD = {
        .speed = 0.0f,
        .compassTexture = LoadTexture("./textures/hud/compass.png"),
        .arrowTexture = LoadTexture("./textures/hud/arrow_2.png")};

    PlayerStats playerStats = {
        .money = 0,
        .miningXP = 0};

    int numBodies = 0;
    CelestialBody **bodies = NULL;
    CelestialBody *playerShip = NULL;
    float theta = 0.0f; // Barnes-Hut threshold - usually 0.5
    int trailIndex = 0;
    QuadTreeNode *root = NULL;

    int cameraLock = 0;
    Vector2 *cameraLockPosition = NULL;
    camera.target = (Vector2){0, 0};
    camera.offset = (Vector2){wMid, hMid}; // Offset from camera target

    int velocityLock = -1;
    CelestialBody *velocityTarget = NULL;

    GameTextures gameTextures = {0};

    Texture2D starTexture1 = LoadTexture("./textures/star/sun.png");
    gameTextures.numStarTextures = 1;
    gameTextures.starTextures = malloc(sizeof(Texture2D *) * (gameTextures.numStarTextures));
    gameTextures.starTextures[0] = &starTexture1;

    Texture2D planetTexture1 = LoadTexture("./textures/planet/planet_1.png");
    Texture2D planetTexture2 = LoadTexture("./textures/planet/planet_2.png");
    Texture2D planetTexture3 = LoadTexture("./textures/planet/planet_3.png");
    Texture2D planetTexture4 = LoadTexture("./textures/planet/planet_4.png");
    Texture2D planetTexture5 = LoadTexture("./textures/planet/planet_5.png");
    gameTextures.numPlanetTextures = 5;
    gameTextures.planetTextures = malloc(sizeof(Texture2D *) * (gameTextures.numPlanetTextures));
    gameTextures.planetTextures[0] = &planetTexture1;
    gameTextures.planetTextures[1] = &planetTexture2;
    gameTextures.planetTextures[2] = &planetTexture3;
    gameTextures.planetTextures[3] = &planetTexture4;
    gameTextures.planetTextures[4] = &planetTexture5;

    Texture2D moonTexture1 = LoadTexture("./textures/moon/moon_1.png");
    gameTextures.numMoonTextures = 1;
    gameTextures.moonTextures = malloc(sizeof(Texture2D *) * (gameTextures.numMoonTextures));
    gameTextures.moonTextures[0] = &moonTexture1;

    Texture2D shipTexture = LoadTexture("./textures/ship/ship_1.png");
    Texture2D shipThrustTexture = LoadTexture("./textures/ship/ship_1_thrust.png");
    gameTextures.numShipTextures = 2;
    gameTextures.shipTextures = malloc(sizeof(Texture2D *) * (gameTextures.numShipTextures));
    gameTextures.shipTextures[0] = &shipTexture;
    gameTextures.shipTextures[1] = &shipThrustTexture;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        switch (gameState)
        {
        case GAME_HOME:
            if (IsKeyPressed(KEY_ENTER))
            {
                if (!bodies)
                {
                    bodies = initBodies(&numBodies, &gameTextures);
                    playerShip = bodies[0];
                }
                gameState = GAME_RUNNING;
            }
            if (IsKeyPressed(KEY_Q))
            {
                CloseWindow();
                return 0; // Quit immediately
            }
            break;

        case GAME_RUNNING:
            if (IsKeyPressed(KEY_ESCAPE))
            {
                gameState = GAME_PAUSED;
            }

            cameraLockPosition = &bodies[cameraLock]->position;
            camera.target = *cameraLockPosition;

            // Handle input
            if (IsKeyDown(KEY_E))
                incrementWarp(&timeScale, dt);
            if (IsKeyDown(KEY_Q))
                decrementWarp(&timeScale, dt);

            if (IsKeyPressed(KEY_C))
            {
                cameraLock++;
                cameraLock = cameraLock % numBodies;
                cameraLockPosition = &bodies[cameraLock]->position;
            }

            if (IsKeyPressed(KEY_V))
            {
                velocityLock++;
                if (velocityLock >= numBodies)
                {
                    velocityLock = -1;
                    velocityTarget = NULL;
                }
                else
                {
                    velocityTarget = bodies[velocityLock];
                }
            }

            // Apply rotation
            if (IsKeyDown(KEY_D))
                playerShip->rotation += 180.0f * dt; // Rotate right
            if (IsKeyDown(KEY_A))
                playerShip->rotation -= 180.0f * dt; // Rotate left

            // Normalize rotation to keep it within 0-360 degrees
            playerShip->rotation = fmod(playerShip->rotation + 360.0f, 360.0f);
            playerShip->texture = gameTextures.shipTextures[0];
            if (IsKeyDown(KEY_W))
            {
                if (playerShip->shipSettings.state == SHIP_LANDED)
                {
                    takeoffShip(playerShip);
                }
                // Convert rotation to radians for vector calculations
                float radians = playerShip->rotation * PI / 180.0f;
                Vector2 thrustDirection = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
                Vector2 thrust = Vector2Scale(thrustDirection, playerShip->shipSettings.thrust * dt);
                playerShip->velocity = Vector2Add(playerShip->velocity, thrust);
                playerShip->texture = gameTextures.shipTextures[1];
                playerShip->shipSettings.fuel -= playerShip->shipSettings.fuelConsumption;
            }

            camera.zoom += (float)GetMouseWheelMove() * (1e-5f + camera.zoom * (camera.zoom / 4.0f));
            camera.zoom = Clamp(camera.zoom, cameraSettings.minZoom, cameraSettings.maxZoom);

            // Build QuadTree
            root = buildQuadTree(bodies, numBodies);

            // Update physics
            float scaledDt = dt * timeScale.val;

            for (int i = 0; i < numBodies; i++)
            {
                // bodies[i]->rotation += 0.1f;
                if (bodies[i]->type == TYPE_SHIP && bodies[i]->shipSettings.state == SHIP_LANDED)
                {
                    continue;
                }
                Vector2 force = computeForce(root, bodies[i], theta);
                Vector2 accel = Vector2Scale(force, 1.0f / bodies[i]->mass);
                bodies[i]->velocity = Vector2Add(bodies[i]->velocity, Vector2Scale(accel, scaledDt));
                bodies[i]->position = Vector2Add(bodies[i]->position, Vector2Scale(bodies[i]->velocity, scaledDt));
                detectCollisions(bodies, numBodies, root, bodies[i]);
                bodies[i]->previousPositions[trailIndex] = bodies[i]->position;
            }
            trailIndex += 1;
            trailIndex = trailIndex % PREVIOUS_POSITIONS;

            updateShipPosition(playerShip);

            if (velocityLock == -1)
            {
                playerHUD.speed = calculateAbsoluteSpeed(playerShip);
            }
            else
            {
                playerHUD.speed = calculateRelativeSpeed(playerShip, velocityTarget);
            }
            playerHUD.playerRotation = playerShip->rotation;
            playerHUD.velocityTarget = velocityTarget;
            break;

        case GAME_PAUSED:
            if (IsKeyPressed(KEY_ESCAPE))
            {
                gameState = GAME_RUNNING;
            }
            if (IsKeyPressed(KEY_Q))
            {
                CloseWindow();
                return 0; // Quit from pause menu
            }
            break;
        }

        // Render
        BeginDrawing();
        ClearBackground(SPACE_COLOUR);

        if (gameState == GAME_HOME)
        {
            DrawText("Gravity Assist", GetScreenWidth() / 2 - MeasureText("Gravity Assist", 40) / 2, 200, 40, WHITE);
            DrawText("Press ENTER to Start", GetScreenWidth() / 2 - MeasureText("Press ENTER to Start", 20) / 2, 300, 20, WHITE);
            DrawText("Press Q to Quit", GetScreenWidth() / 2 - MeasureText("Press Q to Quit", 20) / 2, 340, 20, WHITE);
        }
        else
        {
            BeginMode2D(camera);
            drawCelestialGrid(bodies, numBodies, camera);
            // drawPreviousPositions(bodies, numBodies);
            drawBodies(bodies, numBodies);

            EndMode2D();

            // GUI
            DrawText("Press ESC to pause & view controls", 10, 10, 20, DARKGRAY);

            drawPlayerHUD(&playerHUD);
            drawPlayerStats(&playerStats);

            DrawFPS(screenWidth - 100, 10);
            DrawText(TextFormat("Camera locked to: %s", bodies[cameraLock]->name), screenWidth - 280, 40, 20, DARKGRAY);
            DrawText(TextFormat("Time Scale: %.1fx", timeScale.val), screenWidth - 200, 70, 20, DARKGRAY);
            DrawText(TextFormat("Zoom Level: %.3fx", calculateNormalisedZoom(&cameraSettings, camera.zoom)), screenWidth - 200, 100, 20, DARKGRAY);

            if (gameState == GAME_PAUSED)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GRAY, 0.5f));
                DrawText("Game Paused", GetScreenWidth() / 2 - MeasureText("Game Paused", 40) / 2, 200, 40, WHITE);
                DrawText("Press ESC to Resume", GetScreenWidth() / 2 - MeasureText("Press ESC to Resume", 20) / 2, 300, 20, WHITE);
                DrawText("Press Q to Quit", GetScreenWidth() / 2 - MeasureText("Press Q to Quit", 20) / 2, 340, 20, WHITE);
                DrawText("Press 'C' to switch camera", 10, 40, 20, WHITE);
                DrawText("Press 'Q' and 'E' to time warp", 10, 70, 20, WHITE);
                DrawText("Scroll to zoom", 10, 100, 20, WHITE);
                DrawText("Press 'V' to switch velocity lock", 10, 130, 20, WHITE);
            }
        }

        EndDrawing();
    }

    freeCelestialBodies(bodies, numBodies);
    freeGameTextures(gameTextures);

    CloseWindow();
    return 0;
}

// void DrawGame(void) {
//     BeginDrawing();

//     ClearBackground(SPACE_COLOUR);

// }