#include "raylib.h"
#include "gravity.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(void)
{
    int screenWidth = 1280;
    int screenHeight = 720;

    int wMid = screenWidth / 2;
    int hMid = screenHeight / 2;

    int targetFPS = 60;

    bool enableTrajectories = true;

    InitWindow(screenWidth, screenHeight, "Gravity Assist");
    SetTargetFPS(targetFPS);

    Camera2D camera = {0};
    camera.rotation = 0.0f; // Camera rotation in degrees
    camera.zoom = 0.1f;     // Camera zoom (1.0 is normal size)

    Ship playerShip = {
        {0, -3e4},
        {1e3, 0},
        1e3,
        0.0f,
        1e2f};

    // sol system based on earth's home solar system

    // 0th index in system is the star
    Body solSystem[3] = {
        {{0, 0}, // Position
         {0, 0}, // Velocity
         2e20,   // Mass (kg)
         1000,   // Radius
         YELLOW, // Colour
         0.0f},  // Rotation
        {{0, 0},
         {0, 0},
         2e16,
         10,
         BLUE,
         0.0f},
        {{0, 0},
         {0, 0},
         2e15,
         9,
         RED,
         0.0f}};

    int solSystemBodies = sizeof(solSystem) / sizeof(Body);
    int cameraLock = 0;
    Vector2 *cameraLockPosition = {0};

    cameraLockPosition = &solSystem[cameraLock].position;
    camera.target = *cameraLockPosition;   // Camera target (where the camera looks at)
    camera.offset = (Vector2){wMid, hMid}; // Offset from camera target

    // initialiseRandomPositions(solSystemBodies, solSystem, screenWidth - 100, screenHeight - 100);
    // initialiseStableOrbits(solSystemBodies, solSystem, G);

    initialiseOrbits(solSystemBodies, solSystem);

    const int trailSize = 1000;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        updateBodies(solSystemBodies, solSystem, dt);

        updateShip(&playerShip, solSystemBodies, solSystem, dt);

        if (IsKeyPressed(KEY_T))
        {
            enableTrajectories = !enableTrajectories;
        }

        // Predict trajectories of celestial bodies
        if (enableTrajectories)
        {
            for (int i = 0; i < solSystemBodies; i++)
            {
                solSystem[i].futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
                predictBodyTrajectory(&solSystem[i], solSystem, solSystemBodies, solSystem[i].futurePositions);
                // predictTrajectory(&solSystem[i].position, &solSystem[i].velocity, &solSystem[i].mass, solSystem, solSystemBodies, solSystem[i].trajectory, G);
            }

            // Predict ship trajectory
            // TODO: Revise ship trajectory prediction as it is constantly changing
            playerShip.futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
            predictShipTrajectory(&playerShip, solSystem, solSystemBodies, playerShip.futurePositions);
            // predictTrajectory(&playerShip.position, &playerShip.velocity, &playerShip.mass, solSystem, solSystemBodies, playerShip.trajectory, G);
        }

        camera.target = *cameraLockPosition;

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

        // Camera zoom controls
        camera.zoom += ((float)GetMouseWheelMove() * 0.005f);
        camera.zoom = _Clamp(camera.zoom, 0.001f, 10.0f); // Limit zoom level

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        if (enableTrajectories)
        {
            // Draw trajectories so they're at the bottom
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
        Vector2 shipSize = {8, 16}; // Width, Height of ship triangle
        Vector2 shipPoints[3] = {
            {0, -shipSize.y / 2},
            {-shipSize.x / 2, shipSize.y / 2},
            {shipSize.x / 2, shipSize.y / 2}};

        // Rotate ship points
        for (int i = 0; i < 3; i++)
        {
            float radians = playerShip.rotation * PI / 180.0f;
            float x = shipPoints[i].x * cosf(radians) - shipPoints[i].y * sinf(radians);
            float y = shipPoints[i].x * sinf(radians) + shipPoints[i].y * cosf(radians);
            shipPoints[i] = (Vector2){x, y};
            shipPoints[i] = _Vector2Add(&shipPoints[i], &playerShip.position);
        }
        DrawTriangle(shipPoints[0], shipPoints[1], shipPoints[2], WHITE);

        EndMode2D();

        // GUI
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawText("Press 'T' to toggle trajectories", 10, 40, 20, DARKGRAY);
        DrawText("Press 'L' to switch camera", 10, 70, 20, DARKGRAY);
        DrawFPS(screenWidth - 100, 10);
        if (cameraLock >= 0)
        {
            DrawText(TextFormat("Camera locked to: body %d", cameraLock), screenWidth - 280, 40, 20, DARKGRAY);
        }
        else
        {
            DrawText(TextFormat("Camera locked to: ship", cameraLock), screenWidth - 280, 40, 20, DARKGRAY);
        }

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

    CloseWindow();
    return 0;
}