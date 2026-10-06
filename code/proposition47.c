// SPDX-License-Identifier: GPL-3.0-or-later
#include "proposition47.h"
#include <math.h>
#include <string.h>

static Point add(Point a, Point b) { return (Point){a.x+b.x,a.y+b.y}; }
static Point sub(Point a, Point b) { return (Point){a.x-b.x,a.y-b.y}; }
static Point mul(Point a, double s) { return (Point){a.x*s,a.y*s}; }
static double dot(Point a, Point b) { return a.x*b.x+a.y*b.y; }
static double cross(Point a, Point b) { return a.x*b.y-a.y*b.x; }
static double length(Point a) { return hypot(a.x,a.y); }
static Point rotate_cw(Point a) { return (Point){a.y,-a.x}; }
static Point rotate_ccw(Point a) { return (Point){-a.y,a.x}; }
static double maximum(double a,double b) { return a>b?a:b; }
static bool finite_point(Point p) { return isfinite(p.x)&&isfinite(p.y); }

static ByrneSquare square_on_directed_side(Point first,Point second,bool outward_left) {
    Point edge=sub(second,first);
    Point offset=outward_left?rotate_ccw(edge):rotate_cw(edge);
    return (ByrneSquare){{first,second,add(second,offset),add(first,offset)}};
}

double byrne_square_area(const ByrneSquare *square) {
    if (!square) return NAN;
    double twice=0;
    for (int i=0;i<4;++i) twice += cross(square->vertex[i],square->vertex[(i+1)%4]);
    return fabs(twice)*.5;
}

static double square_quality(const ByrneSquare *square,double expected) {
    double worst=0;
    for (int i=0;i<4;++i) {
        Point edge=sub(square->vertex[(i+1)%4],square->vertex[i]);
        worst=maximum(worst,fabs(length(edge)-expected)/expected);
        Point next=sub(square->vertex[(i+2)%4],square->vertex[(i+1)%4]);
        worst=maximum(worst,fabs(dot(edge,next))/(expected*expected));
    }
    return worst;
}

GeometryStatus construct_pythagoras(Point right_angle,double blue_length,double yellow_length,
                                    double rotation,PythagorasConstruction *out) {
    if (!out) return GEOMETRY_INVALID;
    memset(out,0,sizeof *out);
    out->status=GEOMETRY_INVALID;
    if (!finite_point(right_angle)||!isfinite(blue_length)||!isfinite(yellow_length)||!isfinite(rotation)) return out->status;
    if (blue_length<=0||yellow_length<=0) { out->status=GEOMETRY_DEGENERATE; return out->status; }
    if (blue_length>1e6||yellow_length>1e6) return out->status;

    Point blue_unit={cos(rotation),sin(rotation)};
    Point yellow_unit={-sin(rotation),cos(rotation)};
    Point blue=mul(blue_unit,blue_length),yellow=mul(yellow_unit,yellow_length);
    Point blue_end=add(right_angle,blue),yellow_end=add(right_angle,yellow);
    Point hypotenuse=sub(yellow_end,blue_end);
    double red_length=length(hypotenuse);
    if (!isfinite(red_length)||red_length<=0) { out->status=GEOMETRY_DEGENERATE; return out->status; }

    /* The triangle lies to the left of right->blue and to the right of right->yellow. */
    ByrneSquare blue_square=square_on_directed_side(right_angle,blue_end,false);
    ByrneSquare yellow_square=square_on_directed_side(right_angle,yellow_end,true);
    /* The right-angle vertex lies to the left of blue->yellow, so its square goes right. */
    ByrneSquare red_square=square_on_directed_side(blue_end,yellow_end,false);

    double projection=dot(sub(right_angle,blue_end),hypotenuse)/dot(hypotenuse,hypotenuse);
    Point foot=add(blue_end,mul(hypotenuse,projection));
    Point red_offset=sub(red_square.vertex[3],red_square.vertex[0]);
    Point far_cut=add(foot,red_offset);

    double blue_area=byrne_square_area(&blue_square);
    double yellow_area=byrne_square_area(&yellow_square);
    double red_area=byrne_square_area(&red_square);
    double area_scale=maximum(1.0,red_area);
    double area_error=fabs(red_area-blue_area-yellow_area)/area_scale;
    double square_error=maximum(square_quality(&blue_square,blue_length),square_quality(&yellow_square,yellow_length));
    square_error=maximum(square_error,square_quality(&red_square,red_length));
    double right_angle_error=fabs(dot(blue,yellow))/(blue_length*yellow_length);

    *out=(PythagorasConstruction){
        .right_angle=right_angle,.blue_end=blue_end,.yellow_end=yellow_end,
        .blue_square=blue_square,.yellow_square=yellow_square,.red_square=red_square,
        .hypotenuse_foot=foot,.hypotenuse_far_cut=far_cut,
        .blue_length=blue_length,.yellow_length=yellow_length,.red_length=red_length,
        .blue_area=blue_area,.yellow_area=yellow_area,.red_area=red_area,
        .right_angle_error=right_angle_error,.square_error=square_error,.area_error=area_error,
        .status=GEOMETRY_OK
    };
    return out->status;
}
