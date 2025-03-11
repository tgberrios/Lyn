#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Forward declarations
struct Point;
struct Vector3;
struct Shape;
struct Circle;

// Structure definitions
struct Point {
    float x, y;
};

struct Vector3 {
    float x, y, z;
};

struct Shape {
    float x, y;
};

struct Circle {
    struct Shape _base;
    float radius;
};

// Function declarations
struct Point* new_Point(void);
void Point_init(struct Point* self, float x, float y);
float Point_distance(struct Point* self, struct Point* other);

struct Vector3* new_Vector3(void);
void Vector3_init(struct Vector3* self, float x, float y, float z);
float Vector3_magnitude(struct Vector3* self);
struct Vector3* Vector3_add(struct Vector3* self, struct Vector3* other);

struct Shape* new_Shape(void);
void Shape_init(struct Shape* self, float x, float y);

struct Circle* new_Circle(void);
void Circle_init(struct Circle* self, float x, float y, float r);
float Circle_area(struct Circle* self);
void Circle_scale(struct Circle* self, float factor);

// Constructors and methods implementation
struct Point* new_Point(void) {
    return calloc(1, sizeof(struct Point));
}

void Point_init(struct Point* self, float x, float y) {
    self->x = x;
    self->y = y;
}

float Point_distance(struct Point* self, struct Point* other) {
    float dx = self->x - other->x;
    float dy = self->y - other->y;
    return sqrt(dx * dx + dy * dy);
}

struct Vector3* new_Vector3(void) {
    return calloc(1, sizeof(struct Vector3));
}

void Vector3_init(struct Vector3* self, float x, float y, float z) {
    self->x = x;
    self->y = y;
    self->z = z;
}

float Vector3_magnitude(struct Vector3* self) {
    return sqrt(self->x * self->x + self->y * self->y + self->z * self->z);
}

struct Shape* new_Shape(void) {
    return calloc(1, sizeof(struct Shape));
}

void Shape_init(struct Shape* self, float x, float y) {
    self->x = x;
    self->y = y;
}

struct Circle* new_Circle(void) {
    return calloc(1, sizeof(struct Circle));
}

void Circle_init(struct Circle* self, float x, float y, float r) {
    Shape_init((struct Shape*)self, x, y);
    self->radius = r;
}

float Circle_area(struct Circle* self) {
    return 3.14159f * self->radius * self->radius;
}

void Circle_scale(struct Circle* self, float factor) {
    self->radius *= factor;
}

int main(void) {
    struct Point *p1, *p2;
    struct Vector3 *v1, *v2;
    struct Circle *c1;

    printf("%s\n", "=== Testing Point ===");
    p1 = new_Point();
    Point_init(p1, 0, 0);
    p2 = new_Point();
    Point_init(p2, 3, 4);
    printf("%g\n", (double)(Point_distance(p1, p2)));
    printf("%s\n", "=== Testing Vector3 ===");
    v1 = new_Vector3();
    Vector3_init(v1, 1, 2, 2);
    v2 = new_Vector3();
    Vector3_init(v2, 2, 3, 6);
    printf("%g\n", (double)(Vector3_magnitude(v1)));
    printf("%g\n", (double)(Vector3_magnitude(v2)));
    printf("%s\n", "=== Testing Circle ===");
    c1 = new_Circle();
    Circle_init(c1, 0, 0, 5);
    printf("%g\n", (double)(Circle_area(c1)));
    Circle_scale(c1, 2);
    printf("%g\n", (double)(Circle_area(c1)));
    return 0;
}
