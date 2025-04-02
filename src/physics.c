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

float calculateRelativeSpeed(ship_t *ship, celestialbody_t *body, float gameTime)
{
    Vector2 bodyVelocity = calculateBodyVelocity(body, gameTime);
    Vector2 relativeVelocity = Vector2Subtract(ship->velocity, bodyVelocity);
    return sqrtf((relativeVelocity.x * relativeVelocity.x) + (relativeVelocity.y * relativeVelocity.y));
}

float calculateOrbitalSpeed(float mass, float radius)
{
    return (float){sqrtf((G * mass) / radius)};
}

void updateShipPositions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float dt)
{
    for (int i = 0; i < numShips; i++)
    {
        Vector2 totalForce;
        Vector2 gravityForce = computeShipGravity(ships[i], bodies, numBodies);
        Vector2 dragForce = calculateDragForce(ships[i], bodies, numBodies);
        totalForce = Vector2Add(gravityForce, dragForce);
        Vector2 accel = Vector2Scale(totalForce, 1.0f / ships[i]->mass);
        ships[i]->velocity = Vector2Add(ships[i]->velocity, Vector2Scale(accel, dt));
        ships[i]->position = Vector2Add(ships[i]->position, Vector2Scale(ships[i]->velocity, dt));
    }
}

// Update celestial body positions (on rails)
void updateCelestialPositions(celestialbody_t **bodies, int numBodies, float time)
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

void updateLandedShipPosition(ship_t **ships, int numShips, float gameTime)
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

bool detectShipBodyCollision(ship_t *ship, celestialbody_t *body)
{
    float dist = Vector2Distance(ship->position, body->position);
    if (dist < (ship->radius + body->radius))
        return true;
    return false;
}

bool detectShipAtmosphereCollision(ship_t *ship, celestialbody_t *body)
{
    float dist = Vector2Distance(ship->position, body->position);
    if (dist < (ship->radius + body->atmosphereRadius) && dist > (body->radius))
        return true;
    return false;
}

void detectCollisions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float gameTime)
{
    for (int i = 0; i < numShips; i++)
    {
        for (int j = 0; j < numBodies; j++)
        {
            if (detectShipBodyCollision(ships[i], bodies[j]))
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

Vector2 computeShipGravity(ship_t *ship, celestialbody_t **bodies, int numBodies)
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

Vector2 calculateDragForce(ship_t *ship, celestialbody_t **bodies, int numBodies)
{
    Vector2 velocity = ship->velocity;
    float speed = Vector2Length(velocity);

    Vector2 normalVelocity = Vector2Normalize(velocity);

    Vector2 dragDirection = {-normalVelocity.x, -normalVelocity.y};

    float dragMagnitude;
    Vector2 dragForce = {0, 0};

    for (int i = 0; i < numBodies; i++)
    {
        if (bodies[i]->atmosphereDrag <= 0)
            continue;
        if (detectShipAtmosphereCollision(ship, bodies[i]))
        {
            dragMagnitude = speed * speed * bodies[i]->atmosphereDrag;
            dragForce = Vector2Scale(dragDirection, dragMagnitude);
            break;
        }
    }

    return dragForce;
}

void calculateShipFuturePositions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float gameTime)
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
                Vector2 gravityForce = computeShipGravity(ships[j], bodies, numBodies);
                Vector2 dragForce = calculateDragForce(ships[j], bodies, numBodies);
                Vector2 totalForce = Vector2Add(gravityForce, dragForce);
                Vector2 accel = Vector2Scale(totalForce, 1.0f / ships[j]->mass);
                ships[j]->velocity = Vector2Add(ships[j]->velocity, Vector2Scale(accel, FUTURE_STEP_TIME));
                ships[j]->position = Vector2Add(ships[j]->position, Vector2Scale(ships[j]->velocity, FUTURE_STEP_TIME));

                // Check for collision
                celestialbody_t *collidingBody = NULL;
                Vector2 collisionPosition = ships[j]->position;
                for (int k = 0; k < numBodies; k++)
                {
                    if (detectShipBodyCollision(ships[j], bodies[k]))
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
                    // printf("Ship %d collides with %s at step %d\n", j, collidingBody->name, i);
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

void landShip(ship_t *ship, celestialbody_t *body, float gameTime)
{
    if (ship->state == SHIP_LANDED)
        return;

    // Set landing state
    ship->state = SHIP_LANDED;
    ship->landedBody = body;
    ship->velocity = calculateBodyVelocity(body, gameTime); // Match velocity to the body

    Vector2 direction = Vector2Subtract(ship->position, body->position);
    // float distance = Vector2Length(direction);
    direction = Vector2Normalize(direction);

    // Position ship on the surface and store landing position
    Vector2 surfacePosition = Vector2Scale(direction, body->radius + ship->radius);
    ship->position = Vector2Add(body->position, surfacePosition);
    ship->landingPosition = surfacePosition; // Store relative position
}

void takeoffShip(ship_t *ship)
{
    if (ship->state != SHIP_LANDED || ship->landedBody == NULL)
        return;

    ship->state = SHIP_FLYING;
    ship->landedBody = NULL;
}

Vector2 calculateBodyVelocity(celestialbody_t *body, float gameTime)
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