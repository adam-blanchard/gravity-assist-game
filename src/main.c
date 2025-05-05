#include <math.h>
#include <stdio.h>
#include "raylib.h"
#include "config.h"
#include "physics.h"
#include "body.h"
#include "ship.h"
#include "game.h"
#include "rendering.h"
#include "ui.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

    ScreenState screenState = GAME_HOME;
    gamestate_t gameState = {0};

    ColourScheme colourSchemes[COLOUR_COUNT];
    colourSchemes[0] = (ColourScheme){
        .colourMode = COLOUR_LIGHT,
        .spaceColour = (Color){255, 255, 255, 255},
        .gridColour = (Color){10, 10, 10, 50},
        .orbitColour = (Color){10, 10, 10, 100}};

    colourSchemes[1] = (ColourScheme){
        .colourMode = COLOUR_DARK,
        .spaceColour = (Color){10, 10, 10, 255},
        .gridColour = (Color){255, 255, 255, 50},
        .orbitColour = (Color){255, 255, 255, 100}};

    ColourScheme *currentColourScheme = &colourSchemes[COLOUR_DARK];

    CameraSettings cameraSettings = {
        .defaultZoom = 1e-2f,
        .minZoom = 1e-6f,
        .maxZoom = 2.0f};

    Camera2D camera = {0};
    camera.rotation = 0.0f;
    camera.zoom = cameraSettings.defaultZoom;

    WarpController timeScale = {
        .val = 1.0f,
        .increment = 1.5f,
        .min = 1.0f,
        .max = 64.0f};

    HUD playerHUD = {
        .speed = 0.0f,
        .compassTexture = LoadTexture("./textures/hud/compass.png"),
        .arrowTexture = LoadTexture("./textures/hud/arrow_2.png")};

    Texture2D shipLogo = LoadTexture("./textures/icons/logo_ship.png");

    gameState.gameTime = 0.0f;

    // Resource globalResources[RESOURCE_COUNT] = {
    //     {.type = RESOURCE_WATER_ICE, .name = "Water Ice", .weight = 1.0f, .value = 10},
    //     {.type = RESOURCE_COPPER_ORE, .name = "Copper Ore", .weight = 2.0f, .value = 20},
    //     {.type = RESOURCE_IRON_ORE, .name = "Iron Ore", .weight = 2.5f, .value = 25},
    //     {.type = RESOURCE_GOLD_ORE, .name = "Gold Ore", .weight = 3.0f, .value = 100}};

    gameState.numBodies = 0;
    gameState.bodies = NULL;

    gameState.numShips = 0;
    gameState.ships = NULL;

    // int numBodies = 0;
    // celestialbody_t **bodies = NULL;

    // int numShips = 0;
    // ship_t **ships = NULL;

    int cameraLock = 0;
    Vector2 *cameraLockPosition = NULL;
    camera.target = (Vector2){0, 0};
    camera.offset = (Vector2){wMid, hMid}; // Offset from camera target

    int velocityLock = 0;
    celestialbody_t *velocityTarget = NULL;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        switch (screenState)
        {
        case GAME_HOME:
            if (IsKeyPressed(KEY_ENTER))
            {
                initNewGame(&gameState, &screenState);
                velocityTarget = gameState.bodies[0];
            }
            if (IsKeyPressed(KEY_Q))
            {
                CloseWindow();
                return 0; // Quit immediately
            }
            break;

        case GAME_RUNNING:
            if (IsKeyDown(KEY_PERIOD))
                incrementWarp(&timeScale, dt);
            if (IsKeyDown(KEY_COMMA))
                decrementWarp(&timeScale, dt);

            float scaledDt = dt * timeScale.val;

            gameState.gameTime += scaledDt;

            if (IsKeyPressed(KEY_ESCAPE))
            {
                screenState = GAME_PAUSED;
            }

            if (IsKeyPressed(KEY_C))
            {
                gameState.ships[cameraLock]->isSelected = false;
                cameraLock++;
                cameraLock = cameraLock % gameState.numShips;
                cameraLockPosition = &gameState.ships[cameraLock]->position;
                gameState.ships[cameraLock]->isSelected = true;
            }

            if (IsKeyDown(KEY_LEFT_SHIFT))
            {
                handleThrottle(gameState.ships, gameState.numShips, scaledDt, THROTTLE_UP);
            }
            if (IsKeyDown(KEY_LEFT_CONTROL))
            {
                handleThrottle(gameState.ships, gameState.numShips, scaledDt, THROTTLE_DOWN);
            }

            // Sets engine texture and resets thruster flags before input
            updateShipTextureFlags(gameState.ships, gameState.numShips);

            if (IsKeyDown(KEY_D))
            {
                handleRotation(gameState.ships, gameState.numShips, scaledDt, ROTATION_RIGHT);
            }
            if (IsKeyDown(KEY_A))
            {
                handleRotation(gameState.ships, gameState.numShips, scaledDt, ROTATION_LEFT);
            }
            if (IsKeyDown(KEY_E))
            {
                handleThruster(gameState.ships, gameState.numShips, scaledDt, THRUSTER_RIGHT);
            }
            if (IsKeyDown(KEY_Q))
            {
                handleThruster(gameState.ships, gameState.numShips, scaledDt, THRUSTER_LEFT);
            }
            if (IsKeyDown(KEY_W))
            {
                handleThruster(gameState.ships, gameState.numShips, scaledDt, THRUSTER_UP);
            }
            if (IsKeyDown(KEY_S))
            {
                handleThruster(gameState.ships, gameState.numShips, scaledDt, THRUSTER_DOWN);
            }
            if (IsKeyDown(KEY_X))
            {
                cutEngines(gameState.ships, gameState.numShips);
            }

            if (IsKeyPressed(KEY_T))
            {
                toggleDrawTrajectory(gameState.ships, gameState.numShips);
            }

            if (IsKeyPressed(KEY_V))
            {
                velocityLock++;
                velocityLock = velocityLock % gameState.numBodies;
                velocityTarget = gameState.bodies[velocityLock];
            }

            cameraLockPosition = &gameState.ships[cameraLock]->position;
            camera.target = *cameraLockPosition;

            camera.zoom += (float)GetMouseWheelMove() * (1e-5f + camera.zoom * (camera.zoom / 4.0f));
            camera.zoom = Clamp(camera.zoom, cameraSettings.minZoom, cameraSettings.maxZoom);

            updateCelestialPositions(gameState.bodies, gameState.numBodies, gameState.gameTime);
            updateShipPositions(gameState.ships, gameState.numShips, gameState.bodies, gameState.numBodies, scaledDt);

            updateLandedShipPosition(gameState.ships, gameState.numShips, gameState.gameTime);

            detectCollisions(gameState.ships, gameState.numShips, gameState.bodies, gameState.numBodies, gameState.gameTime);
            calculateShipFuturePositions(gameState.ships, gameState.numShips, gameState.bodies, gameState.numBodies, gameState.gameTime);

            // Vector2 earthVelocity = calculateBodyVelocity(bodies[1], gameTime);
            // printf("Earth velocity\nx: %.2f\ny: %.2f\n", earthVelocity.x, earthVelocity.y);

            // if (IsKeyPressed(KEY_M) && playerShip->shipSettings.landedBody != NULL)
            // {
            //     mineResource(playerShip->shipSettings.landedBody, playerShip, globalResources, RESOURCE_GOLD_ORE, 1);
            // }

            playerHUD.speed = calculateRelativeSpeed(gameState.ships[0], velocityTarget, gameState.gameTime);
            playerHUD.playerRotation = gameState.ships[0]->rotation;
            playerHUD.velocityTarget = velocityTarget;

            break;

        case GAME_PAUSED:
            if (IsKeyPressed(KEY_ESCAPE))
            {
                screenState = GAME_RUNNING;
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
        ClearBackground(currentColourScheme->spaceColour);

        if (screenState == GAME_HOME)
        {
            DrawText("Gravity Assist", GetScreenWidth() / 2 - MeasureText("Gravity Assist", 40) / 2, 200, 40, WHITE);
            if (GuiButton((Rectangle){ GetScreenWidth() / 2 - 60, 280, 120, 40 }, "PLAY (ENTER)")) {
                // screenState = GAME_RUNNING;
                // This results in a segfault...
                printf("Player selected 'PLAY'\n");
            }

            if (GuiButton((Rectangle){ GetScreenWidth() / 2 - 60, 340, 120, 40 }, "QUIT (Q)")) {
                printf("Player selected 'QUIT'\n");
            }
        }
        else
        {
            BeginMode2D(camera);
            drawCelestialGrid(gameState.bodies, gameState.numBodies, camera, currentColourScheme);
            drawOrbits(gameState.bodies, gameState.numBodies, currentColourScheme);
            drawTrajectories(gameState.ships, gameState.numShips, currentColourScheme);
            drawBodies(gameState.bodies, gameState.numBodies);
            drawShips(gameState.ships, gameState.numShips, &camera, &shipLogo);

            EndMode2D();

            // GUI
            DrawText("Press ESC to pause & view controls", 10, 10, 20, DARKGRAY);

            drawPlayerHUD(&playerHUD);
            // drawPlayerStats(&playerStats);
            // drawPlayerInventory(playerShip, globalResources);

            DrawFPS(screenWidth - 100, 10);
            DrawText(TextFormat("Camera locked to Ship: %i", cameraLock), screenWidth - 280, 40, 20, DARKGRAY);
            DrawText(TextFormat("Time Scale: %.1fx", timeScale.val), screenWidth - 200, 70, 20, DARKGRAY);
            // DrawText(TextFormat("Zoom Level: %.3fx", calculateNormalisedZoom(&cameraSettings, camera.zoom)), screenWidth - 200, 100, 20, DARKGRAY);
            DrawText(TextFormat("Camera zoom: %.6fx", camera.zoom), screenWidth - 250, 100, 20, DARKGRAY);

            DrawText(TextFormat("Ship throttle: %.2fpct", gameState.ships[0]->throttle), screenWidth - 250, 130, 20, DARKGRAY);

            if (screenState == GAME_PAUSED)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GRAY, 0.5f));
                DrawText("Game Paused", GetScreenWidth() / 2 - MeasureText("Game Paused", 40) / 2, 200, 40, WHITE);
                DrawText("Press ESC to Resume", GetScreenWidth() / 2 - MeasureText("Press ESC to Resume", 20) / 2, 300, 20, WHITE);
                DrawText("Press Q to Quit", GetScreenWidth() / 2 - MeasureText("Press Q to Quit", 20) / 2, 340, 20, WHITE);
                DrawText("Press 'C' to switch camera", 10, 40, 20, WHITE);
                DrawText("Press '.' and ',' to time warp", 10, 70, 20, WHITE);
                DrawText("Scroll to zoom", 10, 100, 20, WHITE);
                DrawText("Press 'V' to switch velocity lock", 10, 130, 20, WHITE);
            }
        }

        EndDrawing();
    }

    freeCelestialBodies(gameState.bodies, gameState.numBodies);
    freeShips(gameState.ships, gameState.numShips);
    UnloadTexture(playerHUD.compassTexture);
    UnloadTexture(playerHUD.arrowTexture);
    UnloadTexture(shipLogo);
    // freeGameTextures(gameTextures);

    CloseWindow();
    return 0;
}

// void DrawGame(void) {
//     BeginDrawing();

//     ClearBackground(SPACE_COLOUR);

// }