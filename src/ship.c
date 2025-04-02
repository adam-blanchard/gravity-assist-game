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
        .thrust = 2e1,
        .state = SHIP_FLYING,
        .isSelected = true,
        .futurePositions = malloc(sizeof(Vector2) * FUTURE_POSITIONS),
        .idleTexture = LoadTexture("./textures/ship/ship_1.png"),
        .thrustTexture = LoadTexture("./textures/ship/ship_1_thrust.png")};

    return ships;
}

void freeShips(ship_t **ships, int numShips)
{
    {
        if (ships)
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