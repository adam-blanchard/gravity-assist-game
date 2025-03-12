void predictTimesteps(Ship *playerShip, Body *bodies, int numBodies)
{
    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        for (int j = 0; j < numBodies; j++)
        {
            Vector2 force = {0};
            Vector2 body1Position = {0};
            Vector2 body1Velocity = {0};
            Vector2 body2Position = {0};
            Vector2 body2Velocity = {0};

            if (i > 0)
            {
                body1Position = bodies[j].futurePositions[i - 1];
                body1Velocity = bodies[j].futureVelocities[i - 1];
            }
            else
            {
                body1Position = bodies[j].position;
                body1Velocity = bodies[j].velocity;
            }

            for (int k = 0; k < numBodies; k++)
            {
                if (i > 0)
                {
                    body2Position = bodies[k].futurePositions[i - 1];
                    body1Velocity = bodies[k].futureVelocities[i - 1];
                }
                else
                {
                    body2Position = bodies[k].position;
                    body1Velocity = bodies[k].velocity;
                }

                if (j != k)
                {
                    // Vector2 planetaryForce = gravitationalForce(&bodies[j], &bodies[k]);
                    Vector2 planetaryForce = gravitationalForce2(&body1Position, &body2Position, bodies[j].mass, bodies[k].mass);
                    force = _Vector2Add(&force, &planetaryForce);
                }
            }

            Vector2 acceleration = {force.x / bodies[j].mass, force.y / bodies[j].mass};

            bodies[j].futureVelocities[i].x += acceleration.x * TRAJECTORY_STEP_TIME;
            bodies[j].futureVelocities[i].y += acceleration.y * TRAJECTORY_STEP_TIME;

            bodies[j].futurePositions[i].y += bodies[j].futureVelocities[i].y * TRAJECTORY_STEP_TIME;
            bodies[j].futurePositions[i].x += bodies[j].futureVelocities[i].x * TRAJECTORY_STEP_TIME;
        }
    }
}

void predictTrajectory(Vector2 *initialPosition, Vector2 *initialVelocity, float *mass, Body *otherBodies, int numBodies, Vector2 *trajectory)
{
    float radiusThreshold = 5.0f;
    Vector2 currentPosition = *initialPosition;
    Vector2 currentVelocity = *initialVelocity;

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 totalForce = {0};
        for (int j = 0; j < numBodies; j++)
        {
            if (calculateDistance(&currentPosition, &otherBodies[j].position) > radiusThreshold)
            {
                Vector2 force = gravitationalForce2(&currentPosition, &otherBodies[j].position, *mass, otherBodies[j].mass);
                totalForce = _Vector2Add(&totalForce, &force);
            }
        }

        Vector2 acceleration = {totalForce.x / *mass, totalForce.y / *mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

void predictShipTrajectory(Ship *playerShip, Body *otherBodies, int numBodies, Vector2 *trajectory)
{
    // Isolated prediction of the ship based on current position and velocity
    // TODO: improve trajectory accuracy by simulating all bodies x timesteps in advance
    Vector2 currentPosition = playerShip->position;
    Vector2 currentVelocity = playerShip->velocity;

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 force = {0};

        for (int j = 0; j < numBodies; j++)
        {
            Vector2 planetaryForce = gravitationalForce2(&currentPosition, &otherBodies[j].position, playerShip->mass, otherBodies[j].mass);
            force = _Vector2Add(&force, &planetaryForce);
        }

        Vector2 acceleration = {force.x / playerShip->mass, force.y / playerShip->mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

void predictBodyTrajectory(Body *body, Body *otherBodies, int numBodies, Vector2 *trajectory)
{
    Vector2 currentPosition = body->position;
    Vector2 currentVelocity = body->velocity;

    for (int i = 0; i < TRAJECTORY_STEPS; i++)
    {
        Vector2 totalForce = {0};
        for (int j = 0; j < numBodies; j++)
        {
            if (&otherBodies[j] != body)
            {
                Vector2 force = gravitationalForce2(&currentPosition, &otherBodies[j].position, body->mass, otherBodies[j].mass);
                totalForce = _Vector2Add(&totalForce, &force);
            }
        }

        Vector2 acceleration = {totalForce.x / body->mass, totalForce.y / body->mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, TRAJECTORY_STEP_TIME);
        currentVelocity = _Vector2Add(&currentVelocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(currentVelocity, TRAJECTORY_STEP_TIME);
        currentPosition = _Vector2Add(&currentPosition, &scaledVelocity);

        trajectory[i] = currentPosition;
    }
}

void updateShip(Ship *ship, int n, Body *bodies, float dt)
{
    ship->activeTexture = &ship->idleTexture;
    if (ship->state == SHIP_FLYING)
    {
        // Apply rotation
        if (IsKeyDown(KEY_D))
            ship->rotation += 180.0f * dt; // Rotate right
        if (IsKeyDown(KEY_A))
            ship->rotation -= 180.0f * dt; // Rotate left

        // Normalize rotation to keep it within 0-360 degrees
        ship->rotation = fmod(ship->rotation + 360.0f, 360.0f);
    }

    // Apply thrust
    if (IsKeyDown(KEY_W) && ship->fuel > 0)
    {
        // Convert rotation to radians for vector calculations
        float radians = ship->rotation * PI / 180.0f;
        Vector2 thrustDirection = {sinf(radians), -cosf(radians)}; // Negative cos because Y increases downward
        Vector2 thrust = _Vector2Scale(thrustDirection, ship->thrust * dt);
        ship->velocity = _Vector2Add(&ship->velocity, &thrust);
        ship->activeTexture = &ship->thrustTexture;
        ship->fuel -= ship->fuelConsumption;

        if (ship->state == SHIP_LANDED)
        {
            ship->state = SHIP_FLYING;
            ship->landedBody = NULL;
        }
    }

    ship->fuel = _Clamp(ship->fuel, 0.0f, 100.0f);

    Vector2 force = {0};

    if (ship->state == SHIP_FLYING)
    {
        for (int i = 0; i < n; i++)
        {
            Vector2 planetaryForce = gravitationalForce2(&ship->position, &bodies[i].position, ship->mass, bodies[i].mass);
            force = _Vector2Add(&force, &planetaryForce);
        }

        Vector2 acceleration = {force.x / ship->mass, force.y / ship->mass};

        Vector2 scaledAcceleration = _Vector2Scale(acceleration, dt);
        ship->velocity = _Vector2Add(&ship->velocity, &scaledAcceleration);

        Vector2 scaledVelocity = _Vector2Scale(ship->velocity, dt);
        ship->position = _Vector2Add(&ship->position, &scaledVelocity);
    }

    if (ship->state == SHIP_LANDED)
    {
        Vector2 sub = _Vector2Subtract(&ship->position, &ship->landedBody->position);
        Vector2 norm = _Vector2Normalize(sub);
        Vector2 scale = _Vector2Scale(norm, ship->landedBody->radius + ship->colliderRadius);
        ship->position = _Vector2Add(&ship->landedBody->position, &scale);
        ship->velocity = ship->landedBody->velocity; // Only sync here if not taking off
    }
}