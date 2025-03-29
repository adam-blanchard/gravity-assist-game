#include "ship.h"

Ship **initShips(int *numShips)
{
    *numShips = 1;
    Ship **ships = malloc(sizeof(Ship *) * (*numShips));

    ships[0] = malloc(sizeof(Ship));
    *ships[0] = (Ship){
        .position = (Vector2){0, -1e4},
        .velocity = (Vector2){0, 0},
        .mass = 1e6,
        .radius = 32.0f,
        .rotation = 0,
        .rotationSpeed = 90,
        .thrust = 2e1,
        .state = SHIP_FLYING,
        .isSelected = true,
        .futurePositions = malloc(sizeof(Vector2) * FUTURE_POSITIONS)};

    return ships;
}

void freeShips(Ship **ships, int numShips)
{
    {
        if (ships)
            for (int i = 0; i < numShips; i++)
            {
                if (ships[i]->futurePositions)
                    free(ships[i]->futurePositions);
                free(ships[i]);
            }
        free(ships);
    }
}