// SPDX-License-Identifier: GPL-3.0-or-later
#include "proposition1.h"
#include <math.h>
#include <string.h>
GeometryStatus construct_centre(const Circle *circle, Point first, Point second, Construction *result) {
    if (!result) return GEOMETRY_INVALID;
    memset(result, 0, sizeof *result);
    result->status = GEOMETRY_INVALID;
    result->constructed_centre = (Point){NAN, NAN};
    if (!circle || !isfinite(first.x) || !isfinite(first.y) || !isfinite(second.x) || !isfinite(second.y)) return result->status;
    result->chord[0] = first; result->chord[1] = second;
    double radius = circle_radius(circle), length = point_distance(first, second);
    if (circle_point_residual(circle, first) > BYRNE_RESIDUAL_TOLERANCE ||
        circle_point_residual(circle, second) > BYRNE_RESIDUAL_TOLERANCE) return result->status;
    if (length/radius < BYRNE_MIN_CHORD_RATIO) return result->status = GEOMETRY_DEGENERATE;
    result->chord_midpoint = point_midpoint(first, second);
    result->status = perpendicular_through(first, second, result->chord_midpoint, &result->perpendicular);
    if (result->status != GEOMETRY_OK) return result->status;
    result->status = circle_line_intersections(circle, result->perpendicular, result->intersections);
    if (result->status != GEOMETRY_OK) return result->status;
    result->constructed_centre = point_midpoint(result->intersections[0], result->intersections[1]);
    result->midpoint_error = fabs(point_distance(first, result->chord_midpoint)-point_distance(second, result->chord_midpoint))/radius;
    result->perpendicular_error = fabs(((second.x-first.x)*result->perpendicular.direction.x +
                                      (second.y-first.y)*result->perpendicular.direction.y)/length);
    result->boundary_error = fmax(circle_point_residual(circle, result->intersections[0]), circle_point_residual(circle, result->intersections[1]));
    if (!circle_verify_candidate(circle, result->constructed_centre, &result->equal_radius_error) ||
        result->midpoint_error > BYRNE_RESIDUAL_TOLERANCE || result->perpendicular_error > BYRNE_RESIDUAL_TOLERANCE ||
        result->boundary_error > BYRNE_RESIDUAL_TOLERANCE) {
        result->status = GEOMETRY_INVALID;
        result->constructed_centre = (Point){NAN,NAN};
    }
    return result->status;
}
