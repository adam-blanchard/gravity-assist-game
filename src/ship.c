#include "ship.h"

ship_t **initShips(int *numShips)
{
    *numShips = 1;
    ship_t **ships = malloc(sizeof(ship_t *) * (*numShips));

    ships[0] = malloc(sizeof(ship_t));
    *ships[0] = (ship_t){
        .position = (Vector2){0, -1e4},
        .velocity = (Vector2){0, 0},
        .mass = 1e6,
        .radius = 32.0f,
        .rotation = 0,
        .rotationSpeed = 90,
        .thrust = 8e8,
        .thrusterForce = 2,
        .state = SHIP_FLYING,
        .isSelected = true,
        .futurePositions = malloc(sizeof(Vector2) * FUTURE_POSITIONS),
        .drawTrajectory = true,
        .idleTexture = LoadTexture("./textures/ship/ship_1.png"),
        .thrustTexture = LoadTexture("./textures/ship/ship_1_thrust.png")};
    ships[0]->currentTexture = &ships[0]->idleTexture;

    return ships;
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
            UnloadTexture(ships[i]->idleTexture);
            UnloadTexture(ships[i]->thrustTexture);
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
        if (!ships[i]->isSelected)
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

        if (ships[i]->throttle > 0)
        {
            ships[i]->currentTexture = &ships[i]->thrustTexture;
        }
        else
        {
            ships[i]->currentTexture = &ships[i]->idleTexture;
        }
    }
}

void handleThruster(ship_t **ships, int numShips, float dt, ShipThruster thrusterCommand)
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
        }
        if (thrusterCommand == THRUSTER_RIGHT)
        {
            radians = (ships[i]->rotation + 90) * PI / 180.0f;
        }
        if (thrusterCommand == THRUSTER_LEFT)
        {
            radians = (ships[i]->rotation - 90) * PI / 180.0f;
        }
        if (thrusterCommand == THRUSTER_DOWN)
        {
            radians = (ships[i]->rotation - 180) * PI / 180.0f;
        }

        Vector2 direction = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
        Vector2 force = Vector2Scale(direction, ships[i]->thrusterForce * dt);
        ships[i]->velocity = Vector2Add(ships[i]->velocity, force);

        // if (ships[i]->state == SHIP_FLYING)
        // {
        //     // Convert rotation to radians for vector calculations
        //     float radians = ships[i]->rotation * PI / 180.0f;
        //     Vector2 thrustDirection = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
        //     Vector2 thrust = Vector2Scale(thrustDirection, ships[i]->thrust * dt);
        //     ships[i]->velocity = Vector2Add(ships[i]->velocity, thrust);
        //     ships[i]->fuel -= ships[i]->fuelConsumption;
        // }
    }
}

void handleRotation(ship_t **ships, int numShips, float dt, ShipRotation direction)
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
        }
        else
        {
            ships[i]->rotation -= ships[i]->rotationSpeed * dt; // Rotate left
        }

        ships[i]->rotation = fmod(ships[i]->rotation + 360.0f, 360.0f);
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