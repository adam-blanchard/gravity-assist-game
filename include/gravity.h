#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// G is usually 6.67430e-11

#ifndef PI
#define PI 3.14159265358979323846f
#endif
#ifndef G
#define G 6.67430e-5
#endif
#define TRAJECTORY_STEPS 6000
#define TRAJECTORY_STEP_TIME 0.033f
#define PREVIOUS_POSITIONS 1000
#define FUTURE_POSITIONS 1000
#define FUTURE_STEP_TIME 0.1f
#define TEXTURE_SCALE 2.8
#define GRID_SPACING 1e2
#define GRID_LINE_WIDTH 100
#define HUD_ARROW_SCALE 0.01
#define MAX_LANDING_SPEED 50

#define TRAIL_COLOUR \
    CLITERAL(Color) { 255, 255, 255, 255 }

#define GRID_COLOUR \
    CLITERAL(Color) { 255, 255, 255, 50 }

#define SPACE_COLOUR \
    CLITERAL(Color) { 10, 10, 10, 255 }

#define GUI_BACKGROUND \
    CLITERAL(Color) { 255, 255, 255, 200 }

typedef enum
{
    GAME_HOME,
    GAME_RUNNING,
    GAME_PAUSED
} GameState;

typedef enum
{
    SHIP_FLYING,
    SHIP_LANDED
} ShipState;

typedef enum
{
    TYPE_STAR,
    TYPE_PLANET,
    TYPE_MOON,
    TYPE_SHIP,
    TYPE_SPACESTATION
} CelestialType;

typedef enum
{
    MINERAL_IRONORE,
    MINERAL_COPPERORE,
    MINERAL_GOLDORE,
    MINERAL_WATERICE
} Mineral;

typedef struct CelestialBody CelestialBody;
typedef struct Resource Resource;

typedef struct ShipSettings
{
    float thrust;
    float fuel;
    float fuelConsumption;
    bool isSelected;
    Texture2D *thrustTexture;
    ShipState state;
    CelestialBody *landedBody;
    Vector2 landingPosition;
} ShipSettings;

typedef struct CelestialBody
{
    CelestialType type;
    char *name;
    Vector2 position;
    Vector2 velocity;
    float mass; // Kg
    float radius;
    float colliderRadius;
    Vector2 *previousPositions;
    Vector2 *futurePositions;
    float rotation;
    ShipSettings shipSettings;
    Texture2D *texture;
    Resource *resources;
} CelestialBody;

typedef struct GameTextures
{
    int numStarTextures;
    Texture2D **starTextures;
    int numPlanetTextures;
    Texture2D **planetTextures;
    int numMoonTextures;
    Texture2D **moonTextures;
    int numShipTextures;
    Texture2D **shipTextures;
} GameTextures;

typedef struct QuadTreeNode
{
    Rectangle bounds;                 // 2D region (x, y, width, height)
    Vector2 centerOfMass;             // Center of mass of all bodies in this node
    float totalMass;                  // Total mass of bodies in this node
    struct QuadTreeNode *children[4]; // NW, NE, SW, SE quadrants
    CelestialBody *body;              // Pointer to body if leaf node; NULL otherwise
} QuadTreeNode;

typedef struct
{
    float val;
    float increment;
    float min;
    float max;
} WarpController;

typedef struct
{
    float speed;
    float playerRotation;
    CelestialBody *velocityTarget;
    Texture2D compassTexture;
    Texture2D arrowTexture;
} HUD;

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

typedef struct PlayerInventory
{
    int size;
    int iron;
    int copper;
    int gold;
    int ice;
} PlayerInventory;

typedef struct Resource
{
    int amount;
    Mineral mineral;
} Resource;

float calculateOrbitalVelocity(float mass, float radius)
{
    float orbitalVelocity = sqrtf((G * mass) / radius);
    return (float){orbitalVelocity};
}

float calculateOrbitCircumference(float r)
{
    return (float){2 * PI * r};
}

float calculateEscapeVelocity(float mass, float radius)
{
    float escapeVelocity = sqrtf((2 * G * mass) / radius);
    return (float){escapeVelocity};
}

float calculateDistance(Vector2 *pos1, Vector2 *pos2)
{
    float xDist = pos1->x - pos2->x;
    float yDist = pos1->y - pos2->y;
    float radius = sqrt(xDist * xDist + yDist * yDist);
    return (float)(radius);
}

float calculateOrbitalRadius(float period, float mStar)
{
    float r3 = (period * period * G * mStar) / (4 * PI * PI);
    return (float){cbrtf(r3)};
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

float calculateRelativeSpeed(CelestialBody *body1, CelestialBody *body2)
{
    Vector2 relativeVelocity = Vector2Subtract(body1->velocity, body2->velocity);
    return sqrtf((relativeVelocity.x * relativeVelocity.x) + (relativeVelocity.y * relativeVelocity.y));
}

float calculateAbsoluteSpeed(CelestialBody *body)
{
    return sqrtf((body->velocity.x * body->velocity.x) + (body->velocity.y * body->velocity.y));
}

float calculateOrbitalSpeed(float mass, float radius)
{
    return (float){sqrtf((G * mass) / radius)};
}

void landShip(CelestialBody *ship, CelestialBody *body)
{
    if (ship->type != TYPE_SHIP || ship->shipSettings.state == SHIP_LANDED)
        return;

    // Set landing state
    ship->shipSettings.state = SHIP_LANDED;
    ship->shipSettings.landedBody = body;
    ship->velocity = body->velocity; // Match velocity to the body

    Vector2 direction = Vector2Subtract(ship->position, body->position);
    float distance = Vector2Length(direction);
    direction = Vector2Normalize(direction);

    // Position ship on the surface and store landing position
    Vector2 surfacePosition = Vector2Scale(direction, body->radius + ship->radius);
    ship->position = Vector2Add(body->position, surfacePosition);
    ship->shipSettings.landingPosition = surfacePosition; // Store relative position
}

void takeoffShip(CelestialBody *ship)
{
    if (ship->type != TYPE_SHIP || ship->shipSettings.state != SHIP_LANDED || ship->shipSettings.landedBody == NULL)
        return;

    // Set flying state
    ship->shipSettings.state = SHIP_FLYING;

    // Calculate takeoff direction (normal to surface)
    Vector2 direction = Vector2Normalize(ship->shipSettings.landingPosition);
    float takeoffSpeed = calculateEscapeVelocity(ship->shipSettings.landedBody->mass, ship->shipSettings.landedBody->radius) * 0.8f; // Slightly less than escape velocity
    Vector2 takeoffVelocity = Vector2Scale(direction, takeoffSpeed);

    // Set velocity relative to the body's velocity
    ship->velocity = Vector2Add(ship->shipSettings.landedBody->velocity, takeoffVelocity);

    // Clear landed body reference
    ship->shipSettings.landedBody = NULL;
}

QuadTreeNode *createNode(Rectangle bounds)
{
    QuadTreeNode *node = (QuadTreeNode *)malloc(sizeof(QuadTreeNode));
    node->bounds = bounds;
    node->centerOfMass = (Vector2){0, 0};
    node->totalMass = 0.0f;
    for (int i = 0; i < 4; i++)
        node->children[i] = NULL;
    node->body = NULL;
    return node;
}

void subdivide(QuadTreeNode *node)
{
    float x = node->bounds.x;
    float y = node->bounds.y;
    float w = node->bounds.width / 2;
    float h = node->bounds.height / 2;
    node->children[0] = createNode((Rectangle){x, y, w, h});         // NW
    node->children[1] = createNode((Rectangle){x + w, y, w, h});     // NE
    node->children[2] = createNode((Rectangle){x, y + h, w, h});     // SW
    node->children[3] = createNode((Rectangle){x + w, y + h, w, h}); // SE
}

void insertBody(QuadTreeNode *node, CelestialBody *body)
{
    if (node->body != NULL)
    { // Leaf with a body
        CelestialBody *existingBody = node->body;
        subdivide(node);
        // Insert existing body into appropriate child
        int index = (existingBody->position.x < node->bounds.x + node->bounds.width / 2) ? (existingBody->position.y < node->bounds.y + node->bounds.height / 2 ? 0 : 2) : (existingBody->position.y < node->bounds.y + node->bounds.height / 2 ? 1 : 3);
        insertBody(node->children[index], existingBody);
        node->body = NULL;
    }
    if (node->children[0] == NULL)
    { // Leaf node
        node->body = body;
        node->totalMass = body->mass;
        node->centerOfMass = body->position;
    }
    else
    { // Internal node
        int index = (body->position.x < node->bounds.x + node->bounds.width / 2) ? (body->position.y < node->bounds.y + node->bounds.height / 2 ? 0 : 2) : (body->position.y < node->bounds.y + node->bounds.height / 2 ? 1 : 3);
        insertBody(node->children[index], body);
        // Update totalMass and centerOfMass
        node->totalMass = 0;
        node->centerOfMass = (Vector2){0, 0};
        for (int i = 0; i < 4; i++)
        {
            if (node->children[i])
            {
                node->totalMass += node->children[i]->totalMass;
                node->centerOfMass = Vector2Add(node->centerOfMass,
                                                Vector2Scale(node->children[i]->centerOfMass, node->children[i]->totalMass));
            }
        }
        if (node->totalMass > 0)
        {
            node->centerOfMass = Vector2Scale(node->centerOfMass, 1.0f / node->totalMass);
        }
    }
}

QuadTreeNode *buildQuadTree(CelestialBody **bodies, int numBodies)
{
    if (numBodies == 0)
    {
        printf("No bodies");
        return NULL;
    }

    float minX = bodies[0]->position.x, maxX = minX;
    float minY = bodies[0]->position.y, maxY = minY;

    for (int i = 1; i < numBodies; i++)
    {
        minX = fmin(minX, bodies[i]->position.x);
        maxX = fmax(maxX, bodies[i]->position.x);
        minY = fmin(minY, bodies[i]->position.y);
        maxY = fmax(maxY, bodies[i]->position.y);
    }
    float width = maxX - minX, height = maxY - minY;
    if (width > height)
        minY -= (width - height) / 2;
    else
        minX -= (height - width) / 2;
    Rectangle bounds = {minX, minY, fmax(width, height), fmax(width, height)};
    QuadTreeNode *root = createNode(bounds);
    for (int i = 0; i < numBodies; i++)
    {
        insertBody(root, bodies[i]);
    }
    return root;
}

Vector2 computeForce(QuadTreeNode *node, CelestialBody *body, float theta)
{
    if (node->totalMass == 0)
        return (Vector2){0, 0};
    if (node->body != NULL)
    {
        if (node->body == body)
            return (Vector2){0, 0}; // Skip self
        Vector2 dir = Vector2Subtract(node->body->position, body->position);
        float dist = Vector2Length(dir);
        if (dist < 1e-5)
            return (Vector2){0, 0}; // Avoid division by zero
        float mag = (G * body->mass * node->body->mass) / (dist * dist);
        return Vector2Scale(Vector2Normalize(dir), mag);
    }
    float dist = Vector2Distance(body->position, node->centerOfMass);
    if (dist < 1e-5)
        dist = 1e-5; // Prevent singularity
    float size = node->bounds.width;
    if (size / dist < theta)
    { // Approximate as point mass
        float mag = (G * body->mass * node->totalMass) / (dist * dist);
        Vector2 dir = Vector2Normalize(Vector2Subtract(node->centerOfMass, body->position));
        return Vector2Scale(dir, mag);
    }
    Vector2 force = {0, 0};
    for (int i = 0; i < 4; i++)
    {
        if (node->children[i])
        {
            force = Vector2Add(force, computeForce(node->children[i], body, theta));
        }
    }
    return force;
}

void freeQuadTree(QuadTreeNode *node)
{
    if (!node)
        return;
    for (int i = 0; i < 4; i++)
    {
        freeQuadTree(node->children[i]);
    }
    free(node);
}

void detectCollisions(CelestialBody **bodies, int numBodies, QuadTreeNode *node, CelestialBody *body)
{
    if (!node || node->totalMass == 0)
        return;
    if (node->body && node->body != body)
    {
        float dist = Vector2Distance(body->position, node->body->position);
        if (dist < (body->radius + node->body->radius))
        {
            // printf("Collision between %s and %s\n", body->name, node->body->name);
            if (body->type == TYPE_SHIP && body->shipSettings.state != SHIP_LANDED)
            {
                float relVel = calculateRelativeSpeed(body, node->body);
                if (relVel <= MAX_LANDING_SPEED)
                {
                    landShip(body, node->body);
                }
                else
                {
                    // Reset ship (example handling)
                    body->position = Vector2Add(bodies[2]->position, (Vector2){1e4, 0});
                    Vector2 relVel = (Vector2){0, calculateOrbitalSpeed(bodies[2]->mass, 1e4)};
                    body->velocity = Vector2Add(bodies[2]->velocity, relVel);
                }
            }
        }
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        if (node->children[i])
        {
            detectCollisions(bodies, numBodies, node->children[i], body);
        }
    }
}

CelestialBody **initBodies(int *numBodies, GameTextures *gameTextures)
{
    *numBodies = 8;
    CelestialBody **bodies = malloc(sizeof(CelestialBody *) * (*numBodies));

    // Ship
    // Ship texture is 64x64
    bodies[0] = malloc(sizeof(CelestialBody));
    *bodies[0] = (CelestialBody){.type = TYPE_SHIP,
                                 .name = strdup("Ship"),
                                 .position = {0, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e1f,
                                 .radius = 32.0f,
                                 .colliderRadius = 50.0f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 //  .futurePositions = (Vector2 *)malloc(sizeof(Vector2) * FUTURE_POSITIONS),
                                 .rotation = 0.0f,
                                 .shipSettings = (ShipSettings){
                                     .thrust = 4e0f,
                                     .fuel = 100.0f,
                                     .fuelConsumption = 0.0f,
                                     .isSelected = true,
                                     .thrustTexture = gameTextures->shipTextures[1],
                                     .state = SHIP_FLYING,
                                     .landedBody = NULL,
                                     .landingPosition = {0}},
                                 .texture = gameTextures->shipTextures[0]};

    // Star
    bodies[1] = malloc(sizeof(CelestialBody));
    *bodies[1] = (CelestialBody){.type = TYPE_STAR,
                                 .name = strdup("Sol"),
                                 .position = {100, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e15f,
                                 .radius = 3.2e5,
                                 .colliderRadius = 3.2e5,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->starTextures[0]};

    // Planet orbiting Star - Earth
    float orbitalRadius = calculateOrbitalRadius(24 * 60 * 60, bodies[1]->mass);
    bodies[2] = malloc(sizeof(CelestialBody));
    *bodies[2] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Earth"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9,
                                 .radius = 3.2e3f,
                                 .colliderRadius = 3.2e3f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->planetTextures[0],
                                 .resources = NULL};
    bodies[2]->velocity = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};

    // Determine Ship's initial velocity once orbiting body has been initialised
    bodies[0]->position = Vector2Add(bodies[2]->position, (Vector2){5e3, 0});
    Vector2 relVel = (Vector2){0, calculateOrbitalSpeed(bodies[2]->mass, 5e3)};
    bodies[0]->velocity = Vector2Add(bodies[2]->velocity, relVel);

    // Moon orbiting Planet
    orbitalRadius = calculateOrbitalRadius(12 * 60 * 60, bodies[2]->mass);
    bodies[3] = malloc(sizeof(CelestialBody));
    *bodies[3] = (CelestialBody){.type = TYPE_MOON,
                                 .name = strdup("Earth's Moon"),
                                 .position = {bodies[2]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e7,
                                 .radius = 3.2e2f,
                                 .colliderRadius = 3.2e2f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->moonTextures[0],
                                 .resources = (Resource *)malloc(sizeof(Resource) * 1)};
    relVel = (Vector2){0, -calculateOrbitalSpeed(bodies[2]->mass, orbitalRadius)};
    bodies[3]->velocity = Vector2Add(bodies[2]->velocity, relVel);
    bodies[3]->resources[0] = (Resource){.amount = 9999, .mineral = MINERAL_WATERICE};

    // Planet orbiting Star - Mercury
    orbitalRadius = calculateOrbitalRadius(0.39f * 24 * 60 * 60, bodies[1]->mass);
    bodies[4] = malloc(sizeof(CelestialBody));
    *bodies[4] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Mercury"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9 * 0.0553f,
                                 .radius = 3.2e3f * 0.383f,
                                 .colliderRadius = 3.2e3f * 0.383f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->planetTextures[1]};
    bodies[4]->velocity = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};

    // Planet orbiting Star - Venus
    orbitalRadius = calculateOrbitalRadius(0.723f * 24 * 60 * 60, bodies[1]->mass);
    bodies[5] = malloc(sizeof(CelestialBody));
    *bodies[5] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Venus"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9 * 0.815f,
                                 .radius = 3.2e3f * 0.949f,
                                 .colliderRadius = 3.2e3f * 0.949f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->planetTextures[2]};
    bodies[5]->velocity = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};

    // Planet orbiting Star - Mars
    orbitalRadius = calculateOrbitalRadius(1.52f * 24 * 60 * 60, bodies[1]->mass);
    bodies[6] = malloc(sizeof(CelestialBody));
    *bodies[6] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Mars"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9 * 0.107f,
                                 .radius = 3.2e3f * 0.532f,
                                 .colliderRadius = 3.2e3f * 0.532f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->planetTextures[3]};
    bodies[6]->velocity = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};

    // Planet orbiting Star - Jupiter
    orbitalRadius = calculateOrbitalRadius(5.20f * 24 * 60 * 60, bodies[1]->mass);
    bodies[7] = malloc(sizeof(CelestialBody));
    *bodies[7] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Mars"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9 * 317.8f,
                                 .radius = 3.2e3f * 11.21f,
                                 .colliderRadius = 3.2e3f * 11.21f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = gameTextures->planetTextures[4]};
    bodies[7]->velocity = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};

    return bodies;
}

void drawBodies(CelestialBody **bodies, int numBodies)
{
    for (int i = numBodies - 1; i >= 0; i--)
    {

        // Draw collider for debug
        // DrawCircle(bodies[i]->position.x, bodies[i]->position.y, bodies[i]->radius, WHITE);

        float scale = ((bodies[i]->radius * 2) / bodies[i]->texture->width) * TEXTURE_SCALE;
        // Source rectangle (part of the texture to use for drawing i.e. the whole texture)
        Rectangle source = {
            0,
            0,
            (float)bodies[i]->texture->width,
            (float)bodies[i]->texture->height};

        // Destination rectangle (Screen rectangle locating where to draw the texture)
        float widthScale = (bodies[i]->texture->width * scale / 2);
        float heightScale = (bodies[i]->texture->height * scale / 2);
        Rectangle dest = {
            bodies[i]->position.x,
            bodies[i]->position.y,
            (float)bodies[i]->texture->width + widthScale,
            (float)bodies[i]->texture->height + heightScale};

        // Origin of the texture for rotation and scaling - centre for our purpose
        Vector2 origin = {
            (float)((bodies[i]->texture->width + widthScale) / 2),
            (float)((bodies[i]->texture->height + heightScale) / 2)};
        DrawTexturePro(*bodies[i]->texture, source, dest, origin, bodies[i]->rotation, WHITE);
    }
}

void drawQuadtree(QuadTreeNode *node)
{
    if (node == NULL)
        return;
    DrawRectangleLines((int)node->bounds.x, (int)node->bounds.y,
                       (int)node->bounds.width, (int)node->bounds.height, DARKGRAY);

    for (int i = 0; i < 4; i++)
    {
        if (node->children[i] != NULL)
        {
            drawQuadtree(node->children[i]);
        }
    }
}

void drawPreviousPositions(CelestialBody **bodies, int numBodies)
{
    for (int i = 0; i < numBodies; i++)
    {
        for (int j = 0; j < PREVIOUS_POSITIONS; j++)
        {
            DrawPixelV(bodies[i]->previousPositions[j], TRAIL_COLOUR);

            // N
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, -1}), TRAIL_COLOUR);
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, -2}), TRAIL_COLOUR);

            // NE
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){1, -1}), TRAIL_COLOUR);

            // E
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){1, 0}), TRAIL_COLOUR);
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){2, 0}), TRAIL_COLOUR);

            // SE
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){1, 1}), TRAIL_COLOUR);

            // S
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, 1}), TRAIL_COLOUR);
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, 2}), TRAIL_COLOUR);

            // SW
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-1, 1}), TRAIL_COLOUR);

            // W
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-1, 0}), TRAIL_COLOUR);
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-2, 0}), TRAIL_COLOUR);

            // NW
            DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-1, -1}), TRAIL_COLOUR);
        }
    }
}

float calculateNormalisedZoom(CameraSettings *settings, float currentZoom)
{
    float midpoint = settings->minZoom + ((settings->maxZoom - settings->minZoom) / 2);
    return (float){currentZoom / midpoint};
}

void drawPlayerHUD(HUD *playerHUD)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float wMid = screenWidth / 2;
    float hMid = screenHeight / 2;
    // DrawCircle(wMid, screenHeight - 100, 100, (Color){255, 255, 255, 100});
    if (playerHUD->velocityTarget)
    {
        DrawText(TextFormat("Velocity Lock: %s", playerHUD->velocityTarget->name), screenWidth / 2 - MeasureText(TextFormat("Velocity Lock: %s", playerHUD->velocityTarget->name), 16) / 2, screenHeight - 120, 16, WHITE);
    }
    else
    {
        // char text = "Velocity Lock: Absolute";
        DrawText("Velocity Lock: Absolute", screenWidth / 2 - MeasureText("Velocity Lock: Absolute", 16) / 2, screenHeight - 120, 16, WHITE);
    }
    DrawText(TextFormat("%.1fkm/s", playerHUD->speed), screenWidth / 2 - MeasureText(TextFormat("%.1fkm/s", playerHUD->speed), 16) / 2, screenHeight - 100, 16, WHITE);

    Rectangle compassSource = {
        0,
        0,
        (float)playerHUD->compassTexture.width,
        (float)playerHUD->compassTexture.height};
    Rectangle compassDest = {
        wMid,
        screenHeight - 50,
        (float)playerHUD->compassTexture.width,
        (float)playerHUD->compassTexture.height};
    Vector2 compassOrigin = {
        (float)((playerHUD->compassTexture.width) / 2),
        (float)((playerHUD->compassTexture.height) / 2)};
    DrawTexturePro(playerHUD->compassTexture, compassSource, compassDest, compassOrigin, 0.0f, WHITE);

    float widthScale = (playerHUD->arrowTexture.width * -0.5 / 2);
    float heightScale = (playerHUD->arrowTexture.height * -0.5 / 2);
    Rectangle arrowSource = {
        0,
        0,
        (float)playerHUD->arrowTexture.width,
        (float)playerHUD->arrowTexture.height};
    Rectangle arrowDest = {
        wMid,
        screenHeight - 50,
        (float)playerHUD->arrowTexture.width + widthScale,
        (float)playerHUD->arrowTexture.height + heightScale};
    Vector2 arrowOrigin = {
        (float)((playerHUD->arrowTexture.width + widthScale) / 2),
        (float)((playerHUD->arrowTexture.height + heightScale) / 2)};
    DrawTexturePro(playerHUD->arrowTexture, arrowSource, arrowDest, arrowOrigin, playerHUD->playerRotation, WHITE);
}

void drawStaticGrid(float zoomLevel, int numQuadrants)
{
    int numLines = sqrt(numQuadrants) - 1;

    if (numLines < 1)
    {
        return;
    }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    for (int i = 0; i < numLines; i++)
    {
        // Draw horizontal line
        int horizontalHeight = screenHeight * (i + 1) / (numLines + 1);
        DrawLine(-screenWidth, horizontalHeight, screenWidth, horizontalHeight, GRID_COLOUR);
        // Draw vertical line
        int verticalWidth = screenWidth * (i + 1) / (numLines + 1);
        DrawLine(verticalWidth, -screenHeight, verticalWidth, screenHeight, GRID_COLOUR);
    }
}

void drawCelestialGrid(CelestialBody **bodies, int numBodies, Camera2D camera)
{
    /*
        Draws a grid with origin (0, 0) to the edges of visible space
        The grid should scale with the camera to demonstrate distance and velocity
    */
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int gridSpacing = round((GRID_SPACING / camera.zoom) / 1e4) * 1e4; // TODO: Clamp this to levels instead of continuous sizing

    // Calculate visible world-space bounds
    Vector2 topLeft = GetScreenToWorld2D((Vector2){0, 0}, camera);
    Vector2 bottomRight = GetScreenToWorld2D((Vector2){screenWidth, screenHeight}, camera);

    // Snap grid bounds to the nearest gridSpacing multiple
    float startX = floorf(topLeft.x / gridSpacing) * gridSpacing;
    float startY = floorf(topLeft.y / gridSpacing) * gridSpacing;
    float endX = ceilf(bottomRight.x / gridSpacing) * gridSpacing;
    float endY = ceilf(bottomRight.y / gridSpacing) * gridSpacing;

    // Draw vertical lines
    for (float x = startX; x <= endX; x += gridSpacing)
    {
        DrawLineV((Vector2){x, startY}, (Vector2){x, endY}, GRID_COLOUR);
    }

    // Draw horizontal lines
    for (float y = startY; y <= endY; y += gridSpacing)
    {
        DrawLineV((Vector2){startX, y}, (Vector2){endX, y}, GRID_COLOUR);
    }
}

void updateShipPosition(CelestialBody *ship)
{
    if (ship->type == TYPE_SHIP && ship->shipSettings.state == SHIP_LANDED && ship->shipSettings.landedBody != NULL)
    {
        // Keep ship positioned at the landing spot relative to the body
        ship->position = Vector2Add(ship->shipSettings.landedBody->position, ship->shipSettings.landingPosition);
        ship->velocity = ship->shipSettings.landedBody->velocity; // Sync velocity
    }
}

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

void freeCelestialBodies(CelestialBody **bodies, int numBodies)
{
    if (bodies)
    {
        for (int i = 0; i < numBodies; i++)
        {
            free(bodies[i]->name);
            free(bodies[i]->previousPositions);
            if (bodies[i]->futurePositions)
                free(bodies[i]->futurePositions);
            if (bodies[i]->resources)
                free(bodies[i]->resources);
            free(bodies[i]);
        }
        free(bodies);
    }
}

void drawPlayerStats(PlayerStats *playerStats)
{
    DrawText("Money:", 10, 40, 16, WHITE);
    DrawText(TextFormat("%i$", playerStats->money), 125 - MeasureText(TextFormat("%i$", playerStats->money), 16), 40, 16, WHITE);
}

void drawPlayerInventory(PlayerInventory *playerInventory)
{
    DrawText("Ice:", 10, 70, 16, WHITE);
    DrawText(TextFormat("%it", playerInventory->ice), 125 - MeasureText(TextFormat("%it", playerInventory->ice), 16), 70, 16, WHITE);
    DrawText("Copper:", 10, 100, 16, WHITE);
    DrawText(TextFormat("%it", playerInventory->copper), 125 - MeasureText(TextFormat("%it", playerInventory->copper), 16), 100, 16, WHITE);
    DrawText("Iron:", 10, 130, 16, WHITE);
    DrawText(TextFormat("%it", playerInventory->iron), 125 - MeasureText(TextFormat("%it", playerInventory->iron), 16), 130, 16, WHITE);
    DrawText("Gold:", 10, 160, 16, WHITE);
    DrawText(TextFormat("%it", playerInventory->gold), 125 - MeasureText(TextFormat("%it", playerInventory->gold), 16), 160, 16, WHITE);
}

void mineResources(CelestialBody *playerShip, PlayerInventory *playerInventory)
{
    if (playerShip->shipSettings.landedBody == NULL)
        return;
    for (int i = 0; i < playerShip->shipSettings.landedBody->resources; i++)
    {
        // Subtract minerals from body and add them to player inventory
    }
}