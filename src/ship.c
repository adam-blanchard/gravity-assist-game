#include "ship.h"

Ship **initShips(int *numShips)
{
    *numShips = 1;
    Ship **ships = malloc(sizeof(Ship *) * (*numShips));

    ships[0] = malloc(sizeof(Ship));
    *ships[0] = (Ship){
        .position = (Vector2){3e4, 0},
        .velocity = (Vector2){0, 1.2e3},
        .mass = 1e3,
        .radius = 32.0f,
        .rotation = 0,
        .thrust = 1e1,
        .state = SHIP_FLYING,
        .isSelected = true,
        .futurePositions = malloc(sizeof(Vector2) * FUTURE_POSITIONS)};

    return ships;
}