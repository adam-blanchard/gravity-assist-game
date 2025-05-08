#ifndef SHIP_H
#define SHIP_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "body.h"
#include "textures.h"

typedef struct GameState gamestate_t;

typedef enum
{
    SHIP_FLYING,
    SHIP_LANDED
} ShipState;

typedef enum
{
    SHIP_ROCKET,
    SHIP_BOOSTER,
    SHIP_STATION
} ShipType;

typedef enum
{
    THROTTLE_UP,
    THROTTLE_DOWN
} ShipThrottle;

typedef enum
{
    THRUSTER_RIGHT,
    THRUSTER_LEFT,
    THRUSTER_UP,
    THRUSTER_DOWN,
    ROTATION_RIGHT,
    ROTATION_LEFT
} ShipMovement;

typedef struct Ship
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float rotation;
    float rotationSpeed; // Degrees per second
    float radius;
    float thrust;
    float throttle;
    float thrusterForce;
    float fuel;
    float fuelConsumption;
    bool isSelected;
    ShipState state;
    ShipType type;
    celestialbody_t *landedBody;
    Vector2 landingPosition;
    bool drawTrajectory;
    int trajectorySize;
    Vector2 *futurePositions;
    bool mainEnginesOn;
    bool thrusterUp;
    bool thrusterDown;
    bool thrusterRight;
    bool thrusterLeft;
    bool thrusterRotateRight;
    bool thrusterRotateLeft;
    int baseTextureId;
    int engineTextureId;
    int thrusterUpTextureId;
    int thrusterDownTextureId;
    int thrusterRightTextureId;
    int thrusterLeftTextureId;
    int thrusterRotateRightTextureId;
    int thrusterRotateLeftTextureId;
    float textureScale;
    Texture2D baseTexture;
    Texture2D engineTexture;
    Texture2D thrusterUpTexture;
    Texture2D thrusterDownTexture;
    Texture2D thrusterRightTexture;
    Texture2D thrusterLeftTexture;
    Texture2D thrusterRotateRightTexture;
    Texture2D thrusterRotateLeftTexture;
} ship_t;

ship_t **initShips(int *numShips);
bool saveShip(ship_t *ship, FILE *file, gamestate_t *state);
void freeShip(ship_t* ship);
void freeShips(ship_t **ships, int numShips);
void takeoffShip(ship_t *ship);
void handleThrottle(ship_t **ships, int numShips, float dt, ShipThrottle throttleCommand);
void handleThruster(ship_t **ships, int numShips, float dt, ShipMovement thrusterCommand);
void handleRotation(ship_t **ships, int numShips, float dt, ShipMovement direction);
void cutEngines(ship_t **ships, int numShips);
void toggleDrawTrajectory(ship_t **ships, int numShips);
void updateShipTextureFlags(ship_t **ships, int numShips);

#endif