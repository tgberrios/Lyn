#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Point {
    float x;
    float y;
} Point;

void init(void *x, void *y) {
    self.x = x;
    self.y = y;
}

float distance(void *other) {
    dx = (self.x - other.x);
    dy = (self.y - other.y);
    return ((dx * dx) + (dy * dy));
}

p1 = Point(0, 0);
p2 = Point(3, 4);
printf("%g\n", (double)(distance(p1, p2)));
