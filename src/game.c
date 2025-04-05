#include "game.h"

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

void freeGameTextures(GameTextures gameTextures)
{
    if (
        gameTextures.numStarTextures > 0 || gameTextures.numPlanetTextures > 0 || gameTextures.numMoonTextures > 0 || gameTextures.numShipTextures > 0)
    {
        for (int i = 0; i < gameTextures.numStarTextures; i++)
        {
            UnloadTexture(*gameTextures.starTextures[i]);
            free(gameTextures.starTextures[i]);
        }
        free(gameTextures.starTextures);

        for (int i = 0; i < gameTextures.numPlanetTextures; i++)
        {
            UnloadTexture(*gameTextures.planetTextures[i]);
            free(gameTextures.planetTextures[i]);
        }
        free(gameTextures.planetTextures);

        for (int i = 0; i < gameTextures.numMoonTextures; i++)
        {
            UnloadTexture(*gameTextures.moonTextures[i]);
            free(gameTextures.moonTextures[i]);
        }
        free(gameTextures.moonTextures);

        for (int i = 0; i < gameTextures.numShipTextures; i++)
        {
            UnloadTexture(*gameTextures.shipTextures[i]);
            free(gameTextures.shipTextures[i]);
        }
        free(gameTextures.shipTextures);
    }
}

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