#include "utils.h"

float rad2deg(float rad)
{
    return rad * (180.0f / PI);
}

float deg2rad(float deg)
{
    return deg * (PI / 180.0f);
}

float radsPerSecond(int orbitalPeriod)
{
    float deg = 360.0f / orbitalPeriod;
    return deg2rad(deg);
}