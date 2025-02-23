#include "raylib.h"
#include "gravity.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void levelSpace(int *screenWidth, int *screenHeight, int *wMid, int *hMid, int *targetFPS)
{
    // Orbital level
    bool enableTrajectories = true;

    float timeScale = 1.0f;          // Default: normal speed
    const float minTimeScale = 1.0f; // Minimum speed (slowest)
    const float maxTimeScale = 16.0f;

    Camera2D camera = {0};
    camera.rotation = 0.0f; // Camera rotation in degrees
    camera.zoom = 0.01f;    // Camera zoom (1.0 is normal size)

    Ship playerShip = {
        .position = {0, -2e4},
        .velocity = {1e3, 0},
        .mass = 1e3,
        .rotation = 0.0f,
        .thrust = 1e2f,
        .fuel = 100.0f,
        .colliderRadius = 16.0f,
        .idleTexture = LoadTexture("./textures/ship.png"),
        .thrustTexture = LoadTexture("./textures/ship_thrust.png")};

    // sol system based on earth's home solar system

    // 0th index in system is the star
    Body solSystem[3] = {
        {.name = "Sol",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e20,
         .radius = 1000,
         .renderColour = YELLOW,
         .rotation = 0.0f},
        {.name = "Earth",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e16,
         .radius = 10,
         .renderColour = BLUE,
         .rotation = 0.0f},
        {.name = "Mars",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e15,
         .radius = 5,
         .renderColour = RED,
         .rotation = 0.0f}};

    int solSystemBodies = sizeof(solSystem) / sizeof(Body);
    int cameraLock = 0;
    Vector2 *cameraLockPosition = {0};

    cameraLockPosition = &solSystem[cameraLock].position;
    camera.target = *cameraLockPosition;     // Camera target (where the camera looks at)
    camera.offset = (Vector2){*wMid, *hMid}; // Offset from camera target

    // initialiseRandomPositions(solSystemBodies, solSystem, screenWidth - 100, screenHeight - 100);
    // initialiseStableOrbits(solSystemBodies, solSystem, G);

    initialiseOrbits(solSystemBodies, solSystem);

    const int trailSize = 1000;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        if (IsKeyPressed(KEY_Q))
            timeScale -= 1.0f; // Slow down
        if (IsKeyPressed(KEY_E))
            timeScale += 1.0f; // Speed up
        if (timeScale < minTimeScale)
            timeScale = minTimeScale;
        if (timeScale > maxTimeScale)
            timeScale = maxTimeScale;

        float scaledDt = dt * timeScale;

        updateBodies(solSystemBodies, solSystem, scaledDt);

        updateShip(&playerShip, solSystemBodies, solSystem, scaledDt);

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
        camera.zoom = _Clamp(camera.zoom, 0.008f, 10.0f); // Limit zoom level

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
        Rectangle source = {0, 0, (float)playerShip.activeTexture->width, (float)playerShip.activeTexture->height};
        Rectangle dest = {playerShip.position.x, playerShip.position.y, (float)playerShip.activeTexture->width, (float)playerShip.activeTexture->height};
        Vector2 origin = {(float)playerShip.activeTexture->width / 2, (float)playerShip.activeTexture->height / 2};
        DrawTexturePro(*playerShip.activeTexture, source, dest, origin, playerShip.rotation, WHITE);

        EndMode2D();

        // GUI
        // Left
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawText("Press 'T' to toggle trajectories", 10, 40, 20, DARKGRAY);
        DrawText("Press 'L' to switch camera", 10, 70, 20, DARKGRAY);
        DrawText("Press 'Q' and 'E' to time warp", 10, 100, 20, DARKGRAY);

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
        DrawText(TextFormat("Time Scale: %.1fx", timeScale), *screenWidth - 200, 70, 20, DARKGRAY);
        DrawText(TextFormat("Fuel Level: %.1fpct", playerShip.fuel), *screenWidth - 200, 100, 20, DARKGRAY);

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

bool checkPlanetCollision(Ship *ship, Body *planet, float *heightThreshold)
{
    // If velocity is 0, ship is on the ground
    // If velocity is below threshold, land ship
    // If velocity is above threshold, crash
    float velocityThreshold = 100.0f;

    Vector2 direction = _Vector2Subtract(&ship->position, &planet->position);
    float distance = _Vector2Length(direction) - planet->radius;

    return distance <= (planet->radius + *heightThreshold);
}

void levelPlanet(int *screenWidth, int *screenHeight, int *wMid, int *hMid, int *targetFPS)
{
    Body planet[1] = {
        {.name = "Earth",
         .position = {0, 0},
         .velocity = {0, 0},
         .mass = 2e24,
         .radius = 6.378e3,
         .renderColour = BLACK,
         .rotation = 0.0f}};

    int cameraLock = 0;
    Vector2 *cameraLockPosition = {0};

    // Planet level
    Camera2D camera = {0};
    camera.rotation = 0.0f; // Camera rotation in degrees
    camera.zoom = 1.0f;     // Camera zoom (1.0 is normal size)

    Ship playerShip = {
        .position = {0, -planet[0].radius},
        .velocity = {0, 0},
        .mass = 4.6e6,
        .rotation = 0.0f,
        .thrust = 1e-1f};

    float rotationRate = 180.0f;
    float fuelAmount = 100.0f;
    float heightThreshold = 5.0f;
    float altitude = 0.0f;
    float airResistance = 0.99f;

    cameraLockPosition = &playerShip.position;
    camera.target = *cameraLockPosition;     // Camera target (where the camera looks at)
    camera.offset = (Vector2){*wMid, *hMid}; // Offset from camera target

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        Vector2 direction = _Vector2Subtract(&playerShip.position, &planet[0].position);
        float altitude = _Vector2Length(direction) - planet[0].radius;

        // Apply rotation
        if (IsKeyDown(KEY_D))
            playerShip.rotation += rotationRate * dt; // Rotate right
        if (IsKeyDown(KEY_A))
            playerShip.rotation -= rotationRate * dt; // Rotate left

        // Normalize rotation to keep it within 0-360 degrees
        playerShip.rotation = fmod(playerShip.rotation + 360.0f, 360.0f);

        // Apply thrust
        if (IsKeyDown(KEY_W))
        {
            // Convert rotation to radians for vector calculations
            float radians = playerShip.rotation * PI / 180.0f;
            Vector2 thrustDirection = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
            Vector2 thrust = _Vector2Scale(thrustDirection, playerShip.thrust * dt);
            playerShip.velocity = _Vector2Add(&playerShip.velocity, &thrust);
        }

        Vector2 gravity = gravitationalForce2(&playerShip.position, &planet[0].position, playerShip.mass, planet[0].mass);

        if (checkPlanetCollision(&playerShip, &planet[0], &heightThreshold))
        {
            gravity = (Vector2){0, 0};
        }

        Vector2 acceleration = {gravity.x / playerShip.mass, gravity.y / playerShip.mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, dt);
        playerShip.velocity = _Vector2Add(&playerShip.velocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(playerShip.velocity, dt);
        playerShip.position = _Vector2Add(&playerShip.position, &scaledVelocity);

        camera.target = *cameraLockPosition;

        // if (IsKeyPressed(KEY_L))
        // {
        //     cameraLock++;
        //     if (cameraLock >= solSystemBodies)
        //     {
        //         cameraLock = -1;
        //         cameraLockPosition = &playerShip.position;
        //     }
        //     else
        //     {
        //         cameraLockPosition = &solSystem[cameraLock].position;
        //     }

        // Camera zoom controls
        camera.zoom += ((float)GetMouseWheelMove() * 0.05f);
        camera.zoom = _Clamp(camera.zoom, 0.001f, 10.0f); // Limit zoom level

        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode2D(camera);

        // Draw ground
        DrawCircle(0, 0, planet[0].radius, BLACK);

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
        DrawTriangle(shipPoints[0], shipPoints[1], shipPoints[2], RED);

        EndMode2D();

        // GUI
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawText("Press 'L' to switch camera", 10, 40, 20, DARKGRAY);
        DrawFPS(*screenWidth - 100, 10);
        DrawText(TextFormat("Altitude: %d", altitude), *screenWidth - 120, 40, 20, DARKGRAY);
        // if (cameraLock >= 0)
        // {
        //     DrawText(TextFormat("Camera locked to: body %d", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
        // }
        // else
        // {
        //     DrawText(TextFormat("Camera locked to: ship", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
        // }

        EndDrawing();
    }
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