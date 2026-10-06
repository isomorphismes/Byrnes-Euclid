// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_PROPOSITION1_H
#define BYRNE_PROPOSITION1_H
#include "geometry.h"
typedef struct {
    Point chord[2], chord_midpoint;
    Line perpendicular;
    Point intersections[2], constructed_centre;
    double midpoint_error, perpendicular_error, boundary_error, equal_radius_error;
    GeometryStatus status;
} Construction;
GeometryStatus construct_centre(const Circle *circle, Point first, Point second, Construction *result);
#endif
