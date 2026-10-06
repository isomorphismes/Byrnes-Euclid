// SPDX-License-Identifier: GPL-3.0-or-later
#include "geometry.h"
#include <math.h>
#include <stdlib.h>
struct Circle { Point centre; double radius; };
static bool finite_point(Point point) { return isfinite(point.x) && isfinite(point.y); }
Circle *circle_create(Point centre, double radius) {
    /* This slice deliberately bounds coordinates; it is not an arbitrary-scale CAD kernel. */
    if (!finite_point(centre) || !isfinite(radius) || radius < 1e-6 || radius > 1e9 ||
        fabs(centre.x) > radius * 1e6 || fabs(centre.y) > radius * 1e6) return NULL;
    Circle *circle = malloc(sizeof *circle);
    if (circle) *circle = (Circle){centre, radius};
    return circle;
}
void circle_destroy(Circle *circle) { free(circle); }
Point circle_boundary(const Circle *circle, double angle) {
    return (Point){circle->centre.x + circle->radius * cos(angle),
                   circle->centre.y + circle->radius * sin(angle)};
}
bool circle_cursor_angle(const Circle *circle, Point cursor, double *angle) {
    if (!circle || !angle || !finite_point(cursor)) return false;
    double dx = cursor.x - circle->centre.x, dy = cursor.y - circle->centre.y;
    if (hypot(dx, dy) < circle->radius * 1e-6) return false;
    *angle = atan2(dy, dx);
    return true;
}
double circle_radius(const Circle *circle) { return circle->radius; }
double point_distance(Point first, Point second) { return hypot(first.x-second.x, first.y-second.y); }
double circle_point_residual(const Circle *circle, Point point) {
    return fabs(point_distance(circle->centre, point) / circle->radius - 1);
}
Point point_midpoint(Point first, Point second) {
    return (Point){first.x * .5 + second.x * .5, first.y * .5 + second.y * .5};
}
GeometryStatus perpendicular_through(Point first, Point second, Point through, Line *line) {
    if (!line || !finite_point(first) || !finite_point(second) || !finite_point(through)) return GEOMETRY_INVALID;
    double dx = second.x-first.x, dy = second.y-first.y, length = hypot(dx, dy);
    if (!isfinite(length) || length == 0) return GEOMETRY_DEGENERATE;
    *line = (Line){through, {-dy/length, dx/length}};
    return GEOMETRY_OK;
}
GeometryStatus circle_line_intersections(const Circle *circle, Line line, Point cuts[2]) {
    if (!cuts) return GEOMETRY_INVALID;
    cuts[0] = cuts[1] = (Point){NAN, NAN};
    if (!circle || !finite_point(line.through) || !finite_point(line.direction)) return GEOMETRY_INVALID;
    double length = hypot(line.direction.x, line.direction.y);
    if (!isfinite(length) || length == 0) return GEOMETRY_DEGENERATE;
    Point direction = {line.direction.x/length, line.direction.y/length};
    /* Solve in radius-scaled coordinates about the circle. Projection avoids
       cancellation between quadratic roots; the proposition cannot access this centre. */
    Point offset = {(line.through.x-circle->centre.x)/circle->radius,
                    (line.through.y-circle->centre.y)/circle->radius};
    double along = offset.x*direction.x + offset.y*direction.y;
    double across = offset.x*direction.y - offset.y*direction.x;
    double discriminant = 1 - across*across;
    if (!isfinite(along) || !isfinite(discriminant)) return GEOMETRY_INVALID;
    if (discriminant < -1e-12) return GEOMETRY_MISS;
    double half_span = sqrt(fmax(0, discriminant));
    Point foot = {offset.x-along*direction.x, offset.y-along*direction.y};
    for (int index=0; index<2; ++index) {
        double distance = index ? half_span : -half_span;
        cuts[index] = (Point){circle->centre.x + circle->radius*(foot.x+distance*direction.x),
                              circle->centre.y + circle->radius*(foot.y+distance*direction.y)};
    }
    return discriminant <= 1e-12 ? GEOMETRY_TANGENT : GEOMETRY_OK;
}
bool circle_verify_candidate(const Circle *circle, Point candidate, double *error) {
    if (!circle || !error || !finite_point(candidate)) return false;
    *error = 0;
    /* Independent boundary samples, not equality with a stored centre. */
    for (int index=0; index<16; ++index) {
        double residual = fabs(point_distance(candidate, circle_boundary(circle, index*0.3926990816987241548))/circle->radius-1);
        *error = fmax(*error, residual);
    }
    return *error <= BYRNE_RESIDUAL_TOLERANCE;
}
const char *geometry_status_name(GeometryStatus status) {
    switch (status) {
    case GEOMETRY_OK: return "VALID";
    case GEOMETRY_INVALID: return "INVALID INPUT";
    case GEOMETRY_DEGENERATE: return "CHORD TOO SMALL";
    case GEOMETRY_MISS: return "NO INTERSECTIONS";
    case GEOMETRY_TANGENT: return "TANGENT";
    }
    return "UNKNOWN";
}
