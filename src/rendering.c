#include "rendering.h"

void drawBodies(CelestialBody **bodies, int numBodies)
{
    Color bodyColour;
    for (int i = numBodies - 1; i >= 0; i--)
    {
        if (bodies[i]->type == TYPE_STAR)
        {
            bodyColour = RED;
        }
        if (bodies[i]->type == TYPE_PLANET)
        {
            bodyColour = BLUE;
        }
        if (bodies[i]->type != TYPE_STAR && bodies[i]->type != TYPE_PLANET)
        {
            bodyColour = WHITE;
        }
        DrawCircleV(bodies[i]->position, bodies[i]->radius, bodyColour);
        DrawCircleV(bodies[i]->position, bodies[i]->atmosphereRadius, bodies[i]->atmosphereColour);
    }
}

void drawShips(Ship **ships, int numShips)
{
    for (int i = 0; i < numShips; i++)
    {
        DrawCircleV(ships[i]->position, 32.0f, YELLOW);
    }
}

void drawOrbits(CelestialBody **bodies, int numBodies, ColourScheme *colourScheme)
{
    for (int i = 0; i < numBodies; i++)
    {
        if (bodies[i]->orbitalRadius > 0 && bodies[i]->parentBody != NULL)
        {
            // DrawCircleLinesV(bodies[i]->parentBody->position, bodies[i]->orbitalRadius, ORBIT_COLOUR);
            DrawEllipseLines(bodies[i]->parentBody->position.x, bodies[i]->parentBody->position.y, bodies[i]->orbitalRadius, bodies[i]->orbitalRadius, colourScheme->orbitColour);
        }
    }
}

void drawTrajectories(Ship **ships, int numShips, ColourScheme *colourScheme)
{
    for (int i = 0; i < numShips; i++)
    {
        for (int j = 0; j < FUTURE_POSITIONS; j++)
        {
            if (j > 0)
                DrawLineV(ships[i]->futurePositions[j - 1], ships[i]->futurePositions[j], colourScheme->orbitColour);
        }
    }
}

void drawStaticGrid(float zoomLevel, int numQuadrants, ColourScheme *colourScheme)
{
    int numLines = sqrt(numQuadrants) - 1;

    if (numLines < 1)
    {
        return;
    }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    for (int i = 0; i < numLines; i++)
    {
        // Draw horizontal line
        int horizontalHeight = screenHeight * (i + 1) / (numLines + 1);
        DrawLine(-screenWidth, horizontalHeight, screenWidth, horizontalHeight, colourScheme->gridColour);
        // Draw vertical line
        int verticalWidth = screenWidth * (i + 1) / (numLines + 1);
        DrawLine(verticalWidth, -screenHeight, verticalWidth, screenHeight, colourScheme->gridColour);
    }
}

void drawCelestialGrid(CelestialBody **bodies, int numBodies, Camera2D camera, ColourScheme *colourScheme)
{
    /*
        Draws a grid with origin (0, 0) to the edges of visible space
        The grid should scale with the camera to demonstrate distance and velocity
    */
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int gridSpacing;

    // Do not draw grid when zoomed in
    if (camera.zoom >= 0.01f)
        return;

    // Zoom level break points for grid size
    if (camera.zoom >= 0.002f)
    {
        gridSpacing = 1e5;
    }
    else if (camera.zoom >= 0.0002f)
    {
        gridSpacing = 1e6;
    }
    else if (camera.zoom >= 0.00002f)
    {
        gridSpacing = 1e7;
    }
    else if (camera.zoom >= 0.000001f)
    {
        gridSpacing = 1e8;
    }
    else
    {
        gridSpacing = 1e9;
    }

    // Calculate visible world-space bounds
    Vector2 topLeft = GetScreenToWorld2D((Vector2){0, 0}, camera);
    Vector2 bottomRight = GetScreenToWorld2D((Vector2){screenWidth, screenHeight}, camera);

    // Snap grid bounds to the nearest gridSpacing multiple
    float startX = floorf(topLeft.x / gridSpacing) * gridSpacing;
    float startY = floorf(topLeft.y / gridSpacing) * gridSpacing;
    float endX = ceilf(bottomRight.x / gridSpacing) * gridSpacing;
    float endY = ceilf(bottomRight.y / gridSpacing) * gridSpacing;

    // Draw vertical lines
    for (float x = startX; x <= endX; x += gridSpacing)
    {
        DrawLineV((Vector2){x, startY}, (Vector2){x, endY}, colourScheme->gridColour);
    }

    // Draw horizontal lines
    for (float y = startY; y <= endY; y += gridSpacing)
    {
        DrawLineV((Vector2){startX, y}, (Vector2){endX, y}, colourScheme->gridColour);
    }
}

void drawPlayerStats(PlayerStats *playerStats)
{
    DrawText("Money:", 10, 40, HUD_FONT_SIZE, WHITE);
    DrawText(TextFormat("%i$", playerStats->money), 150 - MeasureText(TextFormat("%i$", playerStats->money), HUD_FONT_SIZE), 40, HUD_FONT_SIZE, WHITE);
}

void drawPlayerHUD(HUD *playerHUD)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float wMid = screenWidth / 2;
    float hMid = screenHeight / 2;
    // DrawCircle(wMid, screenHeight - 100, 100, (Color){255, 255, 255, 100});
    if (playerHUD->velocityTarget)
    {
        DrawText(TextFormat("Velocity Lock: %s", playerHUD->velocityTarget->name), screenWidth / 2 - MeasureText(TextFormat("Velocity Lock: %s", playerHUD->velocityTarget->name), 16) / 2, screenHeight - 120, 16, WHITE);
    }
    else
    {
        // char text = "Velocity Lock: Absolute";
        DrawText("Velocity Lock: Absolute", screenWidth / 2 - MeasureText("Velocity Lock: Absolute", 16) / 2, screenHeight - 120, 16, WHITE);
    }
    DrawText(TextFormat("%.1fm/s", playerHUD->speed), screenWidth / 2 - MeasureText(TextFormat("%.1fm/s", playerHUD->speed), 16) / 2, screenHeight - 100, 16, WHITE);

    Rectangle compassSource = {
        0,
        0,
        (float)playerHUD->compassTexture.width,
        (float)playerHUD->compassTexture.height};
    Rectangle compassDest = {
        wMid,
        screenHeight - 50,
        (float)playerHUD->compassTexture.width,
        (float)playerHUD->compassTexture.height};
    Vector2 compassOrigin = {
        (float)((playerHUD->compassTexture.width) / 2),
        (float)((playerHUD->compassTexture.height) / 2)};
    DrawTexturePro(playerHUD->compassTexture, compassSource, compassDest, compassOrigin, 0.0f, WHITE);

    float widthScale = (playerHUD->arrowTexture.width * -0.5 / 2);
    float heightScale = (playerHUD->arrowTexture.height * -0.5 / 2);
    Rectangle arrowSource = {
        0,
        0,
        (float)playerHUD->arrowTexture.width,
        (float)playerHUD->arrowTexture.height};
    Rectangle arrowDest = {
        wMid,
        screenHeight - 50,
        (float)playerHUD->arrowTexture.width + widthScale,
        (float)playerHUD->arrowTexture.height + heightScale};
    Vector2 arrowOrigin = {
        (float)((playerHUD->arrowTexture.width + widthScale) / 2),
        (float)((playerHUD->arrowTexture.height + heightScale) / 2)};
    DrawTexturePro(playerHUD->arrowTexture, arrowSource, arrowDest, arrowOrigin, playerHUD->playerRotation, WHITE);
}

// void drawPlayerInventory(CelestialBody *playerShip, Resource *resourceDefinitions)
// {
//     int initialHeight = 70;
//     int heightStep = 30;
//     for (int i = 0; i < RESOURCE_COUNT; i++)
//     {
//         // Currently renders the associated enum integer of each mineral
//         // Possibly need to change mineral to be a struct so we can store a name (char array)
//         DrawText(TextFormat("%s:", resourceDefinitions[i].name), 10, (initialHeight + (heightStep * i)), HUD_FONT_SIZE, WHITE);
//         DrawText(TextFormat("%it", playerShip->shipSettings.inventory[i].quantity), 150 - MeasureText(TextFormat("%it", playerShip->shipSettings.inventory[i].quantity), HUD_FONT_SIZE), (initialHeight + (heightStep * i)), HUD_FONT_SIZE, WHITE);
//     }
// }

// void drawBodies(CelestialBody **bodies, int numBodies)
// {
//     for (int i = numBodies - 1; i >= 0; i--)
//     {
//         float scale = ((bodies[i]->radius * 2) / bodies[i]->texture->width) * bodies[i]->textureScale;
//         // Source rectangle (part of the texture to use for drawing i.e. the whole texture)
//         Rectangle source = {
//             0,
//             0,
//             (float)bodies[i]->texture->width,
//             (float)bodies[i]->texture->height};

//         // Destination rectangle (Screen rectangle locating where to draw the texture)
//         float widthScale = (bodies[i]->texture->width * scale / 2);
//         float heightScale = (bodies[i]->texture->height * scale / 2);
//         Rectangle dest = {
//             bodies[i]->position.x,
//             bodies[i]->position.y,
//             (float)bodies[i]->texture->width + widthScale,
//             (float)bodies[i]->texture->height + heightScale};

//         // Origin of the texture for rotation and scaling - centre for our purpose
//         Vector2 origin = {
//             (float)((bodies[i]->texture->width + widthScale) / 2),
//             (float)((bodies[i]->texture->height + heightScale) / 2)};
//         DrawTexturePro(*bodies[i]->texture, source, dest, origin, bodies[i]->rotation, WHITE);

//         // Draw collider for debug
//         // DrawCircle(bodies[i]->position.x, bodies[i]->position.y, bodies[i]->radius, WHITE);
//     }
// }