#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LA_IMPLEMENTATION
#include "la.h"

// Display proporties
#define WIDTH        64
#define HEIGHT       30
#define COLOR        '@'
#define BLANK        ' '
#define ASPECT       (float)WIDTH / (float)HEIGHT
#define PIXEL_ASPECT (11.0f / 26.0f) // Ratio of symbols in console

// State prorties
#define FPS      120
#define BOUNDARY 1.0f
#define FRICTION 0.98f
#define RADIUS   0.32f
#define dt       0.01f
#define g        9.81f

#define BOTTOM -BOUNDARY + RADIUS // Boundary of floor
#define RIGHT   BOUNDARY + RADIUS // Boundary of right wall of screen

static char screen[WIDTH * HEIGHT];

void screen_update(void)
{
    printf("\x1b[%dD", WIDTH);
    printf("\x1b[%dA", HEIGHT);
    usleep(1000 * 1000 / FPS);
}

void screen_show(void)
{
    char raw[WIDTH];
    for (size_t i = 0; i < HEIGHT; ++i) {
        memcpy(raw, screen + i * WIDTH, WIDTH);
        fwrite(raw, WIDTH, 1, stdout);
        fputc('\n', stdout);
    }
}

// Right handed coordinates
V2f convert_coords(size_t x, size_t y)
{
    V2f uv = {0};
    uv.x = (float)x / WIDTH * 2.0f - 1.0f;
    uv.y = (float)y / HEIGHT * (-2.0f) + 1.0f;
    uv.x *= ASPECT * PIXEL_ASPECT; // Aligns by screen and symbols in console
    return uv;
}

void circle_draw(V2f c, float r)
{
    for (size_t i = 0; i < WIDTH; ++i) {
        for (size_t j = 0; j < HEIGHT; ++j) {
            char color = BLANK;
            V2f uv = convert_coords(i, j);
            uv = v2f(uv.x - c.x, uv.y - c.y);
            if (v2f_sqrlen(uv) < r*r) color = COLOR;
            screen[j * WIDTH + i] = color;
        }
    }
}

int main(void)
{
    V2f pos = v2f(-0.4f, 0.6f); // Position of ball
    V2f vel = v2f(1.0f, 0.0f);  // Velocity
    
    for (;;) {
        vel = v2f_sum(vel, v2f(0.0f, -g * dt));
        pos = v2f_sum(pos, v2f_mul(vel, v2ff(dt)));

        if (pos.y < BOTTOM) {
            vel = v2f_mul(vel, v2f(FRICTION, -FRICTION));
            pos.y = BOTTOM;
        }

        if (pos.x > RIGHT)
            pos.x = -BOUNDARY - RADIUS;

        circle_draw(pos, RADIUS);
        screen_show();
        screen_update();
    }
    
    return 0;
}
