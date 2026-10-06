// SPDX-License-Identifier: GPL-3.0-or-later
#include "applet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); exit(1); } } while(0)
static void close_point(Point actual,Point expected,double tolerance) { CHECK(point_distance(actual,expected)<=tolerance); }
static unsigned fixtures;
static double worst_drift;
static void fixture(Point known_centre,double radius,double first_angle,double second_angle) {
    Circle *circle=circle_create(known_centre,radius); CHECK(circle);
    Point first=circle_boundary(circle,first_angle),second=circle_boundary(circle,second_angle);
    Construction result;
    GeometryStatus status=construct_centre(circle,first,second,&result);
    if (status!=GEOMETRY_OK) fprintf(stderr,"fixture centre=(%.17g,%.17g) radius=%.17g angles=(%.17g,%.17g) status=%s equal=%.17g\n",known_centre.x,known_centre.y,radius,first_angle,second_angle,geometry_status_name(status),result.equal_radius_error);
    CHECK(status==GEOMETRY_OK);
    close_point(result.chord_midpoint,point_midpoint(first,second),radius*1e-12);
    CHECK(result.perpendicular_error<1e-12);
    CHECK(circle_point_residual(circle,result.intersections[0])<1e-10);
    CHECK(circle_point_residual(circle,result.intersections[1])<1e-10);
    close_point(result.constructed_centre,known_centre,radius*BYRNE_RESIDUAL_TOLERANCE);
    double drift=point_distance(result.constructed_centre,known_centre)/radius;
    worst_drift=fmax(worst_drift,drift);
    for (int index=0;index<31;++index)
        CHECK(fabs(point_distance(result.constructed_centre,circle_boundary(circle,index*.2026833970057931))/radius-1)<BYRNE_RESIDUAL_TOLERANCE);
    ++fixtures; circle_destroy(circle);
}
int main(void) {
    close_point(point_midpoint((Point){-4,2},(Point){8,6}),(Point){2,4},0);
    close_point(point_midpoint((Point){1e300,1e300},(Point){1e300,1e300}),(Point){1e300,1e300},0);
    fixture((Point){0,0},1,0,1.5707963267948966);
    fixture((Point){7,-3},5,0,3.141592653589793);
    fixture((Point){-2.75,4.25},2.5,3.5,.85);
    fixture((Point){1e5,-2e5},100,1.1,1.11);
    fixture((Point){0,0},1,1.1,1.10002);
    fixture((Point){0,0},1e-6,1.5,3.5);
    /* Deterministic generated chords exercise angles, translations and scales. */
    for (int index=0;index<720;++index) {
        double angle=index*.008726646259971648, gap=.08+(index%97)*.0601;
        fixture((Point){37,-21},10,angle,angle+gap);
        fixture((Point){0,0},1,angle,angle+gap);
    }
    Circle *circle=circle_create((Point){2,3},5); CHECK(circle);
    Point cuts[2];
    CHECK(circle_line_intersections(circle,(Line){{2,3},{0,3}},cuts)==GEOMETRY_OK);
    close_point(cuts[0],(Point){2,-2},1e-12); close_point(cuts[1],(Point){2,8},1e-12);
    CHECK(circle_line_intersections(circle,(Line){{-4,6},{2,0}},cuts)==GEOMETRY_OK);
    close_point(cuts[0],(Point){-2,6},1e-12); close_point(cuts[1],(Point){6,6},1e-12);
    CHECK(circle_line_intersections(circle,(Line){{2,8},{1,0}},cuts)==GEOMETRY_TANGENT);
    close_point(cuts[0],(Point){2,8},1e-12); close_point(cuts[1],cuts[0],0);
    CHECK(circle_line_intersections(circle,(Line){{2,8.001},{1,0}},cuts)==GEOMETRY_MISS);
    CHECK(isnan(cuts[0].x));
    CHECK(circle_line_intersections(circle,(Line){{2,3},{0,0}},cuts)==GEOMETRY_DEGENERATE);
    CHECK(circle_line_intersections(circle,(Line){{NAN,3},{1,0}},cuts)==GEOMETRY_INVALID);
    CHECK(isnan(cuts[0].x));
    Construction result;
    Point first=circle_boundary(circle,0);
    CHECK(construct_centre(circle,first,first,&result)==GEOMETRY_DEGENERATE);
    CHECK(isnan(result.constructed_centre.x));
    CHECK(construct_centre(circle,first,circle_boundary(circle,1e-7),&result)==GEOMETRY_DEGENERATE);
    CHECK(construct_centre(circle,first,(Point){2,3},&result)==GEOMETRY_INVALID);
    CHECK(construct_centre(circle,(Point){NAN,0},first,&result)==GEOMETRY_INVALID);
    CHECK(!circle_create((Point){0,0},0)); CHECK(!circle_create((Point){0,0},INFINITY));
    CHECK(!circle_create((Point){NAN,0},1));
    double error; CHECK(!circle_verify_candidate(circle,(Point){2.1,3},&error));
    CHECK(error>BYRNE_RESIDUAL_TOLERANCE); circle_destroy(circle);
    /* Translation plus an extremely short chord can exceed the double-input
       precision budget. Reject this explicitly rather than certify a drifting centre. */
    circle=circle_create((Point){1e5,-2e5},100); CHECK(circle);
    CHECK(construct_centre(circle,circle_boundary(circle,1.1),circle_boundary(circle,1.10002),&result)==GEOMETRY_INVALID);
    CHECK(isnan(result.constructed_centre.x)); circle_destroy(circle);
    Applet applet; CHECK(applet_init(&applet)); applet_size(&applet,576,1152,160);
    Point endpoint=applet_to_screen(&applet,applet.construction.chord[0]);
    double old_angle=applet.angles[0];
    applet_down(&applet,7,endpoint.x+6,endpoint.y+4);
    CHECK(applet.captured==0); CHECK(applet.angles[0]==old_angle);
    applet_move(&applet,7,endpoint.x+7,endpoint.y+5); CHECK(applet.angles[0]==old_angle);
    applet_move(&applet,8,500,400); CHECK(applet.angles[0]==old_angle);
    for (int index=0;index<720;++index) {
        Point cursor=applet_to_screen(&applet,circle_boundary(applet.circle,index*.008726646259971648));
        applet_move(&applet,7,cursor.x,cursor.y);
        CHECK(applet.construction.status==GEOMETRY_OK);
        CHECK(applet.maximum_drift<BYRNE_RESIDUAL_TOLERANCE);
    }
    applet_cancel(&applet); CHECK(applet.captured==-1);
    applet_down(&applet,1,20,1120); CHECK(applet.debug);
    circle_destroy(applet.circle);
    printf("PASS %u fixed/generated chords; 720 drag positions; tolerance=%.1g radius; worst drift=%.17g radius\n",fixtures,BYRNE_RESIDUAL_TOLERANCE,worst_drift);
    puts("PASS midpoint, perpendicular, secant/tangent/miss, boundary, equal radii, degeneracy, pointer capture");
    return 0;
}
