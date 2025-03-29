#ifndef RENDERING_H
#define RENDERING_H

#include "raylib.h"
#include "game.h"
#include "body.h"
#include "ship.h"
#include "ui.h"

void drawBodies(CelestialBody **bodies, int numBodies);
void drawShips(Ship **ships, int numShips);
void drawOrbits(CelestialBody **bodies, int numBodies, ColourScheme *colourScheme);
void drawTrajectories(Ship **ships, int numShips, ColourScheme *colourScheme);
void drawStaticGrid(float zoomLevel, int numQuadrants, ColourScheme *colourScheme);
void drawCelestialGrid(CelestialBody **bodies, int numBodies, Camera2D camera, ColourScheme *colourScheme);
void drawPlayerStats(PlayerStats *playerStats);
void drawPlayerHUD(HUD *playerHUD);
void drawPlayerInventory(CelestialBody *playerShip, Resource *resourceDefinitions);

#endif