#include "raylib.h"
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define NUM_BODIES 2
#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Structure to represent celestial bodies
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float radius;
    Color renderColour;
    float rotation;
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

void updateBodies(int n, Body *bodies[n], float gravitationalConstant, float dt)
{
    int numBodies = sizeof(&bodies) / sizeof(Body);
    for (int m = 0; m < numBodies; m++)
    {
        Vector2 force = {0};

        for (int i = 0; i < numBodies; i++)
        {
            if (m != i)
            {
                Vector2 planetaryForce = gravitationalForce(&bodies[m], &bodies[i], gravitationalConstant);
                force = _Vector2Add(&force, &planetaryForce);
            }
        }

        Vector2 acceleration = {force.x / bodies[m]->mass, force.y / bodies[m]->mass};

        bodies[m]->velocity.x += acceleration.x * dt;
        bodies[m]->velocity.y += acceleration.y * dt;

        bodies[m]->position.x += bodies[m]->velocity.x * dt;
        bodies[m]->position.y += bodies[m]->velocity.y * dt;
    }
}

void updateShip(Ship *ship, float dt)
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
        {0, 0},
        1e6,
        0.0f,
        10.0f};

    // 0th index in system is the star
    Body solSystem[2] = {
        {{wMid, hMid}, {0, 0}, 2e16, 20, YELLOW, 0.0f},
        {{wMid + 200, hMid}, {0, 20}, 8e10, 7, BLUE, 0.0f}};

    const int trailSize = 1000;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        int solSystemBodies = sizeof(solSystem) / sizeof(Body);
        // updateBodies(solSystemBodies, &solSystem, G, dt);

        for (int m = 0; m < solSystemBodies; m++)
        {
            Vector2 force = {0};

            for (int i = 0; i < solSystemBodies; i++)
            {
                if (m != i)
                {
                    Vector2 planetaryForce = gravitationalForce(&solSystem[m], &solSystem[i], G);
                    force = _Vector2Add(&force, &planetaryForce);
                }
            }

            Vector2 acceleration = {force.x / solSystem[m].mass, force.y / solSystem[m].mass};

            solSystem[m].velocity.x += acceleration.x * dt;
            solSystem[m].velocity.y += acceleration.y * dt;

            solSystem[m].position.x += solSystem[m].velocity.x * dt;
            solSystem[m].position.y += solSystem[m].velocity.y * dt;
        }

        updateShip(&playerShip, dt);

        BeginDrawing();
        ClearBackground(BLACK);

        // Plot trails
        // TODO: Allow different length trails
        // static Vector2 lastPositions[NUM_BODIES][trailSize] = {0}; // Initialise an array of 2D vectors to store past positions
        // static int trailIndex = 0;
        // for (int i = 0; i < NUM_BODIES; i++)
        // {
        //     lastPositions[i][trailIndex] = bodies[i].position; // Populate each position each frame until array is filled, overwrite positions after max exceeded
        // }
        // trailIndex = (trailIndex + 1) % trailSize;
        // for (int m = 0; m < NUM_BODIES; m++)
        // {
        //     for (int n = 0; n < trailSize; n++)
        //     {
        //         if (lastPositions[m][n].x != 0 || lastPositions[m][n].y != 0)
        //         {
        //             DrawPixelV(lastPositions[m][n], (Color){255, 255, 255, 50});
        //         }
        //     }
        // }

        // Draw celestial bodies
        for (int i = 0; i < sizeof(solSystem) / sizeof(Body); i++)
        {
            DrawCircle(solSystem[i].position.x, solSystem[i].position.y, solSystem[i].radius, solSystem[i].renderColour);
        }

        // Draw Ship
        Vector2 shipSize = {10, 20}; // Width, Height of ship triangle
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

        // GUI
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}