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
    *numBodies = 2;
    CelestialBody **bodies = malloc(sizeof(CelestialBody *) * (*numBodies));

    // Planet orbiting Star - Earth
    bodies[0] = malloc(sizeof(CelestialBody));
    *bodies[0] = (CelestialBody){
        .type = TYPE_PLANET,
        .name = strdup("Earth"),
        .position = {0, 0},
        .mass = 5.97e9, // Real val = 5.97e24 kg
        .radius = 6e3,  // Real val = 6.378e3 km
        .rotation = 0.0f,
        .parentBody = NULL,
        .angularSpeed = 0, // Real val = 365.2 Days (365.2 * 24 * 60 * 60 seconds)
        .initialAngle = 0,
        .orbitalRadius = 0 // Real val = 1.496e8 km
    };

    // Moon orbiting Planet
    bodies[1] = malloc(sizeof(CelestialBody));
    *bodies[1] = (CelestialBody){
        .type = TYPE_MOON,
        .name = strdup("Earth's Moon"),
        .position = {0, 0},
        .mass = 7.3e7, // Real val = 7.3e22 kg
        .radius = 2e3, // Real val = 1.7375e3 km
        .rotation = 0.0f,
        .parentBody = bodies[0],
        .angularSpeed = radsPerSecond(27.3 * 24 * 60 * 60),
        .initialAngle = 0,
        .orbitalRadius = 2e5 // Real val = 3.84e5 km
    };

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