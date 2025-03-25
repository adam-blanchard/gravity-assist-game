#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "body.h"

typedef struct
{
    float speed;
    float playerRotation;
    CelestialBody *velocityTarget;
    Texture2D compassTexture;
    Texture2D arrowTexture;
} HUD;

#endif