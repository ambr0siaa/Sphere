#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h> // Use for detecteing keys pressing

#define LA_IMPLEMENTATION
#include "la.h"

#define WIDTH 64
#define HEIGHT 32

#define PIXEL_ASPECT (11.0f / 24.0f)    // Ratio of symbols in console
#define ASPECT (float)(WIDTH / HEIGHT)
#define DIFF_SCALER 18.0f               // Scale to get correct color

#define DISABLE_CURSOR printf("\e[?25l")
#define ENABLE_CURSOR printf("\e[?25h")

#define V3F_ZERO (V3f) {0}
#define SPH_CENTER (V3f) {0.0f, 0.0f, 0.0f}
#define RADIUS 1.0f
#define W 0.04f                          // Angular velocity

#define CAMERA_POS (V3f) {0.0f, 0.0f, -2.0f}

#define GO_FORWARD  'w'
#define GO_BACKWARD 's'
#define GO_RIGHT    'd'
#define GO_LEFT     'a'
#define GO_DOWN     'j'
#define GO_UP       'k'
#define ROT_RIGHT   'l'
#define ROT_LEFT    'h'
#define ZOOM_IN     'z'
#define ZOOM_OUT    'x'
#define TURN_OFF    '\033' // To break animation put ESC

#define SCAN_START \
    initscr(); \
    cbreak(); \
    noecho(); \
    scrollok(stdscr, TRUE); \
    nodelay(stdscr, TRUE)

#define SCAN_END endwin()

typedef struct {
    V3f origin;
    V3f direct;
    float zoom;
} Camera;

static char screen[WIDTH * HEIGHT];

static const char *grad = " .:!/(l146ZH9W8$@";
static size_t grad_size = 17 - 2; // minus 2 to correct displaing

/*
* To calculate intersection with sphere need a few steps:
*    1) Make ray vector in parametric form:
*           v = o + n*t, where o - origin of ray; n - direction; t - parameter
*
*    2) Write sphere equation in vector form:
*          (v - c)^2 = r^2, where v - variable coordinates (x, y, z); c - center; r - radius
*
*    3) Just put ray into sphere equation:
*          (o + n*t - c)^2 = r^2
*
*       Next note like a * b means dot product
*
*    4) After few calculations get end formula:
*          a = n * n
*          p = o * n - n * c
*          q = a * (o * o + c * c - 2 * o * c - r^2)
*          D = 4 * (p * p - q)
*
*          t(1;2) = (-p/a) +- sqrt(p * p - q)/a
*
*          But n is normalized vector and dot product is 1.0f and we need 1 root consequently if t < 0.0f return -1.0f (it detects that color is blank),
*          else return the most nearest point (when t = -p - sqrt(p * p - q)).
*          Well, to simplify formula we'll assume that c is zero vector and later just substitute center from ray
*
*          Final formula is:
*              p = o * o
*              q = o * o - r * r
*              d = p * p - q
*              t = -p - sqrt(d)
*
*/
float sphere(V3f o, V3f n, float r) {
    o = v3f_sub(o, SPH_CENTER);
    float p = v3f_dot(o, n);
    float q = v3f_dot(o, o) - r * r;
    float d = p * p - q;
    if (d < 0.0f) return -1.0f;
    d = sqrt(d);
    return -p - d;
}

Camera camera(V3f origin, V3f direct)
{
    Camera cam = {0};
    cam.origin = origin;
    cam.direct = direct;
    cam.zoom = 1.0f;
    return cam;
}

// Rotate right
V3f rotXZ(V3f v, float theta)
{
    V3f w = v;
    w.x = v.x * cosf(theta) - v.z * sinf(theta);
    w.z = v.z * cosf(theta) + v.x * sinf(theta);
    return w;
}

// Rotate left
V3f rotZX(V3f v, float theta)
{
    V3f w = v;
    w.x = v.x * cosf(theta) + v.z * sinf(theta);
    w.z = v.z * cosf(theta) - v.x * sinf(theta);
    return w;
}

// Display screen by rows into terminal
void screen_show(void)
{
    static char raw[WIDTH];
    for (size_t i = 0; i < HEIGHT; ++i) {
        memcpy(raw, screen + i * WIDTH, WIDTH);
        fwrite(raw, WIDTH, 1, stdout);
        fputc('\n', stdout);
    }
}

// Puts cursor to the begining of terminal
void screen_update(void)
{
    printf("\x1b[%dD", WIDTH);
    printf("\x1b[%dA", HEIGHT);
}

void screen_off(void)
{
    SCAN_END;
    char *ptr = screen;
    size_t n = WIDTH * HEIGHT;
    while (n-- > 0) *ptr++ = grad[0];
    ENABLE_CURSOR;
    exit(0);
}

void camera_ctrl(Camera *c, float theta)
{
    SCAN_START;

    switch(getch()) {
        case ZOOM_IN:     c->zoom += 0.1f;     break;
        case ZOOM_OUT:    c->zoom -= 0.1f;     break;
        case GO_FORWARD:  c->origin.z += 0.1f; break;
        case GO_BACKWARD: c->origin.z -= 0.1f; break;
        case GO_LEFT:     c->origin.x -= 0.1f; break;
        case GO_RIGHT:    c->origin.x += 0.1f; break;
        case GO_UP:       c->origin.y += 0.1f; break;
        case GO_DOWN:     c->origin.y -= 0.1f; break;
        case ROT_RIGHT: {
            c->origin = rotXZ(c->origin, 0.35f);
            c->direct = rotXZ(c->direct, theta * W);
            break;
        }
        case ROT_LEFT: {
            c->origin = rotZX(c->origin, 0.35f);
            c->direct = rotZX(c->direct, theta * W);
            break;
        }
        case TURN_OFF:    screen_off(); break;
        default: break;
    }

    SCAN_END;
}

// Right handed coordinate system
V2f coordinates(size_t i, size_t j)
{
    V2f uv = {0};
    uv.x = (float)(i) / (float)(WIDTH) * 2.0f - 1.0f;
    uv.y = (float)(j) / (float)(HEIGHT) * (-2.0f) + 1.0f;
    uv.x *= ASPECT * PIXEL_ASPECT; // Aligns x axis coordinates
    return uv;
}

V3f rotateY(V3f a, float angle)
{
    V3f b = a;
    b.x = a.x * cosf(angle) - a.z * sinf(angle);
    b.z = a.x * sinf(angle) + a.z * cosf(angle);
    return b;
}

float plane(V3f o, V3f n, V3f N)
{
    return -v3f_dot(o, N) / v3f_dot(n, N);
}

int main(void)
{
    DISABLE_CURSOR;

    Camera c = camera(CAMERA_POS, V3F_ZERO);
    V3f light = v3f_norm(v3f(0.5f, 1.0f, -1.0f));

    for (int phi = 0;; ++phi) {
        for (size_t i = 0; i < WIDTH; ++i) {
            for (size_t j = 0; j < HEIGHT; ++j) {
                V2f uv = coordinates(i, j);
                c.direct = v3f_norm(v3f(uv.x, uv.y, c.zoom));
                camera_ctrl(&c, phi);
                float T = 9999.0f;
                int color = 0;
                float diff = 0.0f;

                float t = sphere(c.origin, c.direct, RADIUS);
                if (t > 0.0f) T = t;
                
                if (T < 9999.0f) {
                    V3f ray = v3f_sum(c.origin, v3f_mul(c.direct, v3ff(T)));
                    diff = v3f_dot(ray, light);
                }
                
                color = (int)(diff * DIFF_SCALER);
                color = clampi(color, 0, grad_size);
                screen[j * WIDTH + i] = grad[color];
            }
            screen_show();
            screen_update();
        }
    }

    return 0;
}
