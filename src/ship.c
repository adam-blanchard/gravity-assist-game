#include "ship.h"

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

void serialiseShip(ship_t *ship, FILE *file) {
    // TODO: finish this logic
    fwrite(&ship->position, sizeof(Vector2), 1, file);
    fwrite(&ship->velocity, sizeof(Vector2), 1, file);
}

void freeShips(ship_t **ships, int numShips)
{
    if (ships)
    {
        for (int i = 0; i < numShips; i++)
        {
            if (ships[i]->futurePositions)
            {
                free(ships[i]->futurePositions);
            }
            UnloadTexture(ships[i]->baseTexture);
            if (ships[i]->type != SHIP_STATION)
            {
                UnloadTexture(ships[i]->engineTexture);
            }
            UnloadTexture(ships[i]->thrusterUpTexture);
            UnloadTexture(ships[i]->thrusterDownTexture);
            UnloadTexture(ships[i]->thrusterRightTexture);
            UnloadTexture(ships[i]->thrusterLeftTexture);
            UnloadTexture(ships[i]->thrusterRotateRightTexture);
            UnloadTexture(ships[i]->thrusterRotateLeftTexture);
            free(ships[i]);
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