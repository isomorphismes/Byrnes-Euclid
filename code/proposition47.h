// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_PROPOSITION47_H
#define BYRNE_PROPOSITION47_H
#include "geometry.h"

typedef struct { Point vertex[4]; } ByrneSquare;
typedef struct {
    Point right_angle;
    Point blue_end;
    Point yellow_end;
    ByrneSquare blue_square;
    ByrneSquare yellow_square;
    ByrneSquare red_square;
    Point hypotenuse_foot;
    Point hypotenuse_far_cut;
    double blue_length;
    double yellow_length;
    double red_length;
    double blue_area;
    double yellow_area;
    double red_area;
    double right_angle_error;
    double square_error;
    double area_error;
    GeometryStatus status;
} PythagorasConstruction;

GeometryStatus construct_pythagoras(Point right_angle,
                                    double blue_length,
                                    double yellow_length,
                                    double rotation,
                                    PythagorasConstruction *out);
double byrne_square_area(const ByrneSquare *square);

#endif
