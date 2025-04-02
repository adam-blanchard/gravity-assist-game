#ifndef PHYSICS_H
#define PHYSICS_H

#include <math.h>
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "body.h"
#include "ship.h"

float calculateOrbitalVelocity(float mass, float radius);

float calculateOrbitCircumference(float r);

float calculateEscapeVelocity(float mass, float radius);

float calculateDistance(Vector2 *pos1, Vector2 *pos2);

float calculateOrbitalRadius(float period, float mStar);

float calculateRelativeSpeed(ship_t *ship, celestialbody_t *body, float gameTime);

float rad2deg(float rad);

float deg2rad(float deg);

float radsPerSecond(int orbitalPeriod);

float calculateOrbitalSpeed(float mass, float radius);

void updateShipPositions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float dt);

void updateCelestialPositions(celestialbody_t **bodies, int numBodies, float time);

void updateLandedShipPosition(ship_t **ships, int numShips, float gameTime);

void detectCollisions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float gameTime);

Vector2 computeShipGravity(ship_t *ship, celestialbody_t **bodies, int numBodies);

void calculateShipFuturePositions(ship_t **ships, int numShips, celestialbody_t **bodies, int numBodies, float gameTime);

void landShip(ship_t *ship, celestialbody_t *body, float gameTime);

void takeoffShip(ship_t *ship);

bool detectShipBodyCollision(ship_t *ship, celestialbody_t *body);

bool detectShipAtmosphereCollision(ship_t *ship, celestialbody_t *body);

Vector2 calculateDragForce(ship_t *ship, celestialbody_t **bodies, int numBodies);

Vector2 calculateBodyVelocity(celestialbody_t *body, float gameTime);

#endif