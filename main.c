#include "raylib.h"
#include "gravity.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// void levelSpace(int *screenWidth, int *screenHeight, int *wMid, int *hMid, int *targetFPS)
// {
//     bool enableTrajectories = false;
//     float crashThreshold = 1e3f;
//     const float textureCover = 0.67f;
//     float textureScale = 1 / textureCover;

//     HUD playerHUD = {
//         .speed = 0,
//         .arrowTexture = LoadTexture("./textures/arrow.png")};

//     WarpController timeScale = {
//         .val = 1.0f,
//         .increment = 1.5f,
//         .min = 1.0f,
//         .max = 16.0f};

//     CameraSettings cameraSettings = {
//         .defaultZoom = 2e0f,
//         .minZoom = 1e-4f,
//         .maxZoom = 5.0f};

//     Camera2D camera = {0};
//     camera.rotation = 0.0f;
//     camera.zoom = cameraSettings.defaultZoom;

//     /*
//     Real-world measurements

//         Planet  | Mass     | Orbital Period | Radii
//         Sun     | 1.989e30 | 0              | 6.957e8
//         Mercury | 3.301e23 | 89.97          | 2.439e6
//         Venus   | 4.867e24 | 224.7          | 6.051e6
//         Earth   | 5.972e24 | 365.25         | 6.371e6
//         Mars    | 6.417e23 | 686.98         | 3.389e6
//         Jupiter | 1.898e27 | 4332.59        | 6.991e7
//         Saturn  | 5.683e26 | 10759.22       | 5.823e7
//         Uranus  | 8.681e25 | 30589.74       | 2.536e7
//         Neptune | 1.024e26 | 59800.00       | 2.462e7

//     Scaling
//         Mass - divide by 1e12
//         Radius - divide by 1e3
//         Orbital period - times by 1e3
//     */

//     // Mass and thrust based on SpaceX Starship
//     Ship playerShip = {
//         .position = {0, 0},
//         .velocity = {0, 0},
//         .mass = 1e5,
//         .rotation = 0.0f,
//         .thrust = 3.5e1f,
//         .fuel = 100.0f,
//         .fuelConsumption = 0.0f,
//         .colliderRadius = 8.0f,
//         .state = SHIP_FLYING, // Initialisation to SHIP_LANDED results in a segfault
//         .alive = 1,
//         .activeTexture = NULL,
//         .idleTexture = LoadTexture("./textures/ship.png"),
//         .thrustTexture = LoadTexture("./textures/ship_thrust.png")};

//     Body solSystem[4] = {
//         {.name = "Sol",
//          .type = BODY_STAR,
//          .position = {0, 0},
//          .velocity = {0, 0},
//          .mass = 2e18,
//          .radius = 7e2,
//          .rotation = 0.0f,
//          .fontSize = 100,
//          .texture = LoadTexture("./textures/sun.png")},
//         {.name = "Earth",
//          .type = BODY_PLANET,
//          .position = {0, 0},
//          .velocity = {0, 0},
//          .mass = 6e12,
//          .radius = 6e2,
//          .orbitalPeriod = 3.65e1f,
//          .rotation = 0.0f,
//          .fontSize = 10,
//          .texture = LoadTexture("./textures/earth.png")},
//         {.name = "Moon",
//          .type = BODY_MOON,
//          .position = {0, 0},
//          .velocity = {0, 0},
//          .mass = 7e8,
//          .radius = 2e2,
//          .orbitalPeriod = 2.8e3f,
//          .rotation = 0.0f,
//          .texture = LoadTexture("./textures/moon.png")},
//         {.name = "Mars",
//          .type = BODY_PLANET,
//          .position = {0, 0},
//          .velocity = {0, 0},
//          .mass = 6.417e11,
//          .radius = 3.389e3,
//          .orbitalPeriod = 6.8698e5f,
//          .rotation = 0.0f,
//          .fontSize = 10,
//          .texture = LoadTexture("./textures/mars.png")}};

//     solSystem[1].orbitalBodyIndex = 0; // Earth orbits around the Sun
//     solSystem[2].orbitalBodyIndex = 1; // The Moon orbits around the Earth
//     solSystem[3].orbitalBodyIndex = 0; // Mars orbits around the Sun

//     int solSystemBodies = sizeof(solSystem) / sizeof(Body);
//     int cameraLock = -1;
//     Vector2 *cameraLockPosition = NULL;

//     cameraLockPosition = &playerShip.position;
//     camera.target = *cameraLockPosition;     // Camera target (where the camera looks at)
//     camera.offset = (Vector2){*wMid, *hMid}; // Offset from camera target

//     int speedLock = -1;
//     Vector2 defaultVelocity = {0};
//     Vector2 *targetVelocity = &defaultVelocity;

//     initialiseOrbitsv2(solSystemBodies, solSystem);

//     spawnRocketOnBody(&playerShip, &solSystem[1]);

//     while (!WindowShouldClose())
//     {
//         float dt = GetFrameTime();
//         camera.target = *cameraLockPosition;

//         // Handle input
//         if (IsKeyDown(KEY_E))
//             incrementWarp(&timeScale, dt);
//         if (IsKeyDown(KEY_Q))
//             decrementWarp(&timeScale, dt);

//         if (IsKeyPressed(KEY_T))
//             enableTrajectories = !enableTrajectories;

//         if (IsKeyPressed(KEY_L))
//         {
//             cameraLock++;
//             if (cameraLock >= solSystemBodies)
//             {
//                 cameraLock = -1;
//                 cameraLockPosition = &playerShip.position;
//             }
//             else
//             {
//                 cameraLockPosition = &solSystem[cameraLock].position;
//             }
//         }

//         if (IsKeyPressed(KEY_V))
//         {
//             speedLock++;
//             if (speedLock >= solSystemBodies)
//             {
//                 speedLock = -1;
//                 targetVelocity = &defaultVelocity;
//             }
//             else
//             {
//                 targetVelocity = &solSystem[speedLock].velocity;
//             }
//         }

//         camera.zoom += (float)GetMouseWheelMove() * 0.001f;
//         camera.zoom = _Clamp(camera.zoom, cameraSettings.minZoom, cameraSettings.maxZoom);

//         // Apply physics updates
//         float scaledDt = dt * timeScale.val;

//         updateBodiesv2(solSystemBodies, solSystem, scaledDt);

//         updateShip(&playerShip, solSystemBodies, solSystem, scaledDt);

//         playerHUD.speed = calculateShipSpeed(&playerShip, targetVelocity);

//         for (int i = 0; i < solSystemBodies; i++)
//         {
//             if (checkCollision(&playerShip, &solSystem[i]))
//             {
//                 float relativeSpeed = calculateShipSpeed(&playerShip, &solSystem[i].velocity);
//                 if (relativeSpeed >= crashThreshold)
//                 {
//                     playerShip.alive = 0;
//                 }

//                 if (relativeSpeed < crashThreshold)
//                 {
//                     landShip(&playerShip, &solSystem[i]);
//                 }
//             }
//         }

//         // Predict trajectories of celestial bodies
//         if (enableTrajectories)
//         {
//             for (int i = 0; i < solSystemBodies; i++)
//             {
//                 if (sizeof(solSystem[i].futurePositions) > 0)
//                 {
//                     free(solSystem[i].futurePositions);
//                 }
//                 solSystem[i].futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
//                 predictBodyTrajectory(&solSystem[i], solSystem, solSystemBodies, solSystem[i].futurePositions);
//                 // predictTrajectory(&solSystem[i].position, &solSystem[i].velocity, &solSystem[i].mass, solSystem, solSystemBodies, solSystem[i].trajectory, G);
//             }

//             // Predict ship trajectory
//             // TODO: Revise ship trajectory prediction as it is constantly changing
//             if (sizeof(playerShip.futurePositions) > 0)
//             {
//                 free(playerShip.futurePositions);
//             }
//             playerShip.futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
//             predictShipTrajectory(&playerShip, solSystem, solSystemBodies, playerShip.futurePositions);
//             // predictTrajectory(&playerShip.position, &playerShip.velocity, &playerShip.mass, solSystem, solSystemBodies, playerShip.trajectory, G);
//         }

//         // Draw entities to screen
//         BeginDrawing();
//         ClearBackground(BLACK);
//         BeginMode2D(camera);

//         // Draw trajectories first so they're at the bottom
//         if (enableTrajectories)
//         {
//             for (int i = 0; i < solSystemBodies; i++)
//             {
//                 for (int j = 0; j < TRAJECTORY_STEPS - 1; j++)
//                 {
//                     DrawLineV(solSystem[i].futurePositions[j], solSystem[i].futurePositions[j + 1], (Color){100, 100, 100, 255});
//                 }
//             }

//             // Draw ship trajectory
//             for (int i = 0; i < TRAJECTORY_STEPS - 1; i++)
//             {
//                 DrawLineV(playerShip.futurePositions[i], playerShip.futurePositions[i + 1], (Color){150, 150, 150, 255});
//             }
//         }

//         // Draw celestial bodies
//         for (int i = 0; i < solSystemBodies; i++)
//         {
//             // Draw Body collider for debug
//             // DrawCircle(solSystem[i].position.x, solSystem[i].position.y, solSystem[i].radius, WHITE);
//             float scale = ((solSystem[i].radius * 2) / solSystem[i].texture.width) * textureScale;
//             Vector2 pos = (Vector2){solSystem[i].position.x - (solSystem[i].radius * textureScale), solSystem[i].position.y - (solSystem[i].radius * textureScale)};
//             DrawTextureEx(solSystem[i].texture, pos, solSystem[i].rotation, scale, WHITE);
//         }

//         // Draw Ship collider for debug
//         // DrawCircle(playerShip.position.x, playerShip.position.y, playerShip.colliderRadius, WHITE);

//         // Draw Ship
//         Rectangle shipSource = {0, 0, (float)playerShip.activeTexture->width, (float)playerShip.activeTexture->height};
//         Rectangle shipDest = {playerShip.position.x, playerShip.position.y, (float)playerShip.activeTexture->width, (float)playerShip.activeTexture->height};
//         Vector2 shipOrigin = {(float)playerShip.activeTexture->width / 2, (float)playerShip.activeTexture->height / 2};
//         DrawTexturePro(*playerShip.activeTexture, shipSource, shipDest, shipOrigin, playerShip.rotation, WHITE);

//         EndMode2D();

//         // GUI
//         // Left
//         DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
//         DrawText("Press 'T' to toggle trajectories", 10, 40, 20, DARKGRAY);
//         DrawText("Press 'L' to switch camera", 10, 70, 20, DARKGRAY);
//         DrawText("Press 'V' to switch velocity lock", 10, 100, 20, DARKGRAY);
//         DrawText("Press 'Q' and 'E' to time warp", 10, 130, 20, DARKGRAY);

//         // Right
//         DrawFPS(*screenWidth - 100, 10);
//         if (cameraLock >= 0)
//         {
//             DrawText(TextFormat("Camera locked to: body %d", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
//         }
//         else
//         {
//             DrawText(TextFormat("Camera locked to: ship", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
//         }
//         DrawText(TextFormat("Zoom Level: %.2f", camera.zoom), *screenWidth - 200, 70, 20, DARKGRAY);
//         DrawText(TextFormat("Time Scale: %.1fx", timeScale.val), *screenWidth - 200, 100, 20, DARKGRAY);
//         DrawText(TextFormat("Fuel Level: %.1fpct", playerShip.fuel), *screenWidth - 200, 130, 20, DARKGRAY);

//         // Centre
//         if (speedLock >= 0)
//         {
//             DrawText(TextFormat("Relative - body: %i", speedLock), *wMid, *screenHeight - 120, 12, WHITE);
//         }
//         else
//         {
//             DrawText("Absolute", *wMid, *screenHeight - 120, 12, WHITE);
//         }
//         DrawText(TextFormat("%.1fm/s", playerHUD.speed), *wMid, *screenHeight - 100, 12, WHITE);
//         Rectangle arrowSource = {0, 0, (float)playerHUD.arrowTexture.width, (float)playerHUD.arrowTexture.height};
//         Rectangle arrowDest = {*wMid, *screenHeight - 50, (float)playerHUD.arrowTexture.width, (float)playerHUD.arrowTexture.height};
//         Vector2 arrowOrigin = {(float)playerHUD.arrowTexture.width / 2, (float)playerHUD.arrowTexture.height / 2};
//         DrawTexturePro(playerHUD.arrowTexture, arrowSource, arrowDest, arrowOrigin, playerShip.rotation, WHITE);

//         EndDrawing();
//     }

//     if (enableTrajectories)
//     {
//         for (int i = 0; i < solSystemBodies; i++)
//         {
//             free(solSystem[i].futurePositions);
//         }
//         free(playerShip.futurePositions);
//     }

//     UnloadTexture(playerShip.idleTexture);
//     UnloadTexture(playerShip.thrustTexture);

//     for (int i = 0; i < solSystemBodies; i++)
//     {
//         UnloadTexture(solSystem[i].texture);
//     }
// }

void levelUniverse(int *screenWidth, int *screenHeight, int *wMid, int *hMid, int *targetFPS)
{
    CameraSettings cameraSettings = {
        .defaultZoom = 1.6e0f,
        .minZoom = 1e-4f,
        .maxZoom = 5.0f};

    Camera2D camera = {0};
    camera.rotation = 0.0f;
    camera.zoom = cameraSettings.defaultZoom;

    WarpController timeScale = {
        .val = 1.0f,
        .increment = 1.5f,
        .min = 1.0f,
        .max = 32.0f};

    HUD playerHUD = {
        .speed = 0.0f,
        .arrowTexture = LoadTexture("./textures/arrow.png")};

    ShipSettings shipSettings = {
        .thrust = 1.0f,
        .fuel = 100.0f,
        .fuelConsumption = 0.0f};

    int numBodies;
    CelestialBody **bodies = initBodies(&numBodies);
    float theta = 0.5f; // Barnes-Hut threshold
    int trailIndex = 0;

    CelestialBody *playerShip = bodies[5];

    int cameraLock = 0;
    Vector2 *cameraLockPosition = &bodies[cameraLock]->position;
    camera.target = *cameraLockPosition;
    camera.offset = (Vector2){*wMid, *hMid}; // Offset from camera target

    while (!WindowShouldClose())
    {
        camera.target = *cameraLockPosition;
        float dt = GetFrameTime();

        // Handle input
        if (IsKeyDown(KEY_E))
            incrementWarp(&timeScale, dt);
        if (IsKeyDown(KEY_Q))
            decrementWarp(&timeScale, dt);

        if (IsKeyPressed(KEY_L))
        {
            cameraLock++;
            cameraLock = cameraLock % numBodies;
            cameraLockPosition = &bodies[cameraLock]->position;
        }

        /*
            TODO:
                - [ ] Take player input for the ship (thrust, rotation)
                - [ ] Apply forces to the player's ship
        */

        camera.zoom += (float)GetMouseWheelMove() * 0.001f;
        camera.zoom = _Clamp(camera.zoom, cameraSettings.minZoom, cameraSettings.maxZoom);

        // Build QuadTree
        QuadTreeNode *root = buildQuadTree(bodies, numBodies);

        // Update physics
        float scaledDt = dt * timeScale.val;

        for (int i = 0; i < numBodies; i++)
        {
            if (bodies[i]->type == TYPE_UNIVERSE)
                continue;
            Vector2 force = computeForce(root, bodies[i], theta);
            Vector2 accel = Vector2Scale(force, 1.0f / bodies[i]->mass);
            bodies[i]->velocity = Vector2Add(bodies[i]->velocity, Vector2Scale(accel, scaledDt));
            bodies[i]->position = Vector2Add(bodies[i]->position, Vector2Scale(bodies[i]->velocity, scaledDt));
            detectCollisions(bodies, numBodies, root, bodies[i]);
            bodies[i]->previousPositions[trailIndex] = bodies[i]->position;
        }
        trailIndex += 1;
        trailIndex = trailIndex % PREVIOUS_POSITIONS;

        // Render
        BeginDrawing();
        BeginMode2D(camera);
        ClearBackground(BLACK);

        drawPreviousPositions(bodies, numBodies);
        // drawQuadtree(root);
        drawBodies(bodies, numBodies);

        EndMode2D();

        // GUI
        // Left
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawText("Press 'L' to switch camera", 10, 40, 20, DARKGRAY);
        DrawText("Press 'Q' and 'E' to time warp", 10, 70, 20, DARKGRAY);
        DrawText("Scroll to zoom", 10, 100, 20, DARKGRAY);

        // Centre
        drawPlayerHUD(&playerHUD, &playerShip->rotation, screenWidth, screenHeight);

        // Right
        DrawFPS(*screenWidth - 100, 10);
        DrawText(TextFormat("Camera locked to: body %i", cameraLock), *screenWidth - 280, 40, 20, DARKGRAY);
        DrawText(TextFormat("Time Scale: %.1fx", timeScale.val), *screenWidth - 200, 70, 20, DARKGRAY);
        DrawText(TextFormat("Zoom Level: %.2fx", calculateNormalisedZoom(&cameraSettings, camera.zoom)), *screenWidth - 200, 100, 20, DARKGRAY);

        EndDrawing();

        freeQuadTree(root);
    }

    // Cleanup
    for (int i = 0; i < numBodies; i++)
    {
        free(bodies[i]->name);
        free(bodies[i]);
    }
    free(bodies);
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

    // levelSpace(&screenWidth, &screenHeight, &wMid, &hMid, &targetFPS);
    levelUniverse(&screenWidth, &screenHeight, &wMid, &hMid, &targetFPS);

    CloseWindow();
    return 0;
}