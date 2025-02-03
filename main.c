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

    Camera2D camera = {0};
    camera.rotation = 0.0f; // Camera rotation in degrees
    camera.zoom = 0.1f;     // Camera zoom (1.0 is normal size)

    Ship playerShip = {
        {0, -2e4},
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
    camera.target = *cameraLockPosition;     // Camera target (where the camera looks at)
    camera.offset = (Vector2){*wMid, *hMid}; // Offset from camera target

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
        DrawFPS(*screenWidth - 100, 10);
        if (cameraLock >= 0)
        {
            DrawText(TextFormat("Camera locked to: body %d", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
        }
        else
        {
            DrawText(TextFormat("Camera locked to: ship", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
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
        {{0, 0},
         {0, 0},
         2e24,
         6.378e2,
         BLACK,
         0.0f}};

    int cameraLock = 0;
    Vector2 *cameraLockPosition = {0};

    // Planet level
    Camera2D camera = {0};
    camera.rotation = 0.0f; // Camera rotation in degrees
    camera.zoom = 1.0f;     // Camera zoom (1.0 is normal size)

    Ship playerShip = {
        {0, -planet[0].radius},
        {0, 0},
        1e3,
        0.0f,
        1e1f};

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

    levelPlanet(&screenWidth, &screenHeight, &wMid, &hMid, &targetFPS);

    // levelSpace(&screenWidth, &screenHeight, &wMid, &hMid, &targetFPS);

    CloseWindow();
    return 0;
}