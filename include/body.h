#ifndef BODY_H
#define BODY_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "utils.h"

typedef struct GameState gamestate_t;

typedef enum
{
    TYPE_STAR,
    TYPE_PLANET,
    TYPE_MOON,
    TYPE_SHIP,
    TYPE_SPACESTATION
} CelestialType;

typedef struct CelestialBody celestialbody_t;

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
    celestialbody_t *parentBody;
    float orbitalRadius; // Distance from center for orbits (0 for black hole/ship)
    float angularSpeed;  // Radians per second (0 for black hole/ship)
    float initialAngle;  // Starting angle for orbit
    float atmosphereRadius;
    float atmosphereDrag;
    Color atmosphereColour;
} celestialbody_t;

float getBodyAngle(celestialbody_t *body, float gameTime);
celestialbody_t **initBodies(int *numBodies);
bool saveBody(celestialbody_t *body, FILE *file, gamestate_t *state);
int getBodyIndex(celestialbody_t *body, celestialbody_t **bodies, int numBodies);
celestialbody_t* getBodyPtr(int index, celestialbody_t **bodies, int numBodies);
void freeCelestialBodies(celestialbody_t **bodies, int numBodies);

#endif