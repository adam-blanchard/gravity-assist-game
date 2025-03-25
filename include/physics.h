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

float calculateRelativeSpeed(struct Ship *ship, struct CelestialBody *body, float gameTime);

float rad2deg(float rad);

float deg2rad(float deg);

float radsPerSecond(int orbitalPeriod);

float calculateOrbitalSpeed(float mass, float radius);

void updateShipPositions(struct Ship **ships, int numShips, struct CelestialBody **bodies, int numBodies, float dt);

void updateCelestialPositions(struct CelestialBody **bodies, int numBodies, float time);

void updateLandedShipPosition(struct Ship **ships, int numShips, float gameTime);

void detectCollisions(struct Ship **ships, int numShips, struct CelestialBody **bodies, int numBodies, float gameTime);

Vector2 computeShipGravity(struct Ship *ship, struct CelestialBody **bodies, int numBodies);

void calculateShipFuturePositions(struct Ship **ships, int numShips, struct CelestialBody **bodies, int numBodies, float gameTime);

void landShip(struct Ship *ship, struct CelestialBody *body, float gameTime);

void takeoffShip(struct Ship *ship);

#endif