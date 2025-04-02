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

typedef struct Ship
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float rotation;
    float rotationSpeed; // Degrees per second
    float radius;
    float thrust;
    float fuel;
    float fuelConsumption;
    bool isSelected;
    Texture2D *thrustTexture;
    ShipState state;
    celestialbody_t *landedBody;
    Vector2 landingPosition;
    Vector2 *futurePositions;
} ship_t;

ship_t **initShips(int *numShips);
void freeShips(ship_t **ships, int numShips);

#endif