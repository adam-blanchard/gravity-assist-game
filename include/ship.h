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
    THROTTLE_UP,
    THROTTLE_DOWN
} ShipThrottle;

typedef enum
{
    ROTATION_RIGHT,
    ROTATION_LEFT
} ShipRotation;

typedef enum
{
    THRUSTER_RIGHT,
    THRUSTER_LEFT,
    THRUSTER_UP,
    THRUSTER_DOWN
} ShipThruster;

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
    Texture2D idleTexture;
    Texture2D thrustTexture;
    Texture2D *currentTexture;
    ShipState state;
    celestialbody_t *landedBody;
    Vector2 landingPosition;
    bool drawTrajectory;
    Vector2 *futurePositions;
} ship_t;

ship_t **initShips(int *numShips);
void freeShips(ship_t **ships, int numShips);
void takeoffShip(ship_t *ship);
void handleThrottle(ship_t **ships, int numShips, float dt, ShipThrottle throttleCommand);
void handleThruster(ship_t **ships, int numShips, float dt, ShipThruster thrusterCommand);
void handleRotation(ship_t **ships, int numShips, float dt, ShipRotation direction);
void toggleDrawTrajectory(ship_t **ships, int numShips);

#endif