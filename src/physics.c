#include "physics.h"

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

float calculateRelativeSpeed(Ship *ship, CelestialBody *body, float gameTime)
{
    Vector2 bodyVelocity = calculateBodyVelocity(body, gameTime);
    Vector2 relativeVelocity = Vector2Subtract(ship->velocity, bodyVelocity);
    return sqrtf((relativeVelocity.x * relativeVelocity.x) + (relativeVelocity.y * relativeVelocity.y));
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

float calculateOrbitalSpeed(float mass, float radius)
{
    return (float){sqrtf((G * mass) / radius)};
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

void updateLandedShipPosition(Ship **ships, int numShips, float gameTime)
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

bool detectShipBodyCollision(Ship *ship, CelestialBody *body, float gameTime)
{
    float dist = Vector2Distance(ship->position, body->position);
    if (dist < (ship->radius + body->radius))
        return true;
    return false;
}

void detectCollisions(Ship **ships, int numShips, CelestialBody **bodies, int numBodies, float gameTime)
{
    for (int i = 0; i < numShips; i++)
    {
        for (int j = 0; j < numBodies; j++)
        {
            if (detectShipBodyCollision(ships[i], bodies[j], gameTime))
            {
                printf("Collision between %s and Ship %i\n", bodies[j]->name, i);
                if (ships[i]->state == SHIP_LANDED)
                    continue;
                if (ships[i]->state != SHIP_LANDED)
                {
                    float relVel = calculateRelativeSpeed(ships[i], bodies[j], gameTime);
                    if (relVel <= MAX_LANDING_SPEED)
                    {
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

void calculateShipFuturePositions(Ship **ships, int numShips, CelestialBody **bodies, int numBodies, float gameTime)
{
    Vector2 initialVelocities[numShips];
    Vector2 initialPositions[numShips];
    bool hasCollided[numShips]; // Track collision state for each ship

    // Capture initial state and reset collision flags
    for (int i = 0; i < numShips; i++)
    {
        initialVelocities[i] = ships[i]->velocity;
        initialPositions[i] = ships[i]->position;
        hasCollided[i] = false; // No collisions at start
    }

    // Simulate system forward for FUTURE_POSITIONS timesteps
    for (int i = 0; i < FUTURE_POSITIONS; i++)
    {
        float futureTime = gameTime + (i * FUTURE_STEP_TIME);

        // Update celestial body positions for this timestep
        updateCelestialPositions(bodies, numBodies, futureTime);

        // Update ship positions, but only for non-collided ships
        for (int j = 0; j < numShips; j++)
        {
            if (ships[j]->state == SHIP_FLYING && !hasCollided[j])
            {
                Vector2 force = computeShipGravity(ships[j], bodies, numBodies);
                Vector2 accel = Vector2Scale(force, 1.0f / ships[j]->mass);
                ships[j]->velocity = Vector2Add(ships[j]->velocity, Vector2Scale(accel, FUTURE_STEP_TIME));
                ships[j]->position = Vector2Add(ships[j]->position, Vector2Scale(ships[j]->velocity, FUTURE_STEP_TIME));

                // Check for collision
                CelestialBody *collidingBody = NULL;
                Vector2 collisionPosition = ships[j]->position;
                for (int k = 0; k < numBodies; k++)
                {
                    if (detectShipBodyCollision(ships[j], bodies[k], futureTime))
                    {
                        collidingBody = bodies[k];
                        // Position at surface, not center
                        Vector2 direction = Vector2Normalize(Vector2Subtract(ships[j]->position, bodies[k]->position));
                        collisionPosition = Vector2Add(bodies[k]->position, Vector2Scale(direction, bodies[k]->radius + ships[j]->radius));
                        break;
                    }
                }

                if (collidingBody != NULL)
                {
                    hasCollided[j] = true;
                    ships[j]->futurePositions[i] = collisionPosition;
                    printf("Ship %d collides with %s at step %d\n", j, collidingBody->name, i);
                    // Set all subsequent positions to the collision point
                    for (int k = i + 1; k < FUTURE_POSITIONS; k++)
                    {
                        ships[j]->futurePositions[k] = collisionPosition;
                    }
                }
                else
                {
                    ships[j]->futurePositions[i] = ships[j]->position;
                }
            }
            else if (ships[j]->state == SHIP_FLYING && hasCollided[j])
            {
                // Already collided, keep future positions at collision point
                ships[j]->futurePositions[i] = ships[j]->futurePositions[i - 1];
            }
            else if (ships[j]->state == SHIP_LANDED)
            {
                // Follow landed body's position
                ships[j]->futurePositions[i] = Vector2Add(ships[j]->landedBody->position, ships[j]->landingPosition);
            }
        }
    }

    // Reset to initial state
    updateCelestialPositions(bodies, numBodies, gameTime);
    for (int i = 0; i < numShips; i++)
    {
        ships[i]->velocity = initialVelocities[i];
        ships[i]->position = initialPositions[i];
    }
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