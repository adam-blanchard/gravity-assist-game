#ifndef RENDERING_H
#define RENDERING_H

#include "raylib.h"
#include "game.h"
#include "body.h"
#include "ship.h"
#include "ui.h"

void drawBodies(celestialbody_t **bodies, int numBodies);
void drawShips(ship_t **ships, int numShips, Camera2D *camera, Texture2D *shipLogoTexture);
void drawOrbits(celestialbody_t **bodies, int numBodies, ColourScheme *colourScheme);
void drawTrajectories(ship_t **ships, int numShips, ColourScheme *colourScheme);
void drawStaticGrid(float zoomLevel, int numQuadrants, ColourScheme *colourScheme);
void drawCelestialGrid(celestialbody_t **bodies, int numBodies, Camera2D camera, ColourScheme *colourScheme);
void drawPlayerStats(PlayerStats *playerStats);
void drawPlayerHUD(HUD *playerHUD);
// void drawPlayerInventory(ship_t *playerShip, Resource *resourceDefinitions);

#endif