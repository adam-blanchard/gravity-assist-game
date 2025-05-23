#include "body.h"
#include "game.h"

float getBodyAngle(celestialbody_t *body, float gameTime)
{
    return fmod(body->initialAngle + body->angularSpeed * gameTime, 2 * PI);
}

celestialbody_t **initBodies(int *numBodies)
{
    *numBodies = 2;
    celestialbody_t **bodies = malloc(sizeof(celestialbody_t *) * (*numBodies));

    // Planet orbiting Star - Earth
    bodies[0] = malloc(sizeof(celestialbody_t));
    *bodies[0] = (celestialbody_t){
        .type = TYPE_PLANET,
        .name = strdup("Earth"),
        .position = {0, 0},
        .mass = 5.97e9, // Real val = 5.97e24 kg
        .radius = 6e3,  // Real val = 6.378e3 km
        .rotation = 0.0f,
        .parentBody = NULL,
        .angularSpeed = 0, // Real val = 365.2 Days (365.2 * 24 * 60 * 60 seconds)
        .initialAngle = 0,
        .orbitalRadius = 0, // Real val = 1.496e8 km
        .atmosphereRadius = 8e3,
        .atmosphereDrag = 5,
        .atmosphereColour = (Color){10, 131, 251, 50}};

    // Moon orbiting Planet
    bodies[1] = malloc(sizeof(celestialbody_t));
    *bodies[1] = (celestialbody_t){
        .type = TYPE_MOON,
        .name = strdup("Earth's Moon"),
        .position = {0, 0},
        .mass = 7.3e7, // Real val = 7.3e22 kg
        .radius = 2e3, // Real val = 1.7375e3 km
        .rotation = 0.0f,
        .parentBody = bodies[0],
        .angularSpeed = radsPerSecond(27.3 * 24 * 60 * 60),
        .initialAngle = 0,
        .orbitalRadius = 1.8e5, // Real val = 3.84e5 km,
        .atmosphereRadius = -1,
        .atmosphereDrag = -1};

    return bodies;
}

bool saveBody(celestialbody_t *body, FILE *file, gamestate_t *state) {
    fwrite(&body->type, sizeof(CelestialType), 1, file);
    int nameLen = body->name ? strlen(body->name) : 0;
    fwrite(&nameLen, sizeof(int), 1, file);
    if (nameLen > 0) {
        fwrite(body->name, sizeof(char), nameLen, file);
    }
    fwrite(&body->position, sizeof(Vector2), 1, file);
    fwrite(&body->mass, sizeof(float), 1, file);
    fwrite(&body->radius, sizeof(float), 1, file);
    fwrite(&body->rotation, sizeof(float), 1, file);
    // Texture ID
    fwrite(&body->textureScale, sizeof(float), 1, file);
    int parentIndex = getBodyIndex(body->parentBody, state->bodies, state->numBodies);
    fwrite(parentIndex, sizeof(int), 1, file);
    fwrite(&body->orbitalRadius, sizeof(float), 1, file);
    fwrite(&body->angularSpeed, sizeof(float), 1, file);
    fwrite(&body->initialAngle, sizeof(float), 1, file);
    fwrite(&body->atmosphereRadius, sizeof(float), 1, file);
    fwrite(&body->atmosphereDrag, sizeof(float), 1, file);
    fwrite(&body->atmosphereColour, sizeof(Color), 1, file);

    if (ferror(file)) {
        TraceLog(LOG_ERROR, "Write error in saveBody");
        return false;
    }
    return true;

}

int getBodyIndex(celestialbody_t *body, celestialbody_t **bodies, int numBodies) {
    for (int i = 0; i < numBodies; i++) {
        if (body == bodies[i]) {
            return i;
        }
    }
    return -1;
}

celestialbody_t* getBodyPtr(int index, celestialbody_t **bodies, int numBodies) {
    if (index < 0 || index > numBodies - 1){
        return NULL;
    }
    return bodies[index];
}

void freeCelestialBodies(celestialbody_t **bodies, int numBodies)
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