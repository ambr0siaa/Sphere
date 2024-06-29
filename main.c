#include <stdio.h>
#include <string.h>

#define LA_IMPLEMENTATION
#include "la.h"

#define f64 double

#define WIDTH 64
#define HEIGHT 32
#define RADIUS 1

#define ASPECT ((f64)WIDTH / (f64)HEIGHT)
#define COL_ASPECT (11.0f / 24.0f)

#define PI 3.1415926535

static char screen[HEIGHT][WIDTH] = {0};
static const char *grad = " .:!/r(l1Z4H9W8$@";
static size_t grad_size;

static void show(void)
{
    for (size_t i = 0; i < HEIGHT; ++i) {
        for (size_t j = 0; j < WIDTH; ++j)
            printf("%c", screen[i][j]);
        printf("\n");
    }
}

// put cursor to the begining of terminal
static void back(void)
{
    printf("\x1b[%dD", WIDTH);
    printf("\x1b[%dA", HEIGHT);
}

double v3d_dot(V3d a, V3d b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

V3d v3d_norm(V3d v)
{
    return v3d_div(v, v3dd(v3d_len(v)));
}

double sph_intersection(V3d o, V3d n, V3d c, double r)
{
    double a = v3d_dot(n, n);
    double p = v3d_dot(o, n) - v3d_dot(n, c);
    double q = v3d_dot(o, o) + v3d_dot(c, c) - 2 * v3d_dot(o, c) - r * r;
    double d = p * p - a * q;
    if (d < 0.0) return -1.0;
    d = sqrt(d) / a;
    p = p / a;
    return -p - d;
}

static char sphere_draw(V2d uv, V3d o, V3d c, V3d l)
{
    int color = 0;
    V3d n = v3d_norm(v3d(1.0, uv.x, uv.y));
    f64 t = sph_intersection(o, n, c, RADIUS);
    if (t > 0) {
        V3d v = v3d_norm(v3d_sum(o, v3d_mul(n, v3dd(t))));
        double diff = v3d_dot(v, l);
        color = (int)(diff * 18.0);
    }
    color = clampi(color, 0, grad_size);
    return grad[color];
}

int main(void)
{
    grad_size = strlen(grad) - 2;
    V3d origin = { -2.0, 0.0, 0.0 }; // Origin
    V3d center = {  0.0, 0.0, 0.0 }; // Sphere center

    for (f64 t = 0.0; t < 2*PI; t += 0.01) {
        V3d light = v3d_norm(v3d(sin(3*t), cos(3*t), -0.5));
        for (size_t i = 0; i < WIDTH; ++i) {
            for (size_t j = 0; j < HEIGHT; ++j) {
                V2d uv = v2d((f64)(i) / (f64)(WIDTH) * 2.0 - 1.0, (f64)(j) / (f64)(HEIGHT) * 2.0 - 1.0);
                uv.x *= ASPECT * COL_ASPECT;

                char pixel = sphere_draw(uv, origin, center, light);
                screen[j][i] = pixel;
            }
            back();
            show();
        }
    }
}
