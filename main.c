#include "raylib.h"
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif
#define SCALE_FACTOR 1e-6
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

float calculateDistance(Vector2 *pos1, Vector2 *pos2)
{
    float xDist = pos1->x - pos2->x;
    float yDist = pos1->y - pos2->y;
    float radius = sqrt(xDist * xDist + yDist * yDist);
    return (float)(radius);
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

    Vector2 force = {0};

    for (int i = 0; i < n; i++)
    {
        Vector2 planetaryForce = gravitationalForce2(&ship->position, &bodies[i].position, ship->mass, bodies[i].mass, gravitationalConstant);
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

void initialiseStableOrbits(int n, Body *bodies, float gravitationalConstant)
{
    for (int i = 1; i < n; i++)
    {
        float radius = calculateDistance(&bodies[i].position, &bodies[0].position);

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

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 totalForce = {0};
        for (int j = 0; j < numBodies; j++)
        {
            if (&otherBodies[j] != body)
            {
                Vector2 force = gravitationalForce2(&currentPosition, &otherBodies[j].position, body->mass, otherBodies[j].mass, gravitationalConstant);
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

void predictShipTrajectory(Ship *playerShip, Body *otherBodies, int numBodies, Vector2 *trajectory, float gravitationalConstant)
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
            Vector2 planetaryForce = gravitationalForce2(&currentPosition, &otherBodies[j].position, playerShip->mass, otherBodies[j].mass, gravitationalConstant);
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

void predictTrajectory(Vector2 *initialPosition, Vector2 *initialVelocity, float *mass, Body *otherBodies, int numBodies, Vector2 *trajectory, float gravitationalConstant)
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
                Vector2 force = gravitationalForce2(&currentPosition, &otherBodies[j].position, *mass, otherBodies[j].mass, gravitationalConstant);
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

void predictTimesteps(Ship *playerShip, Body *bodies, int numBodies, float gravitationalConstant)
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
                    // Vector2 planetaryForce = gravitationalForce(&bodies[j], &bodies[k], gravitationalConstant);
                    Vector2 planetaryForce = gravitationalForce2(&body1Position, &body2Position, bodies[j].mass, bodies[k].mass, gravitationalConstant);
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

float calculateOrbitalRadius(float period, float mStar, float mPlanet, float gravitationalConstant)
{
    float r3 = (period * period * gravitationalConstant * (mStar + mPlanet)) / 4 * PI * PI;
    return (float){cbrtf(r3)};
}

void initialiseOrbits(int n, Body *bodies, float gravitationalConstant)
{
    for (int i = 1; i < n; i++)
    {

        // float radius = calculateDistance(&bodies[i].position, &bodies[0].position);

        float radius = calculateOrbitalRadius(360.0f, bodies[0].mass, bodies[i].mass, gravitationalConstant);

        bodies[i].position.x = radius;

        float orbitalVelocity = calculateOrbitalVelocity(gravitationalConstant, bodies[0].mass, radius);
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

int main(void)
{
    int screenWidth = 1280;
    int screenHeight = 720;

    int wMid = screenWidth / 2;
    int hMid = screenHeight / 2;

    float G = 6.67430e-11;

    int targetFPS = 60;

    bool enableTrajectories = true;

    InitWindow(screenWidth, screenHeight, "Gravity Assist");
    SetTargetFPS(targetFPS);

    Camera2D camera = {0};
    camera.rotation = 0.0f; // Camera rotation in degrees
    camera.zoom = 0.1f;     // Camera zoom (1.0 is normal size)

    Ship playerShip = {
        {0, -3e3},
        {2e3, 0},
        1e3,
        0.0f,
        2e2f};

    // sol system based on earth's home solar system
    // All body masses and radiuses scaled down by a factor of

    // 0th index in system is the star
    Body solSystem[2] = {
        {{0, 0}, // Position
         {0, 0}, // Velocity
         2e20,   // Mass (kg)
         500,    // Radius
         YELLOW, // Colour
         0.0f},  // Rotation
        {{0, 0},
         {0, 0},
         2e16,
         50,
         BLUE,
         0.0f}};

    int solSystemBodies = sizeof(solSystem) / sizeof(Body);
    int cameraLock = 0;
    Vector2 *cameraLockPosition = {0};

    cameraLockPosition = &solSystem[cameraLock].position;
    camera.target = *cameraLockPosition;   // Camera target (where the camera looks at)
    camera.offset = (Vector2){wMid, hMid}; // Offset from camera target

    // initialiseRandomPositions(solSystemBodies, solSystem, screenWidth - 100, screenHeight - 100);
    // initialiseStableOrbits(solSystemBodies, solSystem, G);

    initialiseOrbits(solSystemBodies, solSystem, G);

    const int trailSize = 1000;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // Time step

        updateBodies(solSystemBodies, solSystem, G, dt);

        updateShip(&playerShip, solSystemBodies, solSystem, G, dt);

        if (IsKeyPressed(KEY_T))
        {
            enableTrajectories = !enableTrajectories;
        }

        // Predict trajectories of celestial bodies
        if (enableTrajectories)
        {
            for (int i = 0; i < solSystemBodies; i++)
            {
                solSystem[i].futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
                predictBodyTrajectory(&solSystem[i], solSystem, solSystemBodies, solSystem[i].futurePositions, G);
                // predictTrajectory(&solSystem[i].position, &solSystem[i].velocity, &solSystem[i].mass, solSystem, solSystemBodies, solSystem[i].trajectory, G);
            }

            // Predict ship trajectory
            // TODO: Revise ship trajectory prediction as it is constantly changing
            playerShip.futurePositions = (Vector2 *)malloc(TRAJECTORY_STEPS * sizeof(Vector2));
            predictShipTrajectory(&playerShip, solSystem, solSystemBodies, playerShip.futurePositions, G);
            // predictTrajectory(&playerShip.position, &playerShip.velocity, &playerShip.mass, solSystem, solSystemBodies, playerShip.trajectory, G);
        }

        camera.target = *cameraLockPosition;

        if (IsKeyPressed(KEY_L))
        {
            cameraLock++;
            if (cameraLock >= solSystemBodies)
            {
                cameraLock = -1;
                cameraLockPosition = &playerShip.position;
            }
            else
            {
                cameraLockPosition = &solSystem[cameraLock].position;
            }
        }

        // Camera zoom controls
        camera.zoom += ((float)GetMouseWheelMove() * 0.005f);

        if (IsKeyDown(KEY_R))
            camera.zoom += 0.05f;
        if (IsKeyDown(KEY_F))
            camera.zoom -= 0.05f;
        camera.zoom = _Clamp(camera.zoom, 0.001f, 10.0f); // Limit zoom level

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        if (enableTrajectories)
        {
            // Draw trajectories so they're at the bottom
            for (int i = 0; i < solSystemBodies; i++)
            {
                for (int j = 0; j < TRAJECTORY_STEPS - 1; j++)
                {
                    DrawLineV(solSystem[i].futurePositions[j], solSystem[i].futurePositions[j + 1], (Color){100, 100, 100, 255});
                }
            }

            // Draw ship trajectory
            for (int i = 0; i < TRAJECTORY_STEPS - 1; i++)
            {
                DrawLineV(playerShip.futurePositions[i], playerShip.futurePositions[i + 1], (Color){150, 150, 150, 255});
            }
        }

        // Draw celestial bodies
        for (int i = 0; i < solSystemBodies; i++)
        {
            DrawCircle(solSystem[i].position.x, solSystem[i].position.y, solSystem[i].radius, solSystem[i].renderColour);
        }

        // Draw Ship
        // original ship size was 8x16
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

        EndMode2D();

        // GUI
        DrawText("Press ESC to exit", 10, 10, 20, RAYWHITE);
        DrawText("Press 'T' to toggle trajectories", 10, 40, 20, DARKGRAY);
        DrawText("Press 'L' to switch camera", 10, 70, 20, DARKGRAY);
        DrawFPS(screenWidth - 100, 0);

        EndDrawing();
    }

    if (enableTrajectories)
    {
        for (int i = 0; i < solSystemBodies; i++)
        {
            free(solSystem[i].futurePositions);
        }
        free(playerShip.futurePositions);
    }

    CloseWindow();
    return 0;
}