#ifndef BODY_H
#define BODY_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "physics.h"
#include "ship.h"

typedef enum
{
    TYPE_STAR,
    TYPE_PLANET,
    TYPE_MOON,
    TYPE_SHIP,
    TYPE_SPACESTATION
} CelestialType;

typedef struct CelestialBody
{
    CelestialType type;
    char *name;
    Vector2 position;
    float mass; // Kg
    float radius;
    float rotation;
    Texture2D *texture;
    float textureScale;
    struct CelestialBody *parentBody;
    float orbitalRadius; // Distance from center for orbits (0 for black hole/ship)
    float angularSpeed;  // Radians per second (0 for black hole/ship)
    float initialAngle;  // Starting angle for orbit
    float atmosphereRadius;
    float atmosphereDrag;
    Color atmosphereColour;
} CelestialBody;

float getBodyAngle(struct CelestialBody *body, float gameTime);

Vector2 calculateBodyVelocity(struct CelestialBody *body, float gameTime);

CelestialBody **initBodies(int *numBodies);

void freeCelestialBodies(struct CelestialBody **bodies, int numBodies);

#endif