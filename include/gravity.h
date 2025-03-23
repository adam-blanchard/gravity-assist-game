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
#define PREVIOUS_POSITIONS 1000
#define FUTURE_POSITIONS 1000
#define FUTURE_STEP_TIME 0.1f
#define GRID_SPACING 1e2
#define GRID_LINE_WIDTH 100
#define HUD_ARROW_SCALE 0.01
#define MAX_LANDING_SPEED 1e4
#define MAX_CAPACITY 100
#define HUD_FONT_SIZE 16

#define TRAIL_COLOUR \
    CLITERAL(Color){255, 255, 255, 255}

#define GRID_COLOUR \
    CLITERAL(Color){255, 255, 255, 50}

#define SPACE_COLOUR \
    CLITERAL(Color){10, 10, 10, 255}

#define GUI_BACKGROUND \
    CLITERAL(Color){255, 255, 255, 200}

#define ORBIT_COLOUR \
    CLITERAL(Color){255, 255, 255, 100}

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
    RESOURCE_WATER_ICE = 0,
    RESOURCE_COPPER_ORE,
    RESOURCE_IRON_ORE,
    RESOURCE_GOLD_ORE,
    RESOURCE_COUNT // This tracks the number of resources and simplifies future additions
} ResourceType;

typedef struct CelestialBody CelestialBody;

typedef struct
{
    ResourceType type;
    char name[32]; // Name of the resource (e.g., "Water Ice")
    float weight;  // Weight per unit (e.g., for ship capacity limits)
    int value;     // Value for trading or crafting
} Resource;

typedef struct InventoryResource
{
    int resourceId; // Corresponds to ResourceType
    int quantity;   // Amount available on the CelestialBody
} InventoryResource;

typedef struct Ship
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float rotation;
    float radius;
    float thrust;
    float fuel;
    float fuelConsumption;
    bool isSelected;
    Texture2D *thrustTexture;
    ShipState state;
    CelestialBody *landedBody;
    Vector2 landingPosition;
    Vector2 *futurePositions;
} Ship;

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
    CelestialBody *parentBody;
    float orbitalRadius; // Distance from center for orbits (0 for black hole/ship)
    float angularSpeed;  // Radians per second (0 for black hole/ship)
    float initialAngle;  // Starting angle for orbit
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

float rad2deg(float rad)
{
    return rad * (180.0f / PI);
}

float deg2rad(float deg)
{
    return deg * (PI / 180.0f);
}

float radsPerSecond(int orbitalPeriod)
{
    float deg = 360.0f / orbitalPeriod;
    return deg2rad(deg);
}

float getBodyAngle(CelestialBody *body, float gameTime)
{
    return fmod(body->initialAngle + body->angularSpeed * gameTime, 2 * PI);
}

Vector2 calculateBodyVelocity(CelestialBody *body, float gameTime);

Vector2 calculateBodyVelocity(CelestialBody *body, float gameTime)
{
    // Recursive function to sum velocities of current body and its parents

    // Early return for non-orbiting bodies
    if (body->parentBody == NULL || body->orbitalRadius == 0)
        return (Vector2){0, 0};

    float angle = getBodyAngle(body, gameTime);
    float orbitalVelocity = calculateOrbitalVelocity(body->parentBody->mass, body->orbitalRadius);
    Vector2 velocity = (Vector2){
        orbitalVelocity * cosf(angle),
        orbitalVelocity * sinf(angle)};

    Vector2 parentVelocity = calculateBodyVelocity(body->parentBody, gameTime);

    return Vector2Add(velocity, parentVelocity);
}

float calculateRelativeSpeed(Ship *ship, CelestialBody *body, float gameTime)
{
    Vector2 bodyVelocity = calculateBodyVelocity(body, gameTime);
    Vector2 relativeVelocity = Vector2Subtract(ship->velocity, bodyVelocity);
    return sqrtf((relativeVelocity.x * relativeVelocity.x) + (relativeVelocity.y * relativeVelocity.y));
}

// float calculateAbsoluteSpeed(CelestialBody *body)
// {
//     return sqrtf((body->velocity.x * body->velocity.x) + (body->velocity.y * body->velocity.y));
// }

float calculateOrbitalSpeed(float mass, float radius)
{
    return (float){sqrtf((G * mass) / radius)};
}

void landShip(Ship *ship, CelestialBody *body, float gameTime)
{
    if (ship->state == SHIP_LANDED)
        return;

    // Set landing state
    ship->state = SHIP_LANDED;
    ship->landedBody = body;
    ship->velocity = calculateBodyVelocity(body, gameTime); // Match velocity to the body

    Vector2 direction = Vector2Subtract(ship->position, body->position);
    float distance = Vector2Length(direction);
    direction = Vector2Normalize(direction);

    // Position ship on the surface and store landing position
    Vector2 surfacePosition = Vector2Scale(direction, body->radius + ship->radius);
    ship->position = Vector2Add(body->position, surfacePosition);
    ship->landingPosition = surfacePosition; // Store relative position
}

void takeoffShip(Ship *ship)
{
    if (ship->state != SHIP_LANDED || ship->landedBody == NULL)
        return;

    ship->state = SHIP_FLYING;
    ship->landedBody = NULL;
}

CelestialBody **initBodies(int *numBodies)
{
    *numBodies = 3;
    CelestialBody **bodies = malloc(sizeof(CelestialBody *) * (*numBodies));

    // Star
    bodies[0] = malloc(sizeof(CelestialBody));
    *bodies[0] = (CelestialBody){
        .type = TYPE_STAR,
        .name = strdup("Sol"),
        .position = {0, 0},
        .mass = 1e15f,
        .radius = 1e4,
        .rotation = 0.0f,
        .parentBody = NULL,
        .angularSpeed = 0,
        .initialAngle = 0,
        .orbitalRadius = 0};

    // Planet orbiting Star - Earth
    bodies[1] = malloc(sizeof(CelestialBody));
    *bodies[1] = (CelestialBody){
        .type = TYPE_PLANET,
        .name = strdup("Earth"),
        .position = {0, 0},
        .mass = 1e9,
        .radius = 1e3,
        .rotation = 0.0f,
        .parentBody = bodies[0],
        .angularSpeed = radsPerSecond(365 * 24),
        .initialAngle = 0,
        .orbitalRadius = 9e4};

    // Moon orbiting Planet
    bodies[2] = malloc(sizeof(CelestialBody));
    *bodies[2] = (CelestialBody){
        .type = TYPE_MOON,
        .name = strdup("Earth's Moon"),
        .position = {0, 0},
        .mass = 1e7,
        .radius = 5e2,
        .rotation = 0.0f,
        .parentBody = bodies[1],
        .angularSpeed = radsPerSecond(28 * 24),
        .initialAngle = 0,
        .orbitalRadius = 2e4};

    return bodies;
}

Ship **initShips(int *numShips)
{
    *numShips = 1;
    Ship **ships = malloc(sizeof(Ship *) * (*numShips));

    ships[0] = malloc(sizeof(Ship));
    *ships[0] = (Ship){
        .position = (Vector2){3e4, 0},
        .velocity = (Vector2){0, 1.2e3},
        .mass = 1e3,
        .radius = 32.0f,
        .rotation = 0,
        .thrust = 1e2,
        .state = SHIP_FLYING,
        .isSelected = true,
        .futurePositions = malloc(sizeof(Vector2) * FUTURE_POSITIONS)};

    return ships;
}

Vector2 computeShipGravity(Ship *ship, CelestialBody **bodies, int numBodies)
{
    Vector2 totalForce = {0, 0};
    for (int i = 0; i < numBodies; i++)
    {
        Vector2 dir = Vector2Subtract(bodies[i]->position, ship->position);
        float dist = Vector2Length(dir);
        if (dist < 1e-5f)
            dist = 1e-5f;
        float mag = (G * ship->mass * bodies[i]->mass) / (dist * dist);
        totalForce = Vector2Add(totalForce, Vector2Scale(Vector2Normalize(dir), mag));
    }
    return totalForce;
}

void updateShipPositions(Ship **ships, int numShips, CelestialBody **bodies, int numBodies, float dt)
{
    for (int i = 0; i < numShips; i++)
    {
        Vector2 force = computeShipGravity(ships[i], bodies, numBodies);
        Vector2 accel = Vector2Scale(force, 1.0f / ships[i]->mass);
        ships[i]->velocity = Vector2Add(ships[i]->velocity, Vector2Scale(accel, dt));
        ships[i]->position = Vector2Add(ships[i]->position, Vector2Scale(ships[i]->velocity, dt));
    }
}

// Update celestial body positions (on rails)
void updateCelestialPositions(CelestialBody **bodies, int numBodies, float time)
{
    for (int i = 0; i < numBodies; i++)
    {
        if (bodies[i]->orbitalRadius > 0 && bodies[i]->parentBody != NULL)
        { // Stars, planets, moons
            float angle = getBodyAngle(bodies[i], time);
            bodies[i]->position = (Vector2){
                bodies[i]->parentBody->position.x + bodies[i]->orbitalRadius * cosf(angle),
                bodies[i]->parentBody->position.y + bodies[i]->orbitalRadius * sinf(angle)};
        }
    }
}

void detectCollisions(Ship **ships, int numShips, CelestialBody **bodies, int numBodies, float gameTime)
{
    for (int i = 0; i < numShips; i++)
    {
        for (int j = 0; j < numBodies; j++)
        {
            float dist = Vector2Distance(ships[i]->position, bodies[j]->position);
            if (dist < (ships[i]->radius + bodies[j]->radius))
            {
                printf("Collision between %s and Ship %i\n", bodies[j]->name, i);
                if (ships[i]->state != SHIP_LANDED)
                {
                    float relVel = calculateRelativeSpeed(ships[i], bodies[j], gameTime);
                    if (relVel <= MAX_LANDING_SPEED)
                    {
                        // landShip()
                        printf("Ship %i has landed on %s\n", i, bodies[j]->name);
                        landShip(ships[i], bodies[j], gameTime);
                    }
                    else
                    {
                        printf("Ship %i has CRASHED into %s\n", i, bodies[j]->name);
                    }
                }
            }
        }
    }
}

void calculateShipFuturePositions(Ship **ships, int numShips, CelestialBody **bodies, int numBodies, float gameTime)
{
    Vector2 initialVelocities[numShips];
    Vector2 initialPositions[numShips];

    for (int i = 0; i < numShips; i++)
    {
        initialVelocities[i] = ships[i]->velocity;
        initialPositions[i] = ships[i]->position;
    }

    for (int i = 0; i < FUTURE_POSITIONS; i++)
    {
        float futureTime = gameTime + (i * FUTURE_STEP_TIME);

        // Update celestial body positions at this future time
        updateCelestialPositions(bodies, numBodies, futureTime);

        // Compute gravitational force on the ship at this step
        updateShipPositions(ships, numShips, bodies, numBodies, FUTURE_STEP_TIME);

        // Store the predicted position
        for (int j = 0; j < numShips; j++)
        {
            if (ships[j]->state == SHIP_FLYING)
                ships[j]->futurePositions[i] = ships[j]->position;
            if (ships[j]->state == SHIP_LANDED)
                ships[j]->futurePositions[i] = ships[j]->landedBody->position;
        }
    }

    updateCelestialPositions(bodies, numBodies, gameTime);
    for (int i = 0; i < numShips; i++)
    {
        ships[i]->velocity = initialVelocities[i];
        ships[i]->position = initialPositions[i];
    }
}

void drawBodies(CelestialBody **bodies, int numBodies)
{
    Color bodyColour;
    for (int i = numBodies - 1; i >= 0; i--)
    {
        if (bodies[i]->type == TYPE_STAR)
        {
            bodyColour = RED;
        }
        if (bodies[i]->type == TYPE_PLANET)
        {
            bodyColour = BLUE;
        }
        if (bodies[i]->type != TYPE_STAR && bodies[i]->type != TYPE_PLANET)
        {
            bodyColour = WHITE;
        }
        DrawCircle(bodies[i]->position.x, bodies[i]->position.y, bodies[i]->radius, bodyColour);
    }
}

void drawShips(Ship **ships, int numShips)
{
    for (int i = 0; i < numShips; i++)
    {
        DrawCircleV(ships[i]->position, 32.0f, YELLOW);
    }
}

void drawOrbits(CelestialBody **bodies, int numBodies)
{
    for (int i = 0; i < numBodies; i++)
    {
        if (bodies[i]->orbitalRadius > 0 && bodies[i]->parentBody != NULL)
        {
            DrawCircleLinesV(bodies[i]->parentBody->position, bodies[i]->orbitalRadius, ORBIT_COLOUR);
        }
    }
}

void drawTrajectories(Ship **ships, int numShips)
{
    for (int i = 0; i < numShips; i++)
    {
        for (int j = 0; j < FUTURE_POSITIONS; j++)
        {
            if (j > 0)
                DrawLineV(ships[i]->futurePositions[j - 1], ships[i]->futurePositions[j], ORBIT_COLOUR);
        }
    }
}

// void drawBodies(CelestialBody **bodies, int numBodies)
// {
//     for (int i = numBodies - 1; i >= 0; i--)
//     {
//         float scale = ((bodies[i]->radius * 2) / bodies[i]->texture->width) * bodies[i]->textureScale;
//         // Source rectangle (part of the texture to use for drawing i.e. the whole texture)
//         Rectangle source = {
//             0,
//             0,
//             (float)bodies[i]->texture->width,
//             (float)bodies[i]->texture->height};

//         // Destination rectangle (Screen rectangle locating where to draw the texture)
//         float widthScale = (bodies[i]->texture->width * scale / 2);
//         float heightScale = (bodies[i]->texture->height * scale / 2);
//         Rectangle dest = {
//             bodies[i]->position.x,
//             bodies[i]->position.y,
//             (float)bodies[i]->texture->width + widthScale,
//             (float)bodies[i]->texture->height + heightScale};

//         // Origin of the texture for rotation and scaling - centre for our purpose
//         Vector2 origin = {
//             (float)((bodies[i]->texture->width + widthScale) / 2),
//             (float)((bodies[i]->texture->height + heightScale) / 2)};
//         DrawTexturePro(*bodies[i]->texture, source, dest, origin, bodies[i]->rotation, WHITE);

//         // Draw collider for debug
//         // DrawCircle(bodies[i]->position.x, bodies[i]->position.y, bodies[i]->radius, WHITE);
//     }
// }

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

// void drawPreviousPositions(CelestialBody **bodies, int numBodies)
// {
//     for (int i = 0; i < numBodies; i++)
//     {
//         for (int j = 0; j < PREVIOUS_POSITIONS; j++)
//         {
//             DrawPixelV(bodies[i]->previousPositions[j], TRAIL_COLOUR);

//             // N
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, -1}), TRAIL_COLOUR);
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, -2}), TRAIL_COLOUR);

//             // NE
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){1, -1}), TRAIL_COLOUR);

//             // E
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){1, 0}), TRAIL_COLOUR);
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){2, 0}), TRAIL_COLOUR);

//             // SE
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){1, 1}), TRAIL_COLOUR);

//             // S
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, 1}), TRAIL_COLOUR);
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){0, 2}), TRAIL_COLOUR);

//             // SW
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-1, 1}), TRAIL_COLOUR);

//             // W
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-1, 0}), TRAIL_COLOUR);
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-2, 0}), TRAIL_COLOUR);

//             // NW
//             DrawPixelV(Vector2Add(bodies[i]->previousPositions[j], (Vector2){-1, -1}), TRAIL_COLOUR);
//         }
//     }
// }

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

void updateShipPosition(Ship **ships, int numShips, float gameTime)
{
    for (int i = 0; i < numShips; i++)
    {
        if (ships[i]->state == SHIP_LANDED && ships[i]->landedBody != NULL)
        {
            // Keep ship positioned at the landing spot relative to the body
            ships[i]->position = Vector2Add(ships[i]->landedBody->position, ships[i]->landingPosition);
            ships[i]->velocity = calculateBodyVelocity(ships[i]->landedBody, gameTime); // Sync velocity
        }
    }
}

// void freeGameTextures(GameTextures gameTextures)
// {
//     if (
//         gameTextures.numStarTextures > 0 || gameTextures.numPlanetTextures > 0 || gameTextures.numMoonTextures > 0 || gameTextures.numShipTextures > 0)
//     {
//         for (int i = 0; i < gameTextures.numStarTextures; i++)
//         {
//             UnloadTexture(*gameTextures.starTextures[i]);
//             free(gameTextures.starTextures[i]);
//         }
//         free(gameTextures.starTextures);

//         for (int i = 0; i < gameTextures.numPlanetTextures; i++)
//         {
//             UnloadTexture(*gameTextures.planetTextures[i]);
//             free(gameTextures.planetTextures[i]);
//         }
//         free(gameTextures.planetTextures);

//         for (int i = 0; i < gameTextures.numMoonTextures; i++)
//         {
//             UnloadTexture(*gameTextures.moonTextures[i]);
//             free(gameTextures.moonTextures[i]);
//         }
//         free(gameTextures.moonTextures);

//         for (int i = 0; i < gameTextures.numShipTextures; i++)
//         {
//             UnloadTexture(*gameTextures.shipTextures[i]);
//             free(gameTextures.shipTextures[i]);
//         }
//         free(gameTextures.shipTextures);
//     }
// }

// void freeCelestialBodies(CelestialBody **bodies, int numBodies)
// {
//     if (bodies)
//     {
//         for (int i = 0; i < numBodies; i++)
//         {
//             free(bodies[i]->name);
//             free(bodies[i]->previousPositions);
//             if (bodies[i]->futurePositions)
//                 free(bodies[i]->futurePositions);
//             // if (bodies[i]->resources)
//             //     free(bodies[i]->resources);
//             free(bodies[i]);
//         }
//         free(bodies);
//     }
// }

void freeCelestialBodies(CelestialBody **bodies, int numBodies)
{
    if (bodies)
    {
        for (int i = 0; i < numBodies; i++)
        {
            free(bodies[i]->name);
            // if (bodies[i]->resources)
            //     free(bodies[i]->resources);
            free(bodies[i]);
        }
        free(bodies);
    }
}

void drawPlayerStats(PlayerStats *playerStats)
{
    DrawText("Money:", 10, 40, HUD_FONT_SIZE, WHITE);
    DrawText(TextFormat("%i$", playerStats->money), 150 - MeasureText(TextFormat("%i$", playerStats->money), HUD_FONT_SIZE), 40, HUD_FONT_SIZE, WHITE);
}

// void drawPlayerInventory(CelestialBody *playerShip, Resource *resourceDefinitions)
// {
//     int initialHeight = 70;
//     int heightStep = 30;
//     for (int i = 0; i < RESOURCE_COUNT; i++)
//     {
//         // Currently renders the associated enum integer of each mineral
//         // Possibly need to change mineral to be a struct so we can store a name (char array)
//         DrawText(TextFormat("%s:", resourceDefinitions[i].name), 10, (initialHeight + (heightStep * i)), HUD_FONT_SIZE, WHITE);
//         DrawText(TextFormat("%it", playerShip->shipSettings.inventory[i].quantity), 150 - MeasureText(TextFormat("%it", playerShip->shipSettings.inventory[i].quantity), HUD_FONT_SIZE), (initialHeight + (heightStep * i)), HUD_FONT_SIZE, WHITE);
//     }
// }

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

// CelestialBody **copyCelestialBodies(CelestialBody **original, int numBodies)
// {
//     // Step 1: Allocate memory for the new array of pointers
//     CelestialBody **copy = (CelestialBody **)malloc(numBodies * sizeof(CelestialBody *));
//     if (copy == NULL)
//     {
//         // Handle memory allocation failure
//         return NULL;
//     }

//     // Step 2: Copy each CelestialBody
//     for (int i = 0; i < numBodies; i++)
//     {
//         // Allocate memory for the new CelestialBody
//         copy[i] = (CelestialBody *)malloc(sizeof(CelestialBody));
//         if (copy[i] == NULL)
//         {
//             // Handle memory allocation failure and clean up
//             for (int j = 0; j < i; j++)
//             {
//                 free(copy[j]);
//             }
//             free(copy);
//             return NULL;
//         }

//         // Step 3: Copy the data from the original to the new body
//         memcpy(copy[i], original[i], sizeof(CelestialBody));
//     }

//     return copy;
// }

// void predictPositions(CelestialBody **bodies, int numBodies, WarpController *timeScale, float *theta)
// {
//     // Whole-system simulation is computationally expensive, can we make this more lightweight?

//     // Make a copy of the current state of the system to simulate on
//     CelestialBody **workingBodies = copyCelestialBodies(bodies, numBodies);

//     for (int i = 0; i < FUTURE_POSITIONS; i++)
//     {
//         QuadTreeNode *root = buildQuadTree(workingBodies, numBodies);

//         // Update physics
//         float scaledDt = FUTURE_STEP_TIME * timeScale->val;

//         for (int j = 0; j < numBodies; j++)
//         {
//             if (workingBodies[j]->type == TYPE_SHIP && workingBodies[j]->shipSettings.state == SHIP_LANDED)
//             {
//                 continue;
//             }

//             Vector2 force = computeForce(root, workingBodies[j], *theta);
//             Vector2 accel = Vector2Scale(force, 1.0f / workingBodies[j]->mass);
//             workingBodies[j]->velocity = Vector2Add(workingBodies[j]->velocity, Vector2Scale(accel, scaledDt));
//             workingBodies[j]->position = Vector2Add(workingBodies[j]->position, Vector2Scale(workingBodies[j]->velocity, scaledDt));

//             if (bodies[j]->type != TYPE_STAR)
//             {
//                 bodies[j]->futurePositions[i] = workingBodies[j]->position;
//             }
//         }
//         free(root);
//     }

//     for (int i = 0; i < numBodies; i++)
//     {
//         free(workingBodies[i]);
//     }
//     free(workingBodies);
// }

// void drawFuturePositions(CelestialBody **bodies, int numBodies)
// {

//     for (int j = 0; j < FUTURE_POSITIONS; j++)
//     {
//         // Calculate the ship's position relative to the planet at each timestep
//         Vector2 planetPos = bodies[2]->futurePositions[j];
//         Vector2 shipPos = bodies[0]->futurePositions[j];
//         Vector2 relativePos = Vector2Subtract(shipPos, planetPos);

//         // Draw the relative position offset from the planet's current position
//         Vector2 drawPos = Vector2Add(bodies[2]->position, relativePos);
//         DrawPixelV(drawPos, TRAIL_COLOUR);

//         // N
//         DrawPixelV(Vector2Add(drawPos, (Vector2){0, -1}), TRAIL_COLOUR);
//         DrawPixelV(Vector2Add(drawPos, (Vector2){0, -2}), TRAIL_COLOUR);

//         // NE
//         DrawPixelV(Vector2Add(drawPos, (Vector2){1, -1}), TRAIL_COLOUR);

//         // E
//         DrawPixelV(Vector2Add(drawPos, (Vector2){1, 0}), TRAIL_COLOUR);
//         DrawPixelV(Vector2Add(drawPos, (Vector2){2, 0}), TRAIL_COLOUR);

//         // SE
//         DrawPixelV(Vector2Add(drawPos, (Vector2){1, 1}), TRAIL_COLOUR);

//         // S
//         DrawPixelV(Vector2Add(drawPos, (Vector2){0, 1}), TRAIL_COLOUR);
//         DrawPixelV(Vector2Add(drawPos, (Vector2){0, 2}), TRAIL_COLOUR);

//         // SW
//         DrawPixelV(Vector2Add(drawPos, (Vector2){-1, 1}), TRAIL_COLOUR);

//         // W
//         DrawPixelV(Vector2Add(drawPos, (Vector2){-1, 0}), TRAIL_COLOUR);
//         DrawPixelV(Vector2Add(drawPos, (Vector2){-2, 0}), TRAIL_COLOUR);

//         // NW
//         DrawPixelV(Vector2Add(drawPos, (Vector2){-1, -1}), TRAIL_COLOUR);
//     }
// }

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