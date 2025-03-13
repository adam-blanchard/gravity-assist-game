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
#define TEXTURE_SCALE 2.7
#define GRID_SPACING 1e2
#define GRID_LINE_WIDTH 100

#define TRAIL_COLOUR \
    CLITERAL(Color) { 255, 255, 255, 255 }

#define GRID_COLOUR \
    CLITERAL(Color) { 255, 255, 255, 50 }

#define SPACE_COLOUR \
    CLITERAL(Color) { 10, 10, 10, 255 }

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
    BODY_STAR,
    BODY_PLANET,
    BODY_MOON,
    BODY_ASTEROID
} BodyType;

typedef struct ShipSettings
{
    float thrust;
    float fuel;
    float fuelConsumption;
    bool isSelected;
    Texture2D *thrustTexture;
} ShipSettings;

typedef struct CelestialBody
{
    CelestialType type;
    char *name;
    Vector2 position;
    Vector2 velocity;
    float mass; // Kg
    float radius;
    Vector2 *previousPositions;
    float rotation;
    ShipSettings shipSettings;
    Texture2D *texture;
} CelestialBody;

typedef struct
{
    int numStarTextures;
    Texture2D *starTextures;
    int numPlanetTextures;
    Texture2D *planetTextures;
    int numMoonTextures;
    Texture2D *moonTextures;
} GameTextures;

typedef struct QuadTreeNode
{
    Rectangle bounds;                 // 2D region (x, y, width, height)
    Vector2 centerOfMass;             // Center of mass of all bodies in this node
    float totalMass;                  // Total mass of bodies in this node
    struct QuadTreeNode *children[4]; // NW, NE, SW, SE quadrants
    CelestialBody *body;              // Pointer to body if leaf node; NULL otherwise
} QuadTreeNode;

// Structure to represent celestial bodies
typedef struct
{
    char name[16];
    Vector2 position;
    Vector2 velocity;
    float mass; // Kg
    float radius;
    float rotation;
    float rotationPeriod;
    float orbitalPeriod; // 1 is 10 seconds so 360 represents an orbital period of 1 hour
    float orbitalRadius;
    BodyType type;
    int orbitalBodyIndex; // Reference to the primary body this body orbits around - planets around a star, moons around a planet
    Vector2 *futurePositions;
    Vector2 *futureVelocities;
    int futureSteps;
    int fontSize;
    Texture2D texture;
} Body;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass; // Kg
    float rotation;
    float thrust; // kN
    float fuel;
    float fuelConsumption;
    float colliderRadius;
    ShipState state;
    int alive;
    Body *landedBody;
    Vector2 *futurePositions;
    Vector2 *futureVelocitites;
    int futureSteps;
    Texture2D idleTexture;
    Texture2D thrustTexture;
    Texture2D *activeTexture;
} Ship;

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
    Texture2D arrowTexture;
} HUD;

typedef struct
{
    float defaultZoom;
    float minZoom;
    float maxZoom;
} CameraSettings;

float _Clamp(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

Vector2 _Vector2Add(Vector2 *v1, Vector2 *v2)
{
    return (Vector2){
        v1->x + v2->x,
        v1->y + v2->y};
}

Vector2 _Vector2Subtract(Vector2 *v1, Vector2 *v2)
{
    return (Vector2){
        v1->x - v2->x,
        v1->y - v2->y};
}

Vector2 _Vector2Scale(Vector2 v, float scalar)
{
    return (Vector2){
        v.x * scalar,
        v.y * scalar};
}

float _Vector2Length(Vector2 v)
{
    return (float){
        sqrtf(v.x * v.x + v.y * v.y)};
}

Vector2 _Vector2Normalize(Vector2 v)
{
    float length = sqrtf(v.x * v.x + v.y * v.y);

    // Avoid division by zero
    if (length == 0)
        return (Vector2){0, 0};

    return (Vector2){
        v.x / length,
        v.y / length};
}

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

float calculateAngle(Vector2 *planetPosition, Vector2 *sunPosition)
{
    Vector2 relativePosition = _Vector2Subtract(planetPosition, sunPosition);
    float angleInRadians = atan2f(relativePosition.y, relativePosition.x);
    float angleInDegrees = angleInRadians * (180.0f / PI);

    // Normalize to 0-360 degrees
    return fmod(angleInDegrees + 360.0f, 360.0f);
}

float calculateOrbitalRadius(float period, float mStar)
{
    float r3 = (period * period * G * mStar) / (4 * PI * PI);
    return (float){cbrtf(r3)};
}

void incrementWarp(WarpController *timeScale, float dt)
{
    timeScale->val += timeScale->increment * timeScale->val * dt;
    timeScale->val = _Clamp(timeScale->val, timeScale->min, timeScale->max);
}

void decrementWarp(WarpController *timeScale, float dt)
{
    timeScale->val -= timeScale->increment * timeScale->val * dt;
    timeScale->val = _Clamp(timeScale->val, timeScale->min, timeScale->max);
}

float calculateShipSpeed(Ship *playerShip, Vector2 *targetVelocity)
{
    Vector2 relativeVelocity = _Vector2Subtract(&playerShip->velocity, targetVelocity);
    return sqrtf((relativeVelocity.x * relativeVelocity.x) + (relativeVelocity.y * relativeVelocity.y));
}

void landShip(Ship *ship, Body *planet)
{
    ship->state = SHIP_LANDED;
    ship->landedBody = planet;
    ship->velocity = planet->velocity;

    // Position ship on planet’s surface
    // Vector2 direction = _Vector2Subtract(&ship->position, &planet->position);
    // direction = _Vector2Normalize(direction);
    // Vector2 scaledDirection = _Vector2Scale(direction, planet->radius + ship->colliderRadius);
    // ship->position = _Vector2Add(&planet->position, &scaledDirection);
}

void spawnRocketOnBody(Ship *ship, Body *planet)
{
    Vector2 planetSpawnLocation = (Vector2){0, -planet->radius};
    ship->position = _Vector2Add(&planet->position, &planetSpawnLocation);
    ship->velocity = planet->velocity;
}

float calculateOrbitalSpeed(float mass, float radius)
{
    return (float){sqrtf((G * mass) / radius)};
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
    // float padding = 10.0f;
    // minX -= padding;
    // maxX += padding;
    // minY -= padding;
    // maxY += padding;
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
            printf("Collision between %s and %s\n", body->name, node->body->name);
            if (body->type == TYPE_SHIP)
            {
                // Reset ship (example handling)
                body->position = Vector2Add(bodies[1]->position, (Vector2){1e4, 0});
                Vector2 relVel = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, 1e4)};
                body->velocity = Vector2Add(bodies[1]->velocity, relVel);
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

CelestialBody **initBodies(int *numBodies, Texture2D **starTextures, Texture2D **planetTextures, Texture2D **moonTextures, Texture2D **shipTextures)
{
    *numBodies = 4;
    CelestialBody **bodies = malloc(sizeof(CelestialBody *) * (*numBodies));

    // Maximum 64-bit number is 1.8e19
    // Heaviest object in the universe is the supermassive black hole at the centre - 1e18
    // The supermassive black hole is 1 million solar masses of Sol
    // All other masses are derived from Sol's mass

    // Simulating a supermassive black hole at the centre of the universe
    // bodies[0] = malloc(sizeof(CelestialBody));
    // *bodies[0] = (CelestialBody){.type = TYPE_UNIVERSE,
    //                              .name = strdup("Centre of Universe"),
    //                              .position = {0, 0},
    //                              .velocity = {0, 0},
    //                              .mass = 1e18f,
    //                              .radius = 1.0f,
    //                              .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
    //                              .rotation = 0.0f};

    // Star
    bodies[0] = malloc(sizeof(CelestialBody));
    *bodies[0] = (CelestialBody){.type = TYPE_STAR,
                                 .name = strdup("Sol"),
                                 .position = {100, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e15f,
                                 .radius = 3.2e5,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = starTextures[0]};

    // Planet orbiting Star
    float orbitalRadius = calculateOrbitalRadius(24 * 60 * 60, bodies[0]->mass);
    bodies[1] = malloc(sizeof(CelestialBody));
    *bodies[1] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Earth"),
                                 .position = {bodies[0]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9,
                                 .radius = 3.2e3f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = planetTextures[0]};
    bodies[1]->velocity = (Vector2){0, calculateOrbitalSpeed(bodies[0]->mass, orbitalRadius)};

    // Moon orbiting Planet
    orbitalRadius = calculateOrbitalRadius(12 * 60 * 60, bodies[1]->mass);
    bodies[2] = malloc(sizeof(CelestialBody));
    *bodies[2] = (CelestialBody){.type = TYPE_MOON,
                                 .name = strdup("Moon1"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e7,
                                 .radius = 3.2e2f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .texture = moonTextures[0]};
    Vector2 relVel = (Vector2){0, -calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};
    bodies[2]->velocity = Vector2Add(bodies[1]->velocity, relVel);

    // Ship
    // Ship texture is 64x64
    bodies[3] = malloc(sizeof(CelestialBody));
    *bodies[3] = (CelestialBody){.type = TYPE_SHIP,
                                 .name = strdup("Ship"),
                                 .position = {0, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e1f,
                                 .radius = 32.0f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .shipSettings = (ShipSettings){
                                     .thrust = 5e1f,
                                     .fuel = 100.0f,
                                     .fuelConsumption = 0.0f,
                                     .isSelected = true,
                                     .thrustTexture = shipTextures[1]},
                                 .texture = shipTextures[0]};
    bodies[3]->position = Vector2Add(bodies[1]->position, (Vector2){1e4, 0});
    relVel = (Vector2){0, calculateOrbitalSpeed(bodies[1]->mass, 1e4)};
    bodies[3]->velocity = Vector2Add(bodies[1]->velocity, relVel);

    return bodies;
}

void drawBodies(CelestialBody **bodies, int numBodies)
{
    for (int i = 0; i < numBodies; i++)
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

void drawPlayerHUD(HUD *playerHUD, float *shipRotation, int *screenWidth, int *screenHeight)
{
    float wMid = *screenWidth / 2;
    float hMid = *screenHeight / 2;
    Rectangle arrowSource = {0, 0, (float)playerHUD->arrowTexture.width, (float)playerHUD->arrowTexture.height};
    Rectangle arrowDest = {wMid, *screenHeight - 50, (float)playerHUD->arrowTexture.width, (float)playerHUD->arrowTexture.height};
    Vector2 arrowOrigin = {(float)playerHUD->arrowTexture.width / 2, (float)playerHUD->arrowTexture.height / 2};
    DrawTexturePro(playerHUD->arrowTexture, arrowSource, arrowDest, arrowOrigin, *shipRotation, WHITE);
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

    // Highlight the origin (0, 0) with thicker lines
    // DrawLineV((Vector2){0, startY}, (Vector2){0, endY}, RED); // Vertical axis
    // DrawLineV((Vector2){startX, 0}, (Vector2){endX, 0}, RED); //
}

// GameTextures loadGameTextures()
// {
//     Texture2D star1 = LoadTexture("./textures/star/sun.png");
//     Texture2D planet1 = LoadTexture("./textures/planet/planet_1.png");
//     Texture2D moon1 = LoadTexture("./textures/moon/moon.png");

//     GameTextures gameTextures = {
//         .numStarTextures = 1,
//         .starTextures = {&star1},
//         .numPlanetTextures = 1,
//         .planetTextures = {&planet1},
//         .numMoonTextures = 1,
//         .moonTextures = {&moon1}};

//     return gameTextures;
// }