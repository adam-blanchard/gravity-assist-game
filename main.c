#include "raylib.h"
#include "gravity.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void levelSpace(int *screenWidth, int *screenHeight, int *wMid, int *hMid, int *targetFPS)
{
    bool enableTrajectories = true;
    float crashThreshold = 1e3f;

    HUD playerHUD = {
        .speed = 0,
        .arrowTexture = LoadTexture("./textures/arrow.png")};

    WarpController timeScale = {
        .val = 1.0f,
        .increment = 1.5f,
        .min = 1.0f,
        .max = 16.0f};

    CameraSettings cameraSettings = {
        .defaultZoom = 1.0f,
        .minZoom = 0.001f,
        .maxZoom = 10.0f};

    Camera2D camera = {0};
    camera.rotation = 0.0f;
    camera.zoom = cameraSettings.defaultZoom;

    // Mass and thrust based on SpaceX Starship
    Ship playerShip = {
        .position = {0},
        .velocity = {0},
        .mass = 1e5,
        .rotation = 0.0f,
        .thrust = 3.5e1f,
        .fuel = 100.0f,
        .fuelConsumption = 0.0f,
        .colliderRadius = 6.0f,
        .state = SHIP_LANDED,
        .alive = 1,
        .idleTexture = LoadTexture("./textures/ship.png"),
        .thrustTexture = LoadTexture("./textures/ship_thrust.png")};

    Body solSystem[3] = {
        {.name = "Sol",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e20,
         .radius = 1e4,
         .renderColour = YELLOW,
         .rotation = 0.0f,
         .fontSize = 100},
        {.name = "Earth",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e16,
         .radius = 1e3,
         .orbitalPeriod = 120.0f,
         .renderColour = BLUE,
         .rotation = 0.0f,
         .fontSize = 10},
        {.name = "Mars",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e15,
         .radius = 5e2,
         .orbitalPeriod = 240.0f,
         .renderColour = RED,
         .rotation = 0.0f,
         .fontSize = 10}};

    int solSystemBodies = sizeof(solSystem) / sizeof(Body);
    int cameraLock = -1;
    Vector2 *cameraLockPosition = {0};

    cameraLockPosition = &playerShip.position;
    camera.target = *cameraLockPosition;     // Camera target (where the camera looks at)
    camera.offset = (Vector2){*wMid, *hMid}; // Offset from camera target

    int speedLock = -1;
    Vector2 defaultVelocity = {0};
    Vector2 *targetVelocity = &defaultVelocity;

    initialiseOrbits(solSystemBodies, solSystem);

    // spawnRocketOnBody(&playerShip, &solSystem[1]);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        camera.target = *cameraLockPosition;

        // Handle input
        if (IsKeyDown(KEY_E))
            incrementWarp(&timeScale, dt);
        if (IsKeyDown(KEY_Q))
            decrementWarp(&timeScale, dt);

        if (IsKeyPressed(KEY_T))
            enableTrajectories = !enableTrajectories;

        if (IsKeyPressed(KEY_L))
        {
            cameraLock++;
            if (cameraLock >= solSystemBodies)
            {
                cameraLock = -1;
                cameraLockPosition = &playerShip.position;
            }
            else
            {
                cameraLockPosition = &solSystem[cameraLock].position;
            }
        }

        if (IsKeyPressed(KEY_V))
        {
            speedLock++;
            if (speedLock >= solSystemBodies)
            {
                speedLock = -1;
                targetVelocity = &defaultVelocity;
            }
            else
            {
                targetVelocity = &solSystem[speedLock].velocity;
            }
        }

        camera.zoom += (float)GetMouseWheelMove() * 0.005f;
        camera.zoom = _Clamp(camera.zoom, cameraSettings.minZoom, cameraSettings.maxZoom);

        // Apply physics updates
        float scaledDt = dt * timeScale.val;

        updateBodies(solSystemBodies, solSystem, scaledDt);

        updateShip(&playerShip, solSystemBodies, solSystem, scaledDt);

        playerHUD.speed = calculateShipSpeed(&playerShip, targetVelocity);

        for (int i = 0; i < solSystemBodies; i++)
        {
            if (checkCollision(&playerShip, &solSystem[i]))
            {
                float relativeSpeed = calculateShipSpeed(&playerShip, &solSystem[i].velocity);
                if (relativeSpeed >= crashThreshold)
                {
                    playerShip.alive = 0;
                }

                if (relativeSpeed < crashThreshold)
                {
                    landShip(&playerShip, &solSystem[i]);
                }
            }
        }

        // Predict trajectories of celestial bodies
        if (enableTrajectories)
        {
            for (int i = 0; i < solSystemBodies; i++)
            {
                free(solSystem[i].futurePositions);
                solSystem[i].futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
                predictBodyTrajectory(&solSystem[i], solSystem, solSystemBodies, solSystem[i].futurePositions);
                // predictTrajectory(&solSystem[i].position, &solSystem[i].velocity, &solSystem[i].mass, solSystem, solSystemBodies, solSystem[i].trajectory, G);
            }

            // Predict ship trajectory
            // TODO: Revise ship trajectory prediction as it is constantly changing
            free(playerShip.futurePositions);
            playerShip.futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
            predictShipTrajectory(&playerShip, solSystem, solSystemBodies, playerShip.futurePositions);
            // predictTrajectory(&playerShip.position, &playerShip.velocity, &playerShip.mass, solSystem, solSystemBodies, playerShip.trajectory, G);
        }

        // Draw entities to screen
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        // Draw trajectories first so they're at the bottom
        if (enableTrajectories)
        {
            for (int i = 0; i < solSystemBodies; i++)
            {
                for (int j = 0; j < TRAJECTORY_STEPS - 1; j++)
                {
                    DrawLineV(solSystem[i].futurePositions[j], solSystem[i].futurePositions[j + 1], (Color){100, 100, 100, 255});
                }
            }

            // Draw ship trajectory
            for (int i = 0; i < TRAJECTORY_STEPS - 1; i++)
            {
                DrawLineV(playerShip.futurePositions[i], playerShip.futurePositions[i + 1], (Color){150, 150, 150, 255});
            }
        }

        // Draw celestial bodies
        for (int i = 0; i < solSystemBodies; i++)
        {
            DrawCircle(solSystem[i].position.x, solSystem[i].position.y, solSystem[i].radius, solSystem[i].renderColour);
        }

        // Draw Ship
        Rectangle shipSource = {0, 0, (float)playerShip.activeTexture->width, (float)playerShip.activeTexture->height};
        Rectangle shipDest = {playerShip.position.x, playerShip.position.y, (float)playerShip.activeTexture->width, (float)playerShip.activeTexture->height};
        Vector2 shipOrigin = {(float)playerShip.activeTexture->width / 2, (float)playerShip.activeTexture->height / 2};
        DrawTexturePro(*playerShip.activeTexture, shipSource, shipDest, shipOrigin, playerShip.rotation, WHITE);

        EndMode2D();

        // GUI
        // Left
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawText("Press 'T' to toggle trajectories", 10, 40, 20, DARKGRAY);
        DrawText("Press 'L' to switch camera", 10, 70, 20, DARKGRAY);
        DrawText("Press 'V' to switch velocity lock", 10, 100, 20, DARKGRAY);
        DrawText("Press 'Q' and 'E' to time warp", 10, 130, 20, DARKGRAY);

        // Right
        DrawFPS(*screenWidth - 100, 10);
        if (cameraLock >= 0)
        {
            DrawText(TextFormat("Camera locked to: body %d", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
        }
        else
        {
            DrawText(TextFormat("Camera locked to: ship", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
        }
        DrawText(TextFormat("Time Scale: %.1fx", timeScale.val), *screenWidth - 200, 70, 20, DARKGRAY);
        DrawText(TextFormat("Fuel Level: %.1fpct", playerShip.fuel), *screenWidth - 200, 100, 20, DARKGRAY);

        // Centre
        if (speedLock >= 0)
        {
            DrawText(TextFormat("Relative - body: %i", speedLock), *wMid, *screenHeight - 120, 12, WHITE);
        }
        else
        {
            DrawText("Absolute", *wMid, *screenHeight - 120, 12, WHITE);
        }
        DrawText(TextFormat("%.1fm/s", playerHUD.speed), *wMid, *screenHeight - 100, 12, WHITE);
        Rectangle arrowSource = {0, 0, (float)playerHUD.arrowTexture.width, (float)playerHUD.arrowTexture.height};
        Rectangle arrowDest = {*wMid, *screenHeight - 50, (float)playerHUD.arrowTexture.width, (float)playerHUD.arrowTexture.height};
        Vector2 arrowOrigin = {(float)playerHUD.arrowTexture.width / 2, (float)playerHUD.arrowTexture.height / 2};
        DrawTexturePro(playerHUD.arrowTexture, arrowSource, arrowDest, arrowOrigin, playerShip.rotation, WHITE);

        EndDrawing();
    }

    if (enableTrajectories)
    {
        for (int i = 0; i < solSystemBodies; i++)
        {
            free(solSystem[i].futurePositions);
        }
        free(playerShip.futurePositions);
    }

    UnloadTexture(playerShip.idleTexture);
    UnloadTexture(playerShip.thrustTexture);
}

int main(void)
{
    int screenWidth = 1280;
    int screenHeight = 720;

    int targetFPS = 60;

    InitWindow(screenWidth, screenHeight, "Gravity Assist");
    SetTargetFPS(targetFPS);

    int wMid = screenWidth / 2;
    int hMid = screenHeight / 2;

    levelSpace(&screenWidth, &screenHeight, &wMid, &hMid, &targetFPS);

    CloseWindow();
    return 0;
}