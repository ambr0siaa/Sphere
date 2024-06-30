#include <stdio.h>
#include <string.h>

#define LA_IMPLEMENTATION
#include "la.h"

#define WIDTH 64
#define HEIGHT 32
#define RADIUS 1.0f

#define W 1.2f                       // Angular velocity
#define DIFF_SCALER 18.0f            // Scale to get correct color
#define PIXEL_ASPECT (11.0f / 24.0f) // Ratio of symbols in console
#define ASPECT (float)(WIDTH / HEIGHT)

static char screen[WIDTH * HEIGHT] = {0};

static const char *grad = " .:!/r(l1Z4H9W8$@";
static size_t grad_size;

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
*          else return the most nearest point (when t = -p - sqrt(p * p - q))
*
*          Final formula is:
*              p = o * o - n * c
*              q = o * o + c * c - 2 * o * c - r^2
*              d = p * p - q
*              t = -p - sqrt(d)
*
*/
float sph_intersec(V3f o, V3f n, V3f c, float r) {
    float p = v3f_dot(o, n) - v3f_dot(n, c);
    float q = v3f_dot(o, o) + v3f_dot(c, c) - 2.0f * v3f_dot(o, c) - r * r;
    float d = p * p - q;
    if (d < 0.0f) return -1.0f;
    d = sqrt(d);
    return -p - d;
}

char sphere_draw(V2f uv, V3f o, V3f c, V3f l)
{
    int color = 0;
    V3f n = v3f_norm(v3f(1.0f, uv.x, uv.y)); // direction of ray
    float t = sph_intersec(o, n, c, RADIUS);
    if (t > 0.0f) {
        V3f ray = v3f_norm(v3f_sum(o, v3f_mul(n, v3ff(t))));
        float diff = v3f_dot(ray, l);
        color = (int)(diff * DIFF_SCALER);
    }
    color = clampi(color, 0, grad_size);
    return grad[color];
}

int main(void)
{
    grad_size = strlen(grad) - 2;
    V3f origin = { -2.0f, 0.0f, 0.0f }; // Origin of ray
    V3f center = {  0.0f, 0.0f, 0.0f }; // Sphere center

    for (float t = 0.0f;; t += 0.01f) {
        V3f light = v3f_norm(v3f(sin(W*t), cos(W*t), -0.5f));
        for (size_t i = 0; i < WIDTH; ++i) {
            for (size_t j = 0; j < HEIGHT; ++j) {
                V2f uv = v2f((float)(i) / (float)(WIDTH) * 2.0f - 1.0f,
                             (float)(j) / (float)(HEIGHT) * 2.0f - 1.0f);
                uv.x *= ASPECT * PIXEL_ASPECT; // Aligns x axis coordinates

                char pixel = sphere_draw(uv, origin, center, light);
                screen[j * WIDTH + i] = pixel;
            }
            screen_update();
            screen_show();
        }
    }
}
