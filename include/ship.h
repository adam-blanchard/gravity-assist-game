#ifndef SHIP_H
#define SHIP_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "body.h"

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
    Texture2D baseTexture;
    bool mainEnginesOn;
    Texture2D engineTexture;
    bool thrusterUp;
    Texture2D thrusterUpTexture;
    bool thrusterDown;
    Texture2D thrusterDownTexture;
    bool thrusterRight;
    Texture2D thrusterRightTexture;
    bool thrusterLeft;
    Texture2D thrusterLeftTexture;
    bool thrusterRotateRight;
    Texture2D thrusterRotateRightTexture;
    bool thrusterRotateLeft;
    Texture2D thrusterRotateLeftTexture;
    ShipState state;
    ShipType type;
    celestialbody_t *landedBody;
    Vector2 landingPosition;
    bool drawTrajectory;
    int trajectorySize;
    Vector2 *futurePositions;
} ship_t;

ship_t **initShips(int *numShips);
void freeShips(ship_t **ships, int numShips);
void takeoffShip(ship_t *ship);
void handleThrottle(ship_t **ships, int numShips, float dt, ShipThrottle throttleCommand);
void handleThruster(ship_t **ships, int numShips, float dt, ShipMovement thrusterCommand);
void handleRotation(ship_t **ships, int numShips, float dt, ShipMovement direction);
void toggleDrawTrajectory(ship_t **ships, int numShips);
void updateShipTextureFlags(ship_t **ships, int numShips);

#endif