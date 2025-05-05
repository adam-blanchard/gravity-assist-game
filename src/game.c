#include "game.h"

void saveGame(char* filename, gamestate_t* state) {
    // Takes the current state of the game and saves to a local binary
    FILE* file = fopen(filename, "wb"); // Open file in binary write mode
    if (file == NULL) {
        TraceLog(LOG_ERROR, "Failed to open file for saving: %s", filename);
        return;
    }

    // Write the entire gamestate_t struct to the file
    fwrite(state, sizeof(gamestate_t), 1, file);

    fclose(file);
    TraceLog(LOG_INFO, "Game state saved to %s", filename);
}

bool loadGame(char* filename, gamestate_t* state) {
    FILE* file = fopen(filename, "rb"); // Open file in binary read mode
    if (file == NULL) {
        TraceLog(LOG_ERROR, "Failed to open file for loading: %s", filename);
        return false;
    }

    // Read the gamestate_t struct from the file
    size_t read = fread(state, sizeof(gamestate_t), 1, file);
    fclose(file);

    if (read != 1) {
        TraceLog(LOG_ERROR, "Failed to read game state from %s", filename);
        return false;
    }

    TraceLog(LOG_INFO, "Game state loaded from %s", filename);
    return true;
}

void incrementWarp(WarpController *timeScale, float dt)
{
    timeScale->val += timeScale->increment * timeScale->val * dt;
    timeScale->val = Clamp(timeScale->val, timeScale->min, timeScale->max);
}

void decrementWarp(WarpController *timeScale, float dt)
{
    timeScale->val -= timeScale->increment * timeScale->val * dt;
    timeScale->val = Clamp(timeScale->val, timeScale->min, timeScale->max);
}

float calculateNormalisedZoom(CameraSettings *settings, float currentZoom)
{
    float midpoint = settings->minZoom + ((settings->maxZoom - settings->minZoom) / 2);
    return (float){currentZoom / midpoint};
}

// bool mineResource(CelestialBody *body, CelestialBody *playerShip, Resource *resourceDefinitions, ResourceType type, int amount)
// {
//     // Check if planet has enough of the resource
//     if (body->resources[type].quantity < amount)
//     {
//         return false; // Not enough resources
//     }

//     // Calculate weight of the requested amount
//     float weight_to_add = resourceDefinitions[type].weight * amount;
//     if (playerShip->shipSettings.currentCapacity + weight_to_add > playerShip->shipSettings.maxCapacity)
//     {
//         return false; // Ship can't carry more
//     }

//     // Update planet and ship
//     body->resources[type].quantity -= amount;
//     playerShip->shipSettings.inventory[type].quantity += amount;
//     playerShip->shipSettings.currentCapacity += weight_to_add;

//     return true;
// }

void spawnShipOnBody(ship_t *ship, celestialbody_t *body, float gameTime)
{
    Vector2 relPos = (Vector2){0, -body->radius - ship->radius - 1};

    ship->position = Vector2Add(body->position, relPos);

    // landShip(ship, body, gameTime);
}

void initStartPositions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float gameTime)
{
    landShip(ships[0], bodies[0], gameTime);
    initStableOrbit(ships[1], bodies[0], gameTime);
}