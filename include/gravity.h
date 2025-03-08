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
#define G 6.67430e-11
#endif
#define TRAJECTORY_STEPS 6000
#define TRAJECTORY_STEP_TIME 0.033f
#define PREVIOUS_POSITIONS 1000

#define GRID_COLOUR \
    CLITERAL(Color) { 255, 255, 255, 50 }

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
    TYPE_UNIVERSE, // Root node
    TYPE_STAR,
    TYPE_PLANET,
    TYPE_MOON,
    TYPE_SHIP
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
} CelestialBody;

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

// Function to compute gravitational force between two bodies
Vector2 gravitationalForce(Body *b1, Body *b2)
{
    Vector2 direction = _Vector2Subtract(&b2->position, &b1->position);
    float distance = _Vector2Length(direction);
    if (distance == 0)
        return (Vector2){0, 0}; // Avoid division by zero

    float forceMagnitude = G * b1->mass * b2->mass / (distance * distance);
    direction = _Vector2Normalize(direction);
    return _Vector2Scale(direction, forceMagnitude);
}

Vector2 gravitationalForce2(Vector2 *pos1, Vector2 *pos2, float mass1, float mass2)
{
    Vector2 direction = _Vector2Subtract(pos2, pos1);
    float distance = _Vector2Length(direction);
    if (distance == 0)
        return (Vector2){0, 0}; // Avoid division by zero

    float forceMagnitude = G * mass1 * mass2 / (distance * distance);
    direction = _Vector2Normalize(direction);
    return _Vector2Scale(direction, forceMagnitude);
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

void updateBodies(int n, Body *bodies, float dt)
{
    for (int m = 0; m < n; m++)
    {
        Vector2 force = {0};

        for (int i = 0; i < n; i++)
        {
            if (m != i)
            {
                Vector2 planetaryForce = gravitationalForce(&bodies[m], &bodies[i]);
                force = _Vector2Add(&force, &planetaryForce);
            }
        }

        Vector2 acceleration = {force.x / bodies[m].mass, force.y / bodies[m].mass};

        bodies[m].velocity.x += acceleration.x * dt;
        bodies[m].velocity.y += acceleration.y * dt;

        if (bodies[m].type == BODY_MOON)
        {
            int parentIndex = bodies[m].orbitalBodyIndex;
            bodies[m].velocity = _Vector2Add(&bodies[parentIndex].velocity, &bodies[m].velocity);
        }

        bodies[m].position.x += bodies[m].velocity.x * dt;
        bodies[m].position.y += bodies[m].velocity.y * dt;

        // Handle body rotation
        // bodies[m].rotation += 0.1f;
    }
}

void updateShip(Ship *ship, int n, Body *bodies, float dt)
{
    ship->activeTexture = &ship->idleTexture;
    if (ship->state == SHIP_FLYING)
    {
        // Apply rotation
        if (IsKeyDown(KEY_D))
            ship->rotation += 180.0f * dt; // Rotate right
        if (IsKeyDown(KEY_A))
            ship->rotation -= 180.0f * dt; // Rotate left

        // Normalize rotation to keep it within 0-360 degrees
        ship->rotation = fmod(ship->rotation + 360.0f, 360.0f);
    }

    // Apply thrust
    if (IsKeyDown(KEY_W) && ship->fuel > 0)
    {
        // Convert rotation to radians for vector calculations
        float radians = ship->rotation * PI / 180.0f;
        Vector2 thrustDirection = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
        Vector2 thrust = _Vector2Scale(thrustDirection, ship->thrust * dt);
        ship->velocity = _Vector2Add(&ship->velocity, &thrust);
        ship->activeTexture = &ship->thrustTexture;
        ship->fuel -= ship->fuelConsumption;

        if (ship->state == SHIP_LANDED)
        {
            ship->state = SHIP_FLYING;
            ship->landedBody = NULL;
        }
    }

    ship->fuel = _Clamp(ship->fuel, 0.0f, 100.0f);

    Vector2 force = {0};

    if (ship->state == SHIP_FLYING)
    {
        for (int i = 0; i < n; i++)
        {
            Vector2 planetaryForce = gravitationalForce2(&ship->position, &bodies[i].position, ship->mass, bodies[i].mass);
            force = _Vector2Add(&force, &planetaryForce);
        }

        Vector2 acceleration = {force.x / ship->mass, force.y / ship->mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, dt);
        ship->velocity = _Vector2Add(&ship->velocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(ship->velocity, dt);
        ship->position = _Vector2Add(&ship->position, &scaledVelocity);
    }

    if (ship->state == SHIP_LANDED)
    {
        Vector2 sub = _Vector2Subtract(&ship->position, &ship->landedBody->position);
        Vector2 norm = _Vector2Normalize(sub);
        Vector2 scale = _Vector2Scale(norm, ship->landedBody->radius + ship->colliderRadius);
        ship->position = _Vector2Add(&ship->landedBody->position, &scale);
        ship->velocity = ship->landedBody->velocity; // Only sync here if not taking off
    }
}

float calculateAngle(Vector2 *planetPosition, Vector2 *sunPosition)
{
    Vector2 relativePosition = _Vector2Subtract(planetPosition, sunPosition);
    float angleInRadians = atan2f(relativePosition.y, relativePosition.x);
    float angleInDegrees = angleInRadians * (180.0f / PI);

    // Normalize to 0-360 degrees
    return fmod(angleInDegrees + 360.0f, 360.0f);
}

void initialiseRandomPositions(int n, Body *bodies, int maxWidth, int maxHeight)
{
    for (int i = 1; i < n; i++)
    {
        bodies[i].position.x = GetRandomValue(100, maxWidth);
        bodies[i].position.y = GetRandomValue(100, maxHeight);
    }
}

void predictBodyTrajectory(Body *body, Body *otherBodies, int numBodies, Vector2 *trajectory)
{
    Vector2 currentPosition = body->position;
    Vector2 currentVelocity = body->velocity;

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 totalForce = {0};
        for (int j = 0; j < numBodies; j++)
        {
            if (&otherBodies[j] != body)
            {
                Vector2 force = gravitationalForce2(&currentPosition, &otherBodies[j].position, body->mass, otherBodies[j].mass);
                totalForce = _Vector2Add(&totalForce, &force);
            }
        }

        Vector2 acceleration = {totalForce.x / body->mass, totalForce.y / body->mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

void predictShipTrajectory(Ship *playerShip, Body *otherBodies, int numBodies, Vector2 *trajectory)
{
    // Isolated prediction of the ship based on current position and velocity
    // TODO: improve trajectory accuracy by simulating all bodies x timesteps in advance
    Vector2 currentPosition = playerShip->position;
    Vector2 currentVelocity = playerShip->velocity;

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 force = {0};

        for (int j = 0; j < numBodies; j++)
        {
            Vector2 planetaryForce = gravitationalForce2(&currentPosition, &otherBodies[j].position, playerShip->mass, otherBodies[j].mass);
            force = _Vector2Add(&force, &planetaryForce);
        }

        Vector2 acceleration = {force.x / playerShip->mass, force.y / playerShip->mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

void predictTrajectory(Vector2 *initialPosition, Vector2 *initialVelocity, float *mass, Body *otherBodies, int numBodies, Vector2 *trajectory)
{
    float radiusThreshold = 5.0f;
    Vector2 currentPosition = *initialPosition;
    Vector2 currentVelocity = *initialVelocity;

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 totalForce = {0};
        for (int j = 0; j < numBodies; j++)
        {
            if (calculateDistance(&currentPosition, &otherBodies[j].position) > radiusThreshold)
            {
                Vector2 force = gravitationalForce2(&currentPosition, &otherBodies[j].position, *mass, otherBodies[j].mass);
                totalForce = _Vector2Add(&totalForce, &force);
            }
        }

        Vector2 acceleration = {totalForce.x / *mass, totalForce.y / *mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

void predictTimesteps(Ship *playerShip, Body *bodies, int numBodies)
{
    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        for (int j = 0; j < numBodies; j++)
        {
            Vector2 force = {0};
            Vector2 body1Position = {0};
            Vector2 body1Velocity = {0};
            Vector2 body2Position = {0};
            Vector2 body2Velocity = {0};

            if (i > 0)
            {
                body1Position = bodies[j].futurePositions[i - 1];
                body1Velocity = bodies[j].futureVelocities[i - 1];
            }
            else
            {
                body1Position = bodies[j].position;
                body1Velocity = bodies[j].velocity;
            }

            for (int k = 0; k < numBodies; k++)
            {
                if (i > 0)
                {
                    body2Position = bodies[k].futurePositions[i - 1];
                    body1Velocity = bodies[k].futureVelocities[i - 1];
                }
                else
                {
                    body2Position = bodies[k].position;
                    body1Velocity = bodies[k].velocity;
                }

                if (j != k)
                {
                    // Vector2 planetaryForce = gravitationalForce(&bodies[j], &bodies[k]);
                    Vector2 planetaryForce = gravitationalForce2(&body1Position, &body2Position, bodies[j].mass, bodies[k].mass);
                    force = _Vector2Add(&force, &planetaryForce);
                }
            }

            Vector2 acceleration = {force.x / bodies[j].mass, force.y / bodies[j].mass};

            bodies[j].futureVelocities[i].x += acceleration.x * TRAJECTORY_STEP_TIME;
            bodies[j].futureVelocities[i].y += acceleration.y * TRAJECTORY_STEP_TIME;

            bodies[j].futurePositions[i].y += bodies[j].futureVelocities[i].y * TRAJECTORY_STEP_TIME;
            bodies[j].futurePositions[i].x += bodies[j].futureVelocities[i].x * TRAJECTORY_STEP_TIME;
        }
    }
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

void initialiseOrbitsv2(int n, Body *bodies)
{
    for (int i = 0; i < n; i++)
    {
        if (bodies[i].type != BODY_STAR)
        {
            int parentIndex = bodies[i].orbitalBodyIndex;
            Body *parent = &bodies[parentIndex];

            // Calculate orbital radius from period
            bodies[i].orbitalRadius = calculateOrbitalRadius(bodies[i].orbitalPeriod, parent->mass);

            // Set initial position (along x-axis for simplicity)
            Vector2 pos = {bodies[i].orbitalRadius, 0};
            bodies[i].position = _Vector2Add(&parent->position, &pos);

            // Calculate orbital velocity
            float orbitalVelocity = calculateOrbitalVelocity(parent->mass, bodies[i].orbitalRadius);
            bodies[i].velocity = (Vector2){0, orbitalVelocity}; // Perpendicular to radius
            bodies[i].velocity = _Vector2Add(&parent->velocity, &bodies[i].velocity);
        }
    }
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
    float padding = 10.0f;
    minX -= padding;
    maxX += padding;
    minY -= padding;
    maxY += padding;
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
                body->position = (Vector2){0, -10000}; // Arbitrary reset position
                body->velocity = (Vector2){0.8f, 0};
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

float calculateOrbitalSpeed(float mass, float radius)
{
    return (float){sqrtf((G * mass) / radius)};
}

CelestialBody **initBodies(int *numBodies)
{
    *numBodies = 5;
    CelestialBody **bodies = malloc(sizeof(CelestialBody *) * (*numBodies));

    // Maximum 64-bit number is 1.8e19
    // Heaviest object in the universe is the supermassive black hole at the centre - 1e18
    // The supermassive black hole is 1 million solar masses of Sol
    // All other masses are derived from Sol's mass

    // Simulating a supermassive black hole at the centre of the universe
    bodies[0] = malloc(sizeof(CelestialBody));
    *bodies[0] = (CelestialBody){.type = TYPE_UNIVERSE,
                                 .name = strdup("Centre of Universe"),
                                 .position = {0, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e18f,
                                 .radius = 1.0f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f};

    // Star - orbits the universe centre in 28 days (28 * 24 * 60 * 60 seconds)
    float orbitalRadius = calculateOrbitalRadius(365 * 24 * 60 * 60, bodies[0]->mass);
    bodies[1] = malloc(sizeof(CelestialBody));
    *bodies[1] = (CelestialBody){.type = TYPE_STAR,
                                 .name = strdup("Sol"),
                                 .position = {orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e15f,
                                 .radius = 3e4,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f};
    float orbitalSpeed = calculateOrbitalSpeed(bodies[0]->mass, orbitalRadius);
    bodies[1]->velocity = (Vector2){0, orbitalSpeed};

    // Planet orbiting Star in 24 hours (24 * 60 * 60 seconds)
    orbitalRadius = calculateOrbitalRadius(365 * 24 * 60 * 60, bodies[1]->mass);
    bodies[2] = malloc(sizeof(CelestialBody));
    *bodies[2] = (CelestialBody){.type = TYPE_PLANET,
                                 .name = strdup("Planet1"),
                                 .position = {bodies[1]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e9,
                                 .radius = 3e2f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f};
    Vector2 relVel = (Vector2){0, -calculateOrbitalSpeed(bodies[1]->mass, orbitalRadius)};
    bodies[2]->velocity = Vector2Add(bodies[1]->velocity, relVel);

    // Moon orbiting Planet in 1.5 hour (1.5 * 60 * 60 seconds)
    orbitalRadius = calculateOrbitalRadius(28 * 24 * 60 * 60, bodies[2]->mass);
    bodies[3] = malloc(sizeof(CelestialBody));
    *bodies[3] = (CelestialBody){.type = TYPE_MOON,
                                 .name = strdup("Moon1"),
                                 .position = {bodies[2]->position.x + orbitalRadius, 0},
                                 .velocity = {0, 0},
                                 .mass = 1e7,
                                 .radius = 1e2f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f};
    relVel = (Vector2){0, calculateOrbitalSpeed(bodies[2]->mass, orbitalRadius)};
    bodies[3]->velocity = Vector2Add(bodies[2]->velocity, relVel);

    // // Star
    // bodies[3] = malloc(sizeof(CelestialBody));
    // *bodies[3] = (CelestialBody){.type = TYPE_STAR,
    //                              .name = strdup("Star2"),
    //                              .position = {-20000, 0},
    //                              .velocity = {0, 0},
    //                              .mass = 1e14,
    //                              .radius = 100.0f,
    //                              .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
    //                              .rotation = 0.0f};
    // orbitalSpeed = calculateOrbitalSpeed(bodies[0]->mass, 20000);
    // bodies[3]->velocity = (Vector2){0, -orbitalSpeed};

    // // Planet orbiting Star
    // bodies[4] = malloc(sizeof(CelestialBody));
    // *bodies[4] = (CelestialBody){.type = TYPE_PLANET,
    //                              .name = strdup("Planet2"),
    //                              .position = {-20000, 400},
    //                              .velocity = {0, 0},
    //                              .mass = 1e9,
    //                              .radius = 30.0f,
    //                              .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
    //                              .rotation = 0.0f};
    // relVel = (Vector2){-calculateOrbitalSpeed(bodies[3]->mass, 400), 0};
    // bodies[4]->velocity = Vector2Add(bodies[3]->velocity, relVel);

    // // Ship
    bodies[4] = malloc(sizeof(CelestialBody));
    *bodies[4] = (CelestialBody){.type = TYPE_SHIP,
                                 .name = strdup("Ship"),
                                 .position = {0, -10000},
                                 .velocity = {10.0f, 0},
                                 .mass = 1e1f,
                                 .radius = 10.0f,
                                 .previousPositions = (Vector2 *)malloc(sizeof(Vector2) * PREVIOUS_POSITIONS),
                                 .rotation = 0.0f,
                                 .shipSettings = (ShipSettings){
                                     .thrust = 1e0f,
                                     .fuel = 100.0f,
                                     .fuelConsumption = 0.0f,
                                     .isSelected = true}};
    orbitalSpeed = calculateOrbitalSpeed(bodies[4]->mass, 10000);
    bodies[4]->velocity = (Vector2){orbitalSpeed, 0};

    return bodies;
}

void drawBodies(CelestialBody **bodies, int numBodies)
{
    for (int i = 0; i < numBodies; i++)
    {
        Color color = (bodies[i]->type == TYPE_UNIVERSE) ? BLACK
                      : (bodies[i]->type == TYPE_STAR)   ? YELLOW
                      : (bodies[i]->type == TYPE_PLANET) ? BLUE
                      : (bodies[i]->type == TYPE_MOON)   ? GRAY
                                                         : RED;
        DrawCircleV(bodies[i]->position, bodies[i]->radius, color);
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
            if (bodies[i]->type != TYPE_UNIVERSE)
                DrawPixelV(bodies[i]->previousPositions[j], (Color){255, 255, 255, 255});
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

void drawCelestialGrid(float zoomLevel, int numQuadrants)
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