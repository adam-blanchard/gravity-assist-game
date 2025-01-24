#include "raylib.h"
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Structure to represent celestial bodies
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float radius;
    Color renderColour;
} Body;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    Vector2 heading;
    float thrustForce;
    float mass;
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

float calculateOrbitalVelocity(float gravitationalConstant, Body *b, float radius)
{
    float orbitalVelocity = sqrtf((gravitationalConstant * b->mass) / radius);
    return (float){orbitalVelocity};
}

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    float G = 6.67430e-11;

    InitWindow(screenWidth, screenHeight, "2D Solar System Simulation");
    SetTargetFPS(60);

    // Initialize bodies
    Body sun = {
        {screenWidth / 2, screenHeight / 2}, // Position at center
        {0, 0},                              // Sun doesn't move
        2e16,                                // Mass of the sun (scaled)
        20,                                  // Radius for visualization
        YELLOW};

    const int numPlanets = 1;
    const int trailSize = 1000;
    Body planets[numPlanets] = {
        {{screenWidth / 2 + 200, screenHeight / 2},
         {0, calculateOrbitalVelocity(G, &sun, 200)},
         8e10,
         7,
         BLUE}};

    Ship ship = {
        {screenWidth / 2, screenHeight / 2 - 100},
        {0, calculateOrbitalVelocity(G, &sun, 100)},
        {0, 1},
        20.0f,
        1e6};

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        // Update physics
        for (int m = 0; m < numPlanets; m++)
        {
            Vector2 sunForce = gravitationalForce(&planets[m], &sun, G); // Force from the Sun's gravity

            Vector2 force = sunForce;

            for (int n = 0; n < numPlanets; n++)
            {
                if (m != n)
                {
                    Vector2 planetaryForce = gravitationalForce(&planets[m], &planets[n], G);
                    force = _Vector2Add(&force, &planetaryForce);
                }
            }

            Vector2 acceleration = {force.x / planets[m].mass, force.y / planets[m].mass};

            planets[m].velocity.x += acceleration.x * dt;
            planets[m].velocity.y += acceleration.y * dt;

            planets[m].position.x += planets[m].velocity.x * dt;
            planets[m].position.y += planets[m].velocity.y * dt;
        }

        // Ship controls
        // A and D rotate left and right respectively
        // W is thrust

        // Use thrust force to calculate acceleration
        // Use ship acceleration to calculate velocity and position

        BeginDrawing();
        ClearBackground(BLACK);

        // Plot trails
        // TODO: Allow different length trails
        static Vector2 lastPositions[numPlanets][trailSize] = {0}; // Initialise an array of 2D vectors to store past positions
        static int trailIndex = 0;
        for (int i = 0; i < numPlanets; i++)
        {
            lastPositions[i][trailIndex] = planets[i].position; // Populate each position each frame until array is filled, overwrite positions after max exceeded
        }
        trailIndex = (trailIndex + 1) % trailSize;
        for (int m = 0; m < numPlanets; m++)
        {
            for (int n = 0; n < trailSize; n++)
            {
                if (lastPositions[m][n].x != 0 || lastPositions[m][n].y != 0)
                {
                    DrawPixelV(lastPositions[m][n], (Color){255, 255, 255, 50});
                }
            }
        }

        // Draw Sun
        DrawCircle(sun.position.x, sun.position.y, sun.radius, YELLOW);

        // Draw Planets
        for (int i = 0; i < numPlanets; i++)
        {
            DrawCircle(planets[i].position.x, planets[i].position.y, planets[i].radius, planets[i].renderColour);
        }

        DrawLine(ship.position.x, ship.position.y - 3, ship.position.x, ship.position.y + 3, WHITE);

        // GUI
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}