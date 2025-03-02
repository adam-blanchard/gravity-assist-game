#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif
#ifndef G
#define G 6.67430e-11
#endif
#define TRAJECTORY_STEPS 6000
#define TRAJECTORY_STEP_TIME 0.033f

typedef enum
{
    SHIP_FLYING,
    SHIP_LANDED
} ShipState;

typedef enum
{
    TYPE_UNIVERSE, // Root of the hierarchy
    TYPE_GALAXY,   // Collection of star systems
    TYPE_STAR,     // Orbits the centre of a galaxy
    TYPE_PLANET,   // Orbits a star
    TYPE_MOON      // Orbits a planet
} CelestialType;

typedef enum
{
    BODY_STAR,
    BODY_PLANET,
    BODY_MOON,
    BODY_ASTEROID
} BodyType;

typedef struct
{
    float radius;       // Distance from parent (e.g., orbit radius)
    float angularSpeed; // Rotation speed in radians per second
    float initialAngle; // Starting angle in radians
} Orbit;

typedef struct CelestialBody
{
    CelestialType type;
    char *name;
    Vector2 position;
    Vector2 localPosition;
    Orbit orbit;
    struct CelestialBody *parent;
    struct CelestialBody **children;
    int childCount;
    int childCapacity;
    float mass; // Mass for gravity calculations
} CelestialBody;

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
    float r3 = (period * period * G * mStar) / 4 * PI * PI;
    return (float){cbrtf(r3)};
}

void initialiseOrbits(int n, Body *bodies)
{
    // Planets orbit stars
    // Moons orbit planets, which orbit stars
    for (int i = 0; i < n; i++)
    {
        if (bodies[i].type != BODY_STAR)
        {
            int orbitalBodyIndex = bodies[i].orbitalBodyIndex;
            float radius = calculateOrbitalRadius(bodies[i].orbitalPeriod, bodies[orbitalBodyIndex].mass);

            bodies[i].position.x = radius;
            bodies[i].position.y = 0;

            float orbitalVelocity = calculateOrbitalVelocity(bodies[orbitalBodyIndex].mass, radius);
            float currentAngle = calculateAngle(&bodies[i].position, &bodies[orbitalBodyIndex].position);

            bodies[i].velocity.y = orbitalVelocity * cosf(currentAngle);
            bodies[i].velocity.x = orbitalVelocity * sinf(currentAngle);

            if (bodies[i].type == BODY_MOON)
            {
                bodies[i].position.x = bodies[orbitalBodyIndex].position.x + radius;
                bodies[i].velocity = _Vector2Add(&bodies[orbitalBodyIndex].velocity, &bodies[i].velocity);
            }
        }
    }
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

bool checkCollision(Ship *playerShip, Body *planetaryBody)
{
    if (CheckCollisionCircles(playerShip->position, playerShip->colliderRadius, planetaryBody->position, planetaryBody->radius))
    {
        return 1;
    }
    return 0;
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

void updateBodiesv2(int n, Body *bodies, float dt)
{
    for (int m = 0; m < n; m++)
    {
        if (bodies[m].type == BODY_STAR)
        {
            // Star remains stationary for now
            continue;
        }

        int parentIndex = bodies[m].orbitalBodyIndex;
        Body *parent = &bodies[parentIndex];

        // Current relative position
        Vector2 rel_pos = _Vector2Subtract(&bodies[m].position, &parent->position);
        float currentRadius = _Vector2Length(rel_pos);
        float currentAngle = atan2f(rel_pos.y, rel_pos.x);

        // Orbital velocity and angular speed
        float orbitalVelocity = calculateOrbitalVelocity(parent->mass, currentRadius);
        float angularVelocity = orbitalVelocity / currentRadius;

        // Update angle
        currentAngle += angularVelocity * dt;

        // Update position
        rel_pos.x = bodies[m].orbitalRadius * cosf(currentAngle);
        rel_pos.y = bodies[m].orbitalRadius * sinf(currentAngle);
        bodies[m].position = _Vector2Add(&parent->position, &rel_pos);

        // Update velocity (tangential to orbit)
        bodies[m].velocity.x = orbitalVelocity * sinf(currentAngle);
        bodies[m].velocity.y = orbitalVelocity * cosf(currentAngle);
        bodies[m].velocity = _Vector2Add(&parent->velocity, &bodies[m].velocity);
    }
}

CelestialBody *createCelestialBody(CelestialType type, const char *name, Vector2 localPos, Orbit orbit, float mass)
{
    CelestialBody *body = (CelestialBody *)malloc(sizeof(CelestialBody));
    body->type = type;
    body->name = strdup(name); // Duplicate string to avoid pointer issues
    body->localPosition = localPos;
    body->orbit = orbit;
    body->parent = NULL;
    body->mass = mass;
    body->children = NULL;
    body->childCount = 0;
    body->childCapacity = 0;
    return body;
}

void addChild(CelestialBody *parent, CelestialBody *child)
{
    if (parent->childCount == parent->childCapacity)
    {
        int newCapacity = parent->childCapacity == 0 ? 4 : parent->childCapacity * 2;
        CelestialBody **newChildren = (CelestialBody **)realloc(parent->children, sizeof(CelestialBody *) * newCapacity);
        if (!newChildren)
        {
            printf("Failed to allocate memory for children\n");
            exit(1);
        }
        parent->children = newChildren;
        parent->childCapacity = newCapacity;
    }
    parent->children[parent->childCount] = child;
    parent->childCount++;
    child->parent = parent;
}

void updatePositions(CelestialBody *body, float time)
{
    if (body->parent == NULL)
    {
        // Root (universe) is fixed at origin
        body->position = (Vector2){0, 0};
    }
    else if (body->type == TYPE_GALAXY || body->type == TYPE_STAR || body->type == TYPE_PLANET || body->type == TYPE_MOON)
    {
        // Orbiting bodies use circular orbit calculations
        float angle = body->orbit.initialAngle + body->orbit.angularSpeed * time;
        Vector2 orbitalPos = {
            body->orbit.radius * cosf(angle),
            body->orbit.radius * sinf(angle)};
        body->position = Vector2Add(body->parent->position, orbitalPos);
    }
    else
    {
        // Static bodies (stars) use local position offset
        body->position = Vector2Add(body->parent->position, body->localPosition);
    }
    // Recursively update children
    for (int i = 0; i < body->childCount; i++)
    {
        updatePositions(body->children[i], time);
    }
}

Vector2 calculateGravity(Ship *ship, CelestialBody *body)
{
    Vector2 direction = Vector2Subtract(body->position, ship->position);
    float distance = Vector2Length(direction);
    if (distance < 1.0f)
        distance = 1.0f; // Prevent division by zero
    float forceMagnitude = (G * body->mass) / (distance * distance);
    return Vector2Scale(Vector2Normalize(direction), forceMagnitude);
}

// Recursive function to apply gravity from all bodies
void applyGravity(CelestialBody *body, Ship *ship, Vector2 *totalAcceleration)
{
    if (body->type != TYPE_UNIVERSE)
    { // Skip universe itself
        Vector2 acc = calculateGravity(ship, body);
        *totalAcceleration = Vector2Add(*totalAcceleration, acc);
    }
    for (int i = 0; i < body->childCount; i++)
    {
        applyGravity(body->children[i], ship, totalAcceleration);
    }
}

// void updateShip(Ship *ship, CelestialBody *galaxy, float deltaTime)
// {
//     Vector2 totalAcceleration = {0, 0};

//     // Accumulate sum of all accelerations
//     applyGravity(galaxy, ship, &totalAcceleration);

//     // Update velocity and position
//     ship->velocity = Vector2Add(ship->velocity, Vector2Scale(totalAcceleration, deltaTime));
//     ship->position = Vector2Add(ship->position, Vector2Scale(ship->velocity, deltaTime));
// }

void renderCelestialBody(CelestialBody *body)
{
    switch (body->type)
    {
    case TYPE_UNIVERSE:
        // Optional: Draw a faint background (not implemented here)
        break;
    case TYPE_GALAXY:
        DrawCircleV(body->position, 20, DARKGRAY);
        break;
    case TYPE_STAR:
        DrawCircleV(body->position, 10, YELLOW); // Larger yellow circle
        break;
    case TYPE_PLANET:
        DrawCircleV(body->position, 5, BLUE); // Medium blue circle
        break;
    case TYPE_MOON:
        DrawCircleV(body->position, 2, WHITE); // Small gray circle
        break;
    }
    // Render all children
    for (int i = 0; i < body->childCount; i++)
    {
        renderCelestialBody(body->children[i]);
    }
}

void renderShip(Ship *ship)
{
    DrawCircleV(ship->position, 3, RED); // Small red circle
}

void freeCelestialBody(CelestialBody *body)
{
    if (body == NULL)
        return;
    free(body->name);
    for (int i = 0; i < body->childCount; i++)
    {
        freeCelestialBody(body->children[i]);
    }
    free(body->children);
    free(body);
}