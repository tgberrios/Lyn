#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Forward declarations
struct Point;
struct Shape;
struct Circle;
struct Vector3;
void* new_Point();
void Point_init(void* self, double x, double y);
double Point_distance(void* self, void* other);
void* new_Shape();
void Shape_init(void* self, double x, double y);
double Shape_area(void* self);
void* new_Circle();
void Circle_init(void* self, double x, double y, double r);
double Circle_area(void* self);
void Circle_scale(void* self, double factor);
void* new_Vector3();
void Vector3_init(void* self, double x, double y, double z);
double Vector3_magnitude(void* self);
void* Vector3_add(void* self, void* other);

struct Point {
    double x;
    double y;
};
struct Shape {
    double x;
    double y;
};
struct Circle {
    double x;
    double y;
    double radius;
};
struct Vector3 {
    double x;
    double y;
    double z;
};

void* new_Point() {
    void* ptr = malloc(sizeof(struct Point));
    if (ptr) memset(ptr, 0, sizeof(struct Point));
    return ptr;
}
void* new_Shape() {
    void* ptr = malloc(sizeof(struct Shape));
    if (ptr) memset(ptr, 0, sizeof(struct Shape));
    return ptr;
}
void* new_Circle() {
    void* ptr = malloc(sizeof(struct Circle));
    if (ptr) memset(ptr, 0, sizeof(struct Circle));
    return ptr;
}
void* new_Vector3() {
    void* ptr = malloc(sizeof(struct Vector3));
    if (ptr) memset(ptr, 0, sizeof(struct Vector3));
    return ptr;
}

// Class declaration: Point
// Class declaration: Vector3
// Class declaration: Shape
// Class declaration: Circle
void Point_init(void* self, double x, double y) {
    if (!self) return;
    struct Point* p = (struct Point*)self;
    p->x = x;
    p->y = y;
}
void Vector3_init(void* self, double x, double y, double z) {
    if (!self) return;
    struct Vector3* v = (struct Vector3*)self;
    v->x = x;
    v->y = y;
    v->z = z;
}
void Shape_init(void* self, double x, double y) {
    if (!self) return;
    struct Shape* s = (struct Shape*)self;
    s->x = x;
    s->y = y;
}
void Circle_init(void* self, double x, double y, double r) {
    if (!self) return;
    struct Circle* circle = (struct Circle*)self;
    circle->x = x;
    circle->y = y;
    circle->radius = r;
}
double Point_distance(void* self, void* other) {
    if (!self || !other) return 0.0;
    struct Point* p1 = (struct Point*)self;
    struct Point* p2 = (struct Point*)other;
    double dx = p1->x - p2->x;
    double dy = p1->y - p2->y;
    return sqrt(dx * dx + dy * dy);
}
double Vector3_magnitude(void* self) {
    if (!self) return 0.0;
    struct Vector3* v = (struct Vector3*)self;
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
double Circle_area(void* self) {
    if (!self) return 0.0;
    struct Circle* circle = (struct Circle*)self;
    return 3.14159 * circle->radius * circle->radius;
}
void Circle_scale(void* self, double factor) {
    if (!self) return;
    struct Circle* circle = (struct Circle*)self;
    circle->radius *= factor;
}
double Shape_area(void* self) {
    if (!self) return 0.0;
    return 0.0; // Base Shape has no area
}
void* Vector3_add(void* self, void* other) {
    if (!self || !other) return NULL;
    struct Vector3* v1 = (struct Vector3*)self;
    struct Vector3* v2 = (struct Vector3*)other;
    struct Vector3* result = malloc(sizeof(struct Vector3));
    if (!result) return NULL; // Verificar asignación de memoria
    memset(result, 0, sizeof(struct Vector3)); // Inicializar a cero
    result->x = v1->x + v2->x;
    result->y = v1->y + v2->y;
    result->z = v1->z + v2->z;
    return result;
}

int main() {
    void *p1 = NULL, *p2 = NULL, *v1 = NULL, *v2 = NULL, *c1 = NULL;
    printf("%s\n", "=== Testing Point ===");
    p1 =     new_Point()    ;
    p2 =     new_Point()    ;
    if (p1) printf("%g\n",     Point_distance(    p1    ,     p2    )    ); else printf("NULL\n");
    printf("%s\n", "=== Testing Vector3 ===");
    v1 =     new_Vector3()    ;
    v2 =     new_Vector3()    ;
    if (v1) printf("%g\n",     Vector3_magnitude(    v1    )    ); else printf("NULL\n");
    if (v2) printf("%g\n",     Vector3_magnitude(    v2    )    ); else printf("NULL\n");
    printf("%s\n", "=== Testing Circle ===");
    c1 =     new_Circle()    ;
    if (c1) printf("%g\n",     Circle_area(    c1    )    ); else printf("NULL\n");
    if (c1) printf("%g\n",     Circle_area(    c1    )    ); else printf("NULL\n");
    // Limpieza de memoria antes de salir
    if (p1) free(p1);
    if (p2) free(p2);
    if (v1) free(v1);
    if (v2) free(v2);
    if (c1) free(c1);
    return 0;
}
