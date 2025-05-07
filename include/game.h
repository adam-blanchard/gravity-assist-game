#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include "physics.h"

// Forward declarations
typedef struct CelestialBody celestialbody_t;
typedef struct Ship ship_t;

typedef enum
{
    COLOUR_LIGHT,
    COLOUR_DARK,
    COLOUR_COUNT
} ColourMode;

typedef enum
{
    GAME_HOME,
    GAME_RUNNING,
    GAME_PAUSED
} ScreenState;

typedef struct
{
    float val;
    float increment;
    float min;
    float max;
} WarpController;

typedef struct
{
    float defaultZoom;
    float minZoom;
    float maxZoom;
} CameraSettings;

typedef struct PlayerStats
{
    int money;
    uint32_t miningXP;
} PlayerStats;

typedef struct ColourScheme
{
    ColourMode colourMode;
    Color spaceColour;
    Color gridColour;
    Color orbitColour;
} ColourScheme;

typedef struct GameState {
    float gameTime;
    int numBodies;
    celestialbody_t **bodies;
    int numShips;
    ship_t **ships;
} gamestate_t;

// typedef enum
// {
//     RESOURCE_WATER_ICE = 0,
//     RESOURCE_COPPER_ORE,
//     RESOURCE_IRON_ORE,
//     RESOURCE_GOLD_ORE,
//     RESOURCE_COUNT // This tracks the number of resources and simplifies future additions
// } ResourceType;

// typedef struct
// {
//     ResourceType type;
//     char name[32]; // Name of the resource (e.g., "Water Ice")
//     float weight;  // Weight per unit (e.g., for ship capacity limits)
//     int value;     // Value for trading or crafting
// } Resource;

// typedef struct InventoryResource
// {
//     int resourceId; // Corresponds to ResourceType
//     int quantity;   // Amount available on the CelestialBody
// } InventoryResource;

void saveGame(char* filename, gamestate_t* state);
bool loadGame(char* filename, gamestate_t* state);
void initNewGame(gamestate_t* gameState);
void incrementWarp(WarpController *timeScale, float dt);
void decrementWarp(WarpController *timeScale, float dt);
float calculateNormalisedZoom(CameraSettings *settings, float currentZoom);
void spawnShipOnBody(ship_t *ship, celestialbody_t *body, float gameTime);
void initStartPositions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float gameTime);

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

#endif