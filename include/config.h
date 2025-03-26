#ifndef PI
#define PI 3.14159265358979323846f
#endif
// G is usually 6.67430e-11
// G is the strength of gravity - higher is stronger
#ifndef G
#define G 6.67430e-2
#endif

#ifndef MASS_SCALE
#define MASS_SCALE 1e-5
#endif

#ifndef RADIUS_SCALE
#define RADIUS_SCALE 1e-1
#endif

#ifndef ORBIT_SCALE
#define ORBIT_SCALE 1e-3
#endif

#ifndef PREVIOUS_POSITIONS
#define PREVIOUS_POSITIONS 1000
#endif
#ifndef FUTURE_POSITIONS
#define FUTURE_POSITIONS 5000
#endif
#ifndef FUTURE_STEP_TIME
#define FUTURE_STEP_TIME 0.1f
#endif
#ifndef GRID_SPACING
#define GRID_SPACING 1e2
#endif
#ifndef GRID_LINE_WIDTH
#define GRID_LINE_WIDTH 100
#endif
#ifndef HUD_ARROW_SCALE
#define HUD_ARROW_SCALE 0.01
#endif
#ifndef MAX_LANDING_SPEED
#define MAX_LANDING_SPEED 1e4
#endif
#ifndef MAX_CAPACITY
#define MAX_CAPACITY 100
#endif
#ifndef HUD_FONT_SIZE
#define HUD_FONT_SIZE 16
#endif

#ifndef TRAIL_COLOUR
#define TRAIL_COLOUR \
    CLITERAL(Color){255, 255, 255, 255}
#endif

#ifndef GRID_COLOUR
#define GRID_COLOUR \
    CLITERAL(Color){255, 255, 255, 50}
#endif

#ifndef SPACE_COLOUR
#define SPACE_COLOUR \
    CLITERAL(Color){10, 10, 10, 255}
#endif

#ifndef GUI_BACKGROUND
#define GUI_BACKGROUND \
    CLITERAL(Color){255, 255, 255, 200}
#endif

#ifndef ORBIT_COLOUR
#define ORBIT_COLOUR \
    CLITERAL(Color){255, 255, 255, 100}
#endif