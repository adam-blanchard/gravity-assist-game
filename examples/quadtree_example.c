#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>

// Define the GameObject structure
typedef struct GameObject
{
    Rectangle bounds;
    Vector2 velocity;
} GameObject;

// Define the QuadtreeNode structure
typedef struct QuadtreeNode
{
    Rectangle bounds;
    GameObject **objects;
    int objectCount;
    int capacity;
    struct QuadtreeNode *nw; // Northwest
    struct QuadtreeNode *ne; // Northeast
    struct QuadtreeNode *sw; // Southwest
    struct QuadtreeNode *se; // Southeast
} QuadtreeNode;

// Constants
#define CAPACITY 4
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_OBJECTS 100

// Function declarations
QuadtreeNode *createNode(Rectangle bounds, int capacity);
bool contains(Rectangle container, Rectangle contained);
void subdivide(QuadtreeNode *node);
void insert(QuadtreeNode *node, GameObject *obj);
void insertIntoChildren(QuadtreeNode *node, GameObject *obj);
void query(QuadtreeNode *node, Rectangle queryRect, GameObject **result, int *count);
void freeQuadtree(QuadtreeNode *node);
void drawQuadtree(QuadtreeNode *node);

QuadtreeNode *createNode(Rectangle bounds, int capacity)
{
    QuadtreeNode *node = (QuadtreeNode *)malloc(sizeof(QuadtreeNode));
    if (!node)
    {
        printf("Failed to allocate QuadtreeNode\n");
        exit(1);
    }
    node->bounds = bounds;
    node->objects = (GameObject **)malloc(sizeof(GameObject *) * capacity);
    if (!node->objects)
    {
        printf("Failed to allocate objects array\n");
        free(node);
        exit(1);
    }
    node->objectCount = 0;
    node->capacity = capacity;
    node->nw = NULL;
    node->ne = NULL;
    node->sw = NULL;
    node->se = NULL;
    return node;
}

bool contains(Rectangle container, Rectangle contained)
{
    return (contained.x >= container.x &&
            contained.y >= container.y &&
            contained.x + contained.width <= container.x + container.width &&
            contained.y + contained.height <= container.y + container.height);
}

void subdivide(QuadtreeNode *node)
{
    if (!node)
    {
        printf("Null node in subdivide\n");
        return;
    }
    float x = node->bounds.x;
    float y = node->bounds.y;
    float w = node->bounds.width / 2;
    float h = node->bounds.height / 2;

    node->nw = createNode((Rectangle){x, y, w, h}, node->capacity);
    node->ne = createNode((Rectangle){x + w, y, w, h}, node->capacity);
    node->sw = createNode((Rectangle){x, y + h, w, h}, node->capacity);
    node->se = createNode((Rectangle){x + w, y + h, w, h}, node->capacity);
}

void insertIntoChildren(QuadtreeNode *node, GameObject *obj)
{
    if (!node || !obj)
    {
        printf("Null pointer in insertIntoChildren: node=%p, obj=%p\n", (void *)node, (void *)obj);
        return;
    }
    if (node->nw == NULL)
    {
        printf("Attempt to insert into children of leaf node\n");
        return;
    }
    if (contains(node->nw->bounds, obj->bounds))
    {
        insert(node->nw, obj);
    }
    else if (contains(node->ne->bounds, obj->bounds))
    {
        insert(node->ne, obj);
    }
    else if (contains(node->sw->bounds, obj->bounds))
    {
        insert(node->sw, obj);
    }
    else if (contains(node->se->bounds, obj->bounds))
    {
        insert(node->se, obj);
    }
    else
    {
        if (node->objectCount < node->capacity)
        {
            node->objects[node->objectCount] = obj;
            node->objectCount++;
            // printf("Inserted into parent at count=%d\n", node->objectCount);
        }
        else
        {
            printf("Parent node full: count=%d, capacity=%d\n", node->objectCount, node->capacity);
        }
    }
}

void insert(QuadtreeNode *node, GameObject *obj)
{
    if (!node || !obj)
    {
        printf("Null pointer in insert: node=%p, obj=%p\n", (void *)node, (void *)obj);
        return;
    }
    if (!CheckCollisionRecs(node->bounds, obj->bounds))
    {
        return;
    }

    if (node->nw == NULL)
    { // Leaf node
        if (node->objectCount < node->capacity)
        {
            node->objects[node->objectCount] = obj;
            node->objectCount++;
            // printf("Inserted into leaf at count=%d\n", node->objectCount);
        }
        else
        {
            subdivide(node);
            int oldCount = node->objectCount;
            for (int i = 0; i < oldCount; i++)
            {
                if (i >= node->capacity)
                {
                    printf("Error: i=%d exceeds capacity=%d in redistribution\n", i, node->capacity);
                    break;
                }
                insertIntoChildren(node, node->objects[i]);
            }
            node->objectCount = 0;
            insertIntoChildren(node, obj);
        }
    }
    else
    { // Internal node
        insertIntoChildren(node, obj);
    }
}

void query(QuadtreeNode *node, Rectangle queryRect, GameObject **result, int *count)
{
    if (!node || !result || !count)
    {
        printf("Null pointer in query: node=%p, result=%p, count=%p\n", (void *)node, (void *)result, (void *)count);
        return;
    }
    if (!CheckCollisionRecs(node->bounds, queryRect))
    {
        return;
    }

    for (int i = 0; i < node->objectCount; i++)
    {
        if (i >= node->capacity)
        {
            printf("Error: i=%d exceeds capacity=%d in query\n", i, node->capacity);
            break;
        }
        if (CheckCollisionRecs(node->objects[i]->bounds, queryRect))
        {
            if (*count < MAX_OBJECTS)
            {
                result[*count] = node->objects[i];
                (*count)++;
            }
            else
            {
                printf("Warning: Query result exceeds MAX_OBJECTS=%d\n", MAX_OBJECTS);
            }
        }
    }

    if (node->nw != NULL)
    {
        query(node->nw, queryRect, result, count);
        query(node->ne, queryRect, result, count);
        query(node->sw, queryRect, result, count);
        query(node->se, queryRect, result, count);
    }
}

void freeQuadtree(QuadtreeNode *node)
{
    if (node == NULL)
        return;
    if (node->nw != NULL)
    {
        freeQuadtree(node->nw);
        freeQuadtree(node->ne);
        freeQuadtree(node->sw);
        freeQuadtree(node->se);
    }
    free(node->objects);
    free(node);
}

void drawQuadtree(QuadtreeNode *node)
{
    if (node == NULL)
        return;
    DrawRectangleLines((int)node->bounds.x, (int)node->bounds.y,
                       (int)node->bounds.width, (int)node->bounds.height, GRAY);
    if (node->nw != NULL)
    {
        drawQuadtree(node->nw);
        drawQuadtree(node->ne);
        drawQuadtree(node->sw);
        drawQuadtree(node->se);
    }
}

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Quadtree Collision Detection Example");
    SetTargetFPS(60);

    int minSpeed = 1;
    int maxSpeed = 3;

    GameObject objects[] = {
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
        {(Rectangle){GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 20, 20}, (Vector2){GetRandomValue(-minSpeed, maxSpeed), GetRandomValue(-minSpeed, maxSpeed)}},
    };
    int numObjects = sizeof(objects) / sizeof(objects[0]);

    while (!WindowShouldClose())
    {
        for (int i = 0; i < numObjects; i++)
        {
            objects[i].bounds.x += objects[i].velocity.x;
            objects[i].bounds.y += objects[i].velocity.y;

            if (objects[i].bounds.x < 0 || objects[i].bounds.x + objects[i].bounds.width > SCREEN_WIDTH)
            {
                objects[i].velocity.x *= -1;
            }
            if (objects[i].bounds.y < 0 || objects[i].bounds.y + objects[i].bounds.height > SCREEN_HEIGHT)
            {
                objects[i].velocity.y *= -1;
            }
        }

        QuadtreeNode *root = createNode((Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, CAPACITY);
        if (!root)
        {
            printf("Failed to create root node\n");
            break;
        }
        for (int i = 0; i < numObjects; i++)
        {
            insert(root, &objects[i]);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        drawQuadtree(root);

        for (int i = 0; i < numObjects; i++)
        {
            GameObject *potentialColliders[MAX_OBJECTS];
            int count = 0;
            query(root, objects[i].bounds, potentialColliders, &count);

            bool colliding = false;
            for (int j = 0; j < count; j++)
            {
                if (potentialColliders[j] != &objects[i] &&
                    CheckCollisionRecs(objects[i].bounds, potentialColliders[j]->bounds))
                {
                    colliding = true;
                    break;
                }
            }
            DrawRectangleRec(objects[i].bounds, colliding ? RED : BLUE);
        }

        DrawFPS(10, 10);
        EndDrawing();

        freeQuadtree(root);
    }

    CloseWindow();
    return 0;
}