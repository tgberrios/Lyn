#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Estructuras de datos para los objetos
typedef struct {
    double x;
    double y;
} Point;
typedef struct {
    double x;
    double y;
    double z;
} Vector3;
typedef struct {
    int type;     // 0 = base Shape, 1 = Circle, etc.
    double x;
    double y;
} Shape;
typedef struct {
    int type;     // Will always be 1 for Circle
    double x;
    double y;
    double radius;
} Circle;

// Funciones para crear objetos
Point* new_Point() {
    Point* p = (Point*)malloc(sizeof(Point));
    if (!p) {
        fprintf(stderr, "Error: Memory allocation failed for Point\n");
        exit(1);
    }
    p->x = 0.0;
    p->y = 0.0;
    return p;
}
Vector3* new_Vector3() {
    Vector3* v = (Vector3*)malloc(sizeof(Vector3));
    if (!v) {
        fprintf(stderr, "Error: Memory allocation failed for Vector3\n");
        exit(1);
    }
    v->x = 0.0;
    v->y = 0.0;
    v->z = 0.0;
    return v;
}
Shape* new_Shape() {
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (!s) {
        fprintf(stderr, "Error: Memory allocation failed for Shape\n");
        exit(1);
    }
    s->type = 0;  // Base Shape type
    s->x = 0.0;
    s->y = 0.0;
    return s;
}
Circle* new_Circle() {
    Circle* c = (Circle*)malloc(sizeof(Circle));
    if (!c) {
        fprintf(stderr, "Error: Memory allocation failed for Circle\n");
        exit(1);
    }
    c->type = 1;  // Circle type
    c->x = 0.0;
    c->y = 0.0;
    c->radius = 0.0;
    return c;
}
// Funciones de clases
void Point_init(Point* self, double x, double y) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Point_init\n");
        return;
    }
    self->x = x;
    self->y = y;
}
double Point_distance(Point* self, Point* other) {
    if (!self || !other) {
        fprintf(stderr, "Error: NULL pointer in Point_distance\n");
        return 0.0;
    }
    double dx = self->x - other->x;
    double dy = self->y - other->y;
    double result = sqrt(dx * dx + dy * dy);
    return result;
}
void Vector3_init(Vector3* self, double x, double y, double z) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Vector3_init\n");
        return;
    }
    self->x = x;
    self->y = y;
    self->z = z;
}
double Vector3_magnitude(Vector3* self) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Vector3_magnitude\n");
        return 0.0;
    }
    double x2 = self->x * self->x;
    double y2 = self->y * self->y;
    double z2 = self->z * self->z;
    double sum = x2 + y2 + z2;
    double result = sqrt(sum);
    return result;
}
Vector3* Vector3_add(Vector3* self, Vector3* other) {
    if (!self || !other) {
        fprintf(stderr, "Error: NULL pointer in Vector3_add\n");
        return NULL;
    }
    Vector3* result = new_Vector3();
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in Vector3_add\n");
        return NULL;
    }
    result->x = self->x + other->x;
    result->y = self->y + other->y;
    result->z = self->z + other->z;
    return result;
}
void Shape_init(Shape* self, double x, double y) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Shape_init\n");
        return;
    }
    self->x = x;
    self->y = y;
}
double Shape_area(Shape* self) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Shape_area\n");
        return 0.0;
    }
    return 0.0; // Base shape has no area
}
void Circle_init(Circle* self, double x, double y, double r) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Circle_init\n");
        return;
    }
    self->type = 1;  // Circle type
    self->x = x;
    self->y = y;
    self->radius = r;
}
double Circle_area(Circle* self) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Circle_area\n");
        return 0.0;
    }
    const double PI = 3.14159265358979323846;
    double r = self->radius;
    double area = PI * r * r;
    return area;
}
void Circle_scale(Circle* self, double factor) {
    if (!self) {
        fprintf(stderr, "Error: NULL pointer in Circle_scale\n");
        return;
    }
    self->radius = self->radius * factor;
}
int main() {
    Point* p1 = NULL;
    Point* p2 = NULL;
    Vector3* v1 = NULL;
    Vector3* v2 = NULL;
    Circle* c1 = NULL;
    printf("=== Testing Point ===\n");
    p1 = new_Point();
    p2 = new_Point();
    if (!p1 || !p2) {
        fprintf(stderr, "Error: Failed to allocate Points\n");
        goto cleanup;
    }
    Point_init(p1, 0.0, 0.0);
    Point_init(p2, 3.0, 4.0);
    // Calculando distancia euclidiana entre (0,0) y (3,4): sqrt(3^2 + 4^2) = sqrt(9 + 16) = sqrt(25) = 5
    printf("Point distance (0,0) to (3,4): ");
    printf("%.6f\n", Point_distance(p1, p2));
    printf("=== Testing Vector3 ===\n");
    v1 = new_Vector3();
    v2 = new_Vector3();
    if (!v1 || !v2) {
        fprintf(stderr, "Error: Failed to allocate Vectors\n");
        goto cleanup;
    }
    Vector3_init(v1, 1.0, 2.0, 2.0);
    Vector3_init(v2, 2.0, 3.0, 6.0);
    printf("Vector3 (1,2,2) magnitude: ");
    printf("%.6f\n", Vector3_magnitude(v1));
    printf("Vector3 (2,3,6) magnitude: ");
    printf("%.6f\n", Vector3_magnitude(v2));
    printf("=== Testing Circle ===\n");
    c1 = new_Circle();
    if (!c1) {
        fprintf(stderr, "Error: Failed to allocate Circle\n");
        goto cleanup;
    }
    Circle_init(c1, 0.0, 0.0, 5.0);
    printf("Circle area with radius=5: ");
    printf("%.6f\n", Circle_area(c1));
    Circle_scale(c1, 2.0);
    printf("Circle area after scale(2) with radius=10: ");
    printf("%.6f\n", Circle_area(c1));
    cleanup:
    if (p1) free(p1);
    if (p2) free(p2);
    if (v1) free(v1);
    if (v2) free(v2);
    if (c1) free(c1);
    return 0;
}
