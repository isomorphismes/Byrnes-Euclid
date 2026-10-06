// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_GEOMETRY_H
#define BYRNE_GEOMETRY_H
#include <stdbool.h>
typedef struct { double x, y; } Point;
typedef struct { Point through, direction; } Line;
typedef struct Circle Circle; /* Centre is private to the kernel. */
typedef enum { GEOMETRY_OK, GEOMETRY_INVALID, GEOMETRY_DEGENERATE,
               GEOMETRY_MISS, GEOMETRY_TANGENT } GeometryStatus;
#define BYRNE_MIN_CHORD_RATIO 1e-5
#define BYRNE_RESIDUAL_TOLERANCE 1e-9
Circle *circle_create(Point centre, double radius);
void circle_destroy(Circle *circle);
Point circle_boundary(const Circle *circle, double angle);
bool circle_cursor_angle(const Circle *circle, Point cursor, double *angle);
double circle_radius(const Circle *circle);
double circle_point_residual(const Circle *circle, Point point);
Point point_midpoint(Point first, Point second);
double point_distance(Point first, Point second);
GeometryStatus perpendicular_through(Point first, Point second, Point through, Line *line);
GeometryStatus circle_line_intersections(const Circle *circle, Line line, Point cuts[2]);
bool circle_verify_candidate(const Circle *circle, Point candidate, double *equal_radius_error);
const char *geometry_status_name(GeometryStatus status);
#endif
