#include "textures.h"
#include <string.h>

static struct {
    int id;
    const char *filename;
} textureMap[] = {
    {0, "assets/ship/ship_1/ship_1.png"},
    {1, "assets/ship/ship_1/ship_1_thrust.png"},
    {2, "assets/ship/ship_1/ship_1_move_up.png"},
    {3, "assets/ship/ship_1/ship_1_move_down.png"},
    {4, "assets/ship/ship_1/ship_1_move_right.png"},
    {5, "assets/ship/ship_1/ship_1_move_left.png"},
    {6, "assets/ship/ship_1/ship_1_rotate_right.png"},
    {7, "assets/ship/ship_1/ship_1_rotate_left.png"},
    {8, "assets/ship/station_1/station_1_base.png"},
    {9, "assets/ship/station_1/station_1_move_up.png"},
    {10, "assets/ship/station_1/station_1_move_down.png"},
    {11, "assets/ship/station_1/station_1_move_right.png"},
    {12, "assets/ship/station_1/station_1_move_left.png"},
    {13, "assets/ship/station_1/station_1_rotate_right.png"},
    {14, "assets/ship/station_1/station_1_rotate_left.png"},
    {-1, NULL}
};

Texture2D LoadTextureById(int id) {
    for (int i = 0; textureMap[i].id != -1; i++) {
        if (textureMap[i].id == id) {
            return LoadTexture(textureMap[i].filename);
        }
    }
    TraceLog(LOG_WARNING, "Texture ID %d not found, using default", id);
    return LoadTexture("assets/icons/logo_ship.png");
}