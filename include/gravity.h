#include "raylib.h"
#include <math.h>
#include <stdio.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif
#ifndef G
#define G 6.67430e-11
#endif
#define TRAJECTORY_STEPS 1000
#define TRAJECTORY_STEP_TIME 0.06f

// Structure to represent celestial bodies
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float radius;
    Color renderColour;
    float rotation;
    Vector2 *futurePositions;
    Vector2 *futureVelocities;
    int futureSteps;
} Body;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float rotation;
    float thrust;
    Vector2 *futurePositions;
    Vector2 *futureVelocitites;
    int futureSteps;
} Ship;

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

        bodies[m].position.x += bodies[m].velocity.x * dt;
        bodies[m].position.y += bodies[m].velocity.y * dt;
    }
}

void updateShip(Ship *ship, int n, Body *bodies, float dt)
{
    // Apply rotation
    if (IsKeyDown(KEY_D))
        ship->rotation += 180.0f * dt; // Rotate right
    if (IsKeyDown(KEY_A))
        ship->rotation -= 180.0f * dt; // Rotate left

    // Normalize rotation to keep it within 0-360 degrees
    ship->rotation = fmod(ship->rotation + 360.0f, 360.0f);

    // Apply thrust
    if (IsKeyDown(KEY_W))
    {
        // Convert rotation to radians for vector calculations
        float radians = ship->rotation * PI / 180.0f;
        Vector2 thrustDirection = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
        Vector2 thrust = _Vector2Scale(thrustDirection, ship->thrust * dt);
        ship->velocity = _Vector2Add(&ship->velocity, &thrust);
    }

    Vector2 force = {0};

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

void initialiseStableOrbits(int n, Body *bodies)
{
    for (int i = 1; i < n; i++)
    {
        float radius = calculateDistance(&bodies[i].position, &bodies[0].position);

        float orbitalVelocity = calculateOrbitalVelocity(bodies[0].mass, radius);
        float currentAngle = calculateAngle(&bodies[i].position, &bodies[0].position);

        bodies[i].velocity.y = orbitalVelocity * cosf(currentAngle);
        bodies[i].velocity.x = orbitalVelocity * sinf(currentAngle);
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

float calculateOrbitalRadius(float period, float mStar, float mPlanet)
{
    float r3 = (period * period * G * (mStar + mPlanet)) / 4 * PI * PI;
    return (float){cbrtf(r3)};
}

void initialiseOrbits(int n, Body *bodies)
{
    float orbitalPeriod = 180.0f; // 1 is 10 seconds so 360 represents an orbital period of 1 hour

    for (int i = 1; i < n; i++)
    {

        // float radius = calculateDistance(&bodies[i].position, &bodies[0].position);

        float radius = calculateOrbitalRadius(orbitalPeriod, bodies[0].mass, bodies[i].mass);

        bodies[i].position.x = radius;

        float orbitalVelocity = calculateOrbitalVelocity(bodies[0].mass, radius);
        float currentAngle = calculateAngle(&bodies[i].position, &bodies[0].position);

        bodies[i].velocity.y = orbitalVelocity * cosf(currentAngle);
        bodies[i].velocity.x = orbitalVelocity * sinf(currentAngle);
    }
}

float _Clamp(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}