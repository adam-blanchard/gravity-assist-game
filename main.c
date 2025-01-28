#include "raylib.h"
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif
#define TRAJECTORY_STEPS 100
#define TRAJECTORY_STEP_TIME 0.1f

// Structure to represent celestial bodies
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float radius;
    Color renderColour;
    float rotation;
    Vector2 *trajectory;
    int trajectorySteps;
} Body;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float rotation;
    float thrust;
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
Vector2 gravitationalForce(Body *b1, Body *b2, float gravitationalConstant)
{
    Vector2 direction = _Vector2Subtract(&b2->position, &b1->position);
    float distance = _Vector2Length(direction);
    if (distance == 0)
        return (Vector2){0, 0}; // Avoid division by zero

    float forceMagnitude = gravitationalConstant * b1->mass * b2->mass / (distance * distance);
    direction = _Vector2Normalize(direction);
    return _Vector2Scale(direction, forceMagnitude);
}

Vector2 gravitationalForce2(Vector2 *pos1, Vector2 *pos2, float mass1, float mass2, float gravitationalConstant)
{
    Vector2 direction = _Vector2Subtract(pos2, pos1);
    float distance = _Vector2Length(direction);
    if (distance == 0)
        return (Vector2){0, 0}; // Avoid division by zero

    float forceMagnitude = gravitationalConstant * mass1 * mass2 / (distance * distance);
    direction = _Vector2Normalize(direction);
    return _Vector2Scale(direction, forceMagnitude);
}

float calculateOrbitalVelocity(float gravitationalConstant, float mass, float radius)
{
    float orbitalVelocity = sqrtf((gravitationalConstant * mass) / radius);
    return (float){orbitalVelocity};
}

float calculateOrbitCircumference(float r)
{
    return (float){2 * PI * r};
}

float calculateEscapeVelocity(float gravitationalConstant, float mass, float radius)
{
    float escapeVelocity = sqrtf((2 * gravitationalConstant * mass) / radius);
    return (float){escapeVelocity};
}

void updateBodies(int n, Body *bodies, float gravitationalConstant, float dt)
{
    for (int m = 0; m < n; m++)
    {
        Vector2 force = {0};

        for (int i = 0; i < n; i++)
        {
            if (m != i)
            {
                Vector2 planetaryForce = gravitationalForce(&bodies[m], &bodies[i], gravitationalConstant);
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

void updateShip(Ship *ship, int n, Body *bodies, float gravitationalConstant, float dt)
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

    Vector2 scaledShipVelocity = _Vector2Scale(ship->velocity, dt);

    // Update position
    ship->position = _Vector2Add(&ship->position, &scaledShipVelocity);

    Vector2 force = {0};

    for (int i = 0; i < n; i++)
    {
        Vector2 planetaryForce = gravitationalForce2(&ship->position, &bodies[i].position, ship->mass, bodies[i].mass, gravitationalConstant);
        force = _Vector2Add(&force, &planetaryForce);
    }

    Vector2 acceleration = {force.x / ship->mass, force.y / ship->mass};

    ship->velocity.x += acceleration.x * dt;
    ship->velocity.y += acceleration.y * dt;

    ship->position.x += ship->velocity.x * dt;
    ship->position.y += ship->velocity.y * dt;
}

float calculateAngle(Vector2 *planetPosition, Vector2 *sunPosition)
{
    Vector2 relativePosition = _Vector2Subtract(planetPosition, sunPosition);
    float angleInRadians = atan2f(relativePosition.y, relativePosition.x);
    float angleInDegrees = angleInRadians * (180.0f / PI);

    // Normalize to 0-360 degrees
    return fmod(angleInDegrees + 360.0f, 360.0f);
}

void initialiseStableOrbits(int n, Body *bodies, float gravitationalConstant)
{
    for (int i = 1; i < n; i++)
    {
        float xDist = bodies[i].position.x - bodies[0].position.x;
        float yDist = bodies[i].position.y - bodies[0].position.y;
        float radius = sqrt(xDist * xDist + yDist * yDist);

        float orbitalVelocity = calculateOrbitalVelocity(gravitationalConstant, bodies[0].mass, radius);
        float currentAngle = calculateAngle(&bodies[i].position, &bodies[0].position);

        bodies[i].velocity.y = orbitalVelocity * cosf(currentAngle);
        bodies[i].velocity.x = orbitalVelocity * sinf(currentAngle);
    }
}

void predictBodyTrajectory(Body *body, Body *otherBodies, int numBodies, Vector2 *trajectory, float gravitationalConstant)
{
    Vector2 currentPosition = body->position;
    Vector2 currentVelocity = body->velocity;

    for (int i = 0; i < TRAJECTORY_STEPS; ++i)
    {
        Vector2 totalForce = {0, 0};
        for (int j = 0; j < numBodies; ++j)
        {
            if (&otherBodies[j] != body)
            { // Don't calculate force with itself
                Vector2 force = gravitationalForce(body, &otherBodies[j], gravitationalConstant);
                totalForce = _Vector2Add(&totalForce, &force);
            }
        }

        Vector2 acceleration = {totalForce.x / body->mass, totalForce.y / body->mass};
        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

int main(void)
{
    int screenWidth = 1280;
    int screenHeight = 720;

    int wMid = screenWidth / 2;
    int hMid = screenHeight / 2;

    float G = 6.67430e-11;

    InitWindow(screenWidth, screenHeight, "2D Solar System Simulation");
    SetTargetFPS(60);

    Ship playerShip = {
        {wMid, hMid - 100},
        {75, 0},
        1e6,
        0.0f,
        20.0f};

    // 0th index in system is the star
    Body solSystem[2] = {
        {{wMid, hMid}, {0, 0}, 2e16, 20, YELLOW, 0.0f},
        {{wMid + 200, hMid}, {0, 60}, 8e10, 7, BLUE, 0.0f}};

    int solSystemBodies = sizeof(solSystem) / sizeof(Body);

    initialiseStableOrbits(solSystemBodies, solSystem, G);

    const int trailSize = 1000;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        updateBodies(solSystemBodies, solSystem, G, dt);

        updateShip(&playerShip, solSystemBodies, solSystem, G, dt);

        for (int i = 0; i < solSystemBodies; i++)
        {
            solSystem[i].trajectory = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
            predictBodyTrajectory(&solSystem[i], solSystem, solSystemBodies, solSystem[i].trajectory, G);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // Plot trails
        // TODO: Allow different length trails
        Vector2 lastPositions[2][trailSize] = {0}; // Initialise an array of 2D vectors to store past positions
        static int trailIndex = 0;
        for (int i = 1; i < solSystemBodies; i++)
        {
            lastPositions[i][trailIndex] = solSystem[i].position; // Populate each position each frame until array is filled, overwrite positions after max exceeded
        }
        trailIndex = (trailIndex + 1) % trailSize;
        for (int m = 1; m < solSystemBodies; m++)
        {
            for (int n = 0; n < trailSize; n++)
            {
                DrawPixelV(lastPositions[m][n], (Color){255, 255, 255, 50});
            }
        }

        // Draw celestial bodies
        for (int i = 0; i < solSystemBodies; i++)
        {
            DrawCircle(solSystem[i].position.x, solSystem[i].position.y, solSystem[i].radius, solSystem[i].renderColour);
        }

        // Draw Ship
        Vector2 shipSize = {8, 16}; // Width, Height of ship triangle
        Vector2 shipPoints[3] = {
            {0, -shipSize.y / 2},
            {-shipSize.x / 2, shipSize.y / 2},
            {shipSize.x / 2, shipSize.y / 2}};

        // Rotate ship points
        for (int i = 0; i < 3; i++)
        {
            float radians = playerShip.rotation * PI / 180.0f;
            float x = shipPoints[i].x * cosf(radians) - shipPoints[i].y * sinf(radians);
            float y = shipPoints[i].x * sinf(radians) + shipPoints[i].y * cosf(radians);
            shipPoints[i] = (Vector2){x, y};
            shipPoints[i] = _Vector2Add(&shipPoints[i], &playerShip.position);
        }
        DrawTriangle(shipPoints[0], shipPoints[1], shipPoints[2], WHITE);

        for (int i = 0; i < solSystemBodies; i++)
        {
            for (int j = 0; j < TRAJECTORY_STEPS - 1; j++)
            {
                DrawLineV(solSystem[i].trajectory[j], solSystem[i].trajectory[j + 1], (Color){255, 255, 255, 50});
            }
        }

        // GUI
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawFPS(screenWidth - 100, 0);

        EndDrawing();
    }

    for (int i = 0; i < solSystemBodies; i++)
    {
        free(solSystem[i].trajectory);
    }
    CloseWindow();
    return 0;
}