#include "ship.h"
#include "game.h"

ship_t **initShips(int *numShips)
{
    *numShips = 2;
    ship_t **ships = malloc(sizeof(ship_t *) * (*numShips));

    ships[0] = malloc(sizeof(ship_t));
    *ships[0] = (ship_t){
        .position = (Vector2){0, -1e4},
        .velocity = (Vector2){0, 0},
        .mass = 1e6,
        .radius = 32.0f,
        .rotation = 0,
        .rotationSpeed = 90,
        .thrust = 6e8,
        .thrusterForce = 1,
        .state = SHIP_FLYING,
        .type = SHIP_ROCKET,
        .isSelected = true,
        .trajectorySize = 36000,
        .drawTrajectory = true,
        .textureScale = 1,
        .baseTextureId = 0,
        .engineTextureId = 1,
        .thrusterUpTextureId = 2,
        .thrusterDownTextureId = 3,
        .thrusterRightTextureId = 4,
        .thrusterLeftTextureId = 5,
        .thrusterRotateRightTextureId = 6,
        .thrusterRotateLeftTextureId = 7,
        .mainEnginesOn = false,
        .thrusterUp = false,
        .thrusterDown = false,
        .thrusterRight = false,
        .thrusterLeft = false,
        .thrusterRotateRight = false,
        .thrusterRotateLeft = false};

    if (ships[0]->trajectorySize > MAX_FUTURE_POSITIONS)
    {
        printf("Ship trajectory size %i is greater than max size %i. Exiting.", ships[0]->trajectorySize, MAX_FUTURE_POSITIONS);
        exit(0);
    }
    ships[0]->futurePositions = malloc(sizeof(Vector2) * ships[0]->trajectorySize);
    ships[0]->baseTexture = LoadTextureById(ships[0]->baseTextureId);
    ships[0]->engineTexture = LoadTextureById(ships[0]->engineTextureId);
    ships[0]->thrusterUpTexture = LoadTextureById(ships[0]->thrusterUpTextureId);
    ships[0]->thrusterDownTexture = LoadTextureById(ships[0]->thrusterDownTextureId);
    ships[0]->thrusterRightTexture = LoadTextureById(ships[0]->thrusterRightTextureId);
    ships[0]->thrusterLeftTexture = LoadTextureById(ships[0]->thrusterLeftTextureId);
    ships[0]->thrusterRotateRightTexture = LoadTextureById(ships[0]->thrusterRotateRightTextureId);
    ships[0]->thrusterRotateLeftTexture = LoadTextureById(ships[0]->thrusterRotateLeftTextureId);

    ships[1] = malloc(sizeof(ship_t));
    *ships[1] = (ship_t){
        .position = (Vector2){0, 0},
        .velocity = (Vector2){0, 0},
        .mass = 3e6,
        .radius = 32.0f,
        .rotation = 0,
        .rotationSpeed = 90,
        .thrust = 0,
        .thrusterForce = 1,
        .state = SHIP_FLYING,
        .type = SHIP_STATION,
        .isSelected = false,
        .trajectorySize = 8780,
        .drawTrajectory = true,
        .textureScale = 3,
        .baseTextureId = 8,
        .thrusterUpTextureId = 9,
        .thrusterDownTextureId = 10,
        .thrusterRightTextureId = 11,
        .thrusterLeftTextureId = 12,
        .thrusterRotateRightTextureId = 13,
        .thrusterRotateLeftTextureId = 14,
        .mainEnginesOn = false,
        .thrusterUp = false,
        .thrusterDown = false,
        .thrusterRight = false,
        .thrusterLeft = false,
        .thrusterRotateRight = false,
        .thrusterRotateLeft = false};

    if (ships[1]->trajectorySize > MAX_FUTURE_POSITIONS)
    {
        printf("Ship trajectory size %i is greater than max size %i. Exiting.", ships[1]->trajectorySize, MAX_FUTURE_POSITIONS);
        exit(0);
    }
    ships[1]->futurePositions = malloc(sizeof(Vector2) * ships[1]->trajectorySize);
    ships[1]->baseTexture = LoadTextureById(ships[1]->baseTextureId);
    ships[1]->thrusterUpTexture = LoadTextureById(ships[1]->thrusterUpTextureId);
    ships[1]->thrusterDownTexture = LoadTextureById(ships[1]->thrusterDownTextureId);
    ships[1]->thrusterRightTexture = LoadTextureById(ships[1]->thrusterRightTextureId);
    ships[1]->thrusterLeftTexture = LoadTextureById(ships[1]->thrusterLeftTextureId);
    ships[1]->thrusterRotateRightTexture = LoadTextureById(ships[1]->thrusterRotateRightTextureId);
    ships[1]->thrusterRotateLeftTexture = LoadTextureById(ships[1]->thrusterRotateLeftTextureId);

    return ships;
}

bool saveShip(ship_t *ship, FILE *file, gamestate_t *state) {
    // Serialises all ship attributes that cannot be re-computed
    fwrite(&ship->position, sizeof(Vector2), 1, file);
    fwrite(&ship->velocity, sizeof(Vector2), 1, file);
    fwrite(&ship->mass, sizeof(float), 1, file);
    fwrite(&ship->rotation, sizeof(float), 1, file);
    fwrite(&ship->rotationSpeed, sizeof(float), 1, file);
    fwrite(&ship->radius, sizeof(float), 1, file);
    fwrite(&ship->thrust, sizeof(float), 1, file);
    fwrite(&ship->thrusterForce, sizeof(float), 1, file);
    fwrite(&ship->fuel, sizeof(float), 1, file);
    fwrite(&ship->fuelConsumption, sizeof(float), 1, file);
    fwrite(&ship->state, sizeof(ShipState), 1, file);
    fwrite(&ship->type, sizeof(ShipType), 1, file);
    int landedIndex = getBodyIndex(ship->landedBody, state->bodies, state->numBodies);
    fwrite(&landedIndex, sizeof(int), 1, file);
    fwrite(&ship->landingPosition, sizeof(Vector2), 1, file);
    fwrite(&ship->trajectorySize, sizeof(int), 1, file);
    fwrite(&ship->baseTextureId, sizeof(int), 1, file);
    fwrite(&ship->engineTextureId, sizeof(int), 1, file);
    fwrite(&ship->thrusterUpTextureId, sizeof(int), 1, file);
    fwrite(&ship->thrusterDownTextureId, sizeof(int), 1, file);
    fwrite(&ship->thrusterRightTextureId, sizeof(int), 1, file);
    fwrite(&ship->thrusterLeftTextureId, sizeof(int), 1, file);
    fwrite(&ship->thrusterRotateRightTextureId, sizeof(int), 1, file);
    fwrite(&ship->thrusterRotateLeftTextureId, sizeof(int), 1, file);
    fwrite(&ship->textureScale, sizeof(float), 1, file);

    if (ferror(file)) {
        TraceLog(LOG_ERROR, "Write error in saveShip");
        return false;
    }
    return true;
}

ship_t* loadShip(FILE *file, gamestate_t* state) {
    ship_t* ship = malloc(sizeof(ship_t));
    if (!ship) {
        TraceLog(LOG_ERROR, "Failed to allocate ship_t");
        return NULL;
    }

    ship->futurePositions = NULL;
    ship->landedBody = NULL;
    int landedIndex = -1;

    if (fread(&ship->position, sizeof(Vector2), 1, file) != 1 
        || fread(&ship->velocity, sizeof(Vector2), 1, file) != 1 
        || fread(&ship->mass, sizeof(float), 1, file) != 1
        || fread(&ship->rotation, sizeof(float), 1, file) != 1
        || fread(&ship->rotationSpeed, sizeof(float), 1, file) != 1
        || fread(&ship->radius, sizeof(float), 1, file) != 1
        || fread(&ship->thrust, sizeof(float), 1, file) != 1
        || fread(&ship->thrusterForce, sizeof(float), 1, file) != 1
        || fread(&ship->fuel, sizeof(float), 1, file) != 1
        || fread(&ship->fuelConsumption, sizeof(float), 1, file) != 1
        || fread(&ship->state, sizeof(ShipState), 1, file) != 1
        || fread(&ship->type, sizeof(ShipType), 1, file) != 1
        || fread(&landedIndex, sizeof(int), 1, file) != 1
        || fread(&ship->landingPosition, sizeof(Vector2), 1, file) != 1
        || fread(&ship->trajectorySize, sizeof(int), 1, file) != 1
        || fread(&ship->baseTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->engineTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->thrusterUpTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->thrusterDownTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->thrusterRightTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->thrusterLeftTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->thrusterRotateRightTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->thrusterRotateLeftTextureId, sizeof(int), 1, file) != 1
        || fread(&ship->textureScale, sizeof(float), 1, file) != 1)
        {
        TraceLog(LOG_ERROR, "Failed to read ship fields");
        goto error;
    }

    ship->landedBody = getBodyPtr(landedIndex, state->bodies, state->numBodies);

    // Load textures
    ship->baseTexture = LoadTextureById(ship->baseTextureId);
    ship->thrusterUpTexture = LoadTextureById(ship->thrusterUpTextureId);
    ship->thrusterDownTexture = LoadTextureById(ship->thrusterDownTextureId);
    ship->thrusterRightTexture = LoadTextureById(ship->thrusterRightTextureId);
    ship->thrusterLeftTexture = LoadTextureById(ship->thrusterLeftTextureId);
    ship->thrusterRotateRightTexture = LoadTextureById(ship->thrusterRotateRightTextureId);
    ship->thrusterRotateLeftTexture = LoadTextureById(ship->thrusterRotateLeftTextureId);

    // Initialise remaining attributes
    ship->isSelected = false;
    ship->drawTrajectory = true;
    ship->mainEnginesOn = false;
    ship->thrusterUp = false;
    ship->thrusterDown = false;
    ship->thrusterRight = false;
    ship->thrusterLeft = false;
    ship->thrusterRotateRight = false;
    ship->thrusterRotateLeft = false;

    return ship;

    error:
        freeShip(ship);
        return NULL;
}

void freeShip(ship_t* ship) {
    if (ship->futurePositions)
    {
        free(ship->futurePositions);
    }
    UnloadTexture(ship->baseTexture);
    if (ship->type != SHIP_STATION)
    {
        UnloadTexture(ship->engineTexture);
    }
    UnloadTexture(ship->thrusterUpTexture);
    UnloadTexture(ship->thrusterDownTexture);
    UnloadTexture(ship->thrusterRightTexture);
    UnloadTexture(ship->thrusterLeftTexture);
    UnloadTexture(ship->thrusterRotateRightTexture);
    UnloadTexture(ship->thrusterRotateLeftTexture);
    free(ship);
}

void freeShips(ship_t **ships, int numShips)
{
    if (ships)
    {
        for (int i = 0; i < numShips; i++)
        {
            freeShip(ships[i]);
        }
        free(ships);
    }
}


void takeoffShip(ship_t *ship)
{
    if (ship->state != SHIP_LANDED || ship->landedBody == NULL)
        return;

    ship->state = SHIP_FLYING;
    ship->landedBody = NULL;
}

/*
Intended control scheme
    Shift and ctrl for throttle up and down
    Q and E for x translation
    W and S for y translation
    A and D for rotation
*/

void handleThrottle(ship_t **ships, int numShips, float dt, ShipThrottle throttleCommand)
{
    for (int i = 0; i < numShips; i++)
    {
        if (!ships[i]->isSelected || ships[i]->type == SHIP_STATION)
        {
            continue;
        }

        if (throttleCommand == THROTTLE_UP)
        {
            ships[i]->throttle += THROTTLE_INCREMENT;
        }
        else if (throttleCommand == THROTTLE_DOWN)
        {
            ships[i]->throttle -= THROTTLE_INCREMENT;
        }
        ships[i]->throttle = Clamp(ships[i]->throttle, 0, 1);
    }
}

void handleThruster(ship_t **ships, int numShips, float dt, ShipMovement thrusterCommand)
{
    for (int i = 0; i < numShips; i++)
    {
        if (!ships[i]->isSelected)
        {
            continue;
        }

        float radians;

        if (thrusterCommand == THRUSTER_UP)
        {
            radians = (ships[i]->rotation) * PI / 180.0f;
            ships[i]->thrusterUp = true;
        }
        if (thrusterCommand == THRUSTER_RIGHT)
        {
            radians = (ships[i]->rotation + 90) * PI / 180.0f;
            ships[i]->thrusterRight = true;
        }
        if (thrusterCommand == THRUSTER_LEFT)
        {
            radians = (ships[i]->rotation - 90) * PI / 180.0f;
            ships[i]->thrusterLeft = true;
        }
        if (thrusterCommand == THRUSTER_DOWN)
        {
            radians = (ships[i]->rotation - 180) * PI / 180.0f;
            ships[i]->thrusterDown = true;
        }

        Vector2 direction = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
        Vector2 force = Vector2Scale(direction, ships[i]->thrusterForce * dt);
        ships[i]->velocity = Vector2Add(ships[i]->velocity, force);
    }
}

void handleRotation(ship_t **ships, int numShips, float dt, ShipMovement direction)
{
    for (int i = 0; i < numShips; i++)
    {
        if (!ships[i]->isSelected)
        {
            continue;
        }
        if (direction == ROTATION_RIGHT)
        {
            ships[i]->rotation += ships[i]->rotationSpeed * dt; // Rotate right
            ships[i]->thrusterRotateRight = true;
        }
        else
        {
            ships[i]->rotation -= ships[i]->rotationSpeed * dt; // Rotate left
            ships[i]->thrusterRotateLeft = true;
        }

        ships[i]->rotation = fmod(ships[i]->rotation + 360.0f, 360.0f);
    }
}

void cutEngines(ship_t **ships, int numShips)
{
    for (int i = 0; i < numShips; i++)
    {
        if (!ships[i]->isSelected)
        {
            continue;
        }
        ships[i]->throttle = 0;
    }
}

void toggleDrawTrajectory(ship_t **ships, int numShips)
{
    for (int i = 0; i < numShips; i++)
    {
        if (!ships[i]->isSelected)
        {
            continue;
        }

        ships[i]->drawTrajectory = !ships[i]->drawTrajectory;
    }
}

void updateShipTextureFlags(ship_t **ships, int numShips)
{
    for (int i = 0; i < numShips; i++)
    {
        if (ships[i]->throttle > 0)
        {
            ships[i]->mainEnginesOn = true;
        }
        else
        {
            ships[i]->mainEnginesOn = false;
        }

        ships[i]->thrusterUp = false;
        ships[i]->thrusterDown = false;
        ships[i]->thrusterRight = false;
        ships[i]->thrusterLeft = false;
        ships[i]->thrusterRotateRight = false;
        ships[i]->thrusterRotateLeft = false;
    }
}