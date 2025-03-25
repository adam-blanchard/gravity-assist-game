#include "body.h"

float getBodyAngle(CelestialBody *body, float gameTime)
{
    return fmod(body->initialAngle + body->angularSpeed * gameTime, 2 * PI);
}

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
        .angularSpeed = radsPerSecond(365 * 24 * 60), // One second in-game is one minute IRL
        .initialAngle = 0,
        .orbitalRadius = 9e5};

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
        .angularSpeed = radsPerSecond(28 * 24 * 60),
        .initialAngle = 0,
        .orbitalRadius = 2e4};

    return bodies;
}

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