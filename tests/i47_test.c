// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); exit(1); } } while(0)
static double distance(Point a,Point b) { return hypot(a.x-b.x,a.y-b.y); }
static unsigned fixtures;
static double worst_area;
static void fixture(double blue,double yellow,double rotation) {
    PythagorasConstruction p;
    CHECK(construct_pythagoras((Point){.25,-.5},blue,yellow,rotation,&p)==GEOMETRY_OK);
    CHECK(fabs(distance(p.right_angle,p.blue_end)-blue)<1e-11*fmax(1.0,blue));
    CHECK(fabs(distance(p.right_angle,p.yellow_end)-yellow)<1e-11*fmax(1.0,yellow));
    CHECK(fabs(p.red_length-hypot(blue,yellow))<1e-11*fmax(1.0,p.red_length));
    CHECK(p.right_angle_error<1e-12); CHECK(p.square_error<1e-12); CHECK(p.area_error<1e-12);
    CHECK(fabs(p.red_area-p.blue_area-p.yellow_area)<1e-10*fmax(1.0,p.red_area));
    worst_area=fmax(worst_area,p.area_error); ++fixtures;
}
int main(void) {
    fixture(3,4,0); fixture(.35,1.35,.2); fixture(1.35,.35,-.5);
    for (int i=0;i<720;++i) {
        double blue=.35+(i%101)/100.0;
        double yellow=.35+((i*37)%101)/100.0;
        fixture(blue,yellow,(i-360)*.002);
    }
    PythagorasConstruction p;
    CHECK(construct_pythagoras((Point){0,0},0,1,0,&p)==GEOMETRY_DEGENERATE);
    CHECK(construct_pythagoras((Point){0,0},1,-1,0,&p)==GEOMETRY_DEGENERATE);
    CHECK(construct_pythagoras((Point){NAN,0},1,1,0,&p)==GEOMETRY_INVALID);
    CHECK(construct_pythagoras((Point){0,0},1,1,NAN,&p)==GEOMETRY_INVALID);

    I47Applet a; CHECK(i47_applet_init(&a)); i47_applet_size(&a,576,1152,160);
    Point blue=i47_applet_to_screen(&a,a.construction.blue_end);
    i47_applet_down(&a,7,blue.x,blue.y); CHECK(a.captured==0);
    i47_applet_move(&a,8,blue.x+100,blue.y); CHECK(a.drag_updates==0);
    i47_applet_move(&a,7,blue.x+4,blue.y); CHECK(a.drag_updates==0);
    Point target=i47_applet_to_screen(&a,(Point){1.30,0});
    i47_applet_move(&a,7,target.x,target.y); CHECK(a.drag_updates==1); CHECK(fabs(a.blue_length-1.30)<1e-12);
    CHECK(a.construction.area_error<1e-12);
    i47_applet_cancel(&a); CHECK(a.captured==-1);
    CHECK(!a.debug); i47_applet_down(&a,1,480,70); CHECK(a.debug); CHECK(a.captured==-1);
    i47_applet_down(&a,1,480,70); CHECK(!a.debug);
    CHECK(i47_applet_restore(&a,.8,1.2,true)); CHECK(a.debug); CHECK(fabs(a.blue_length-.8)<1e-12);
    CHECK(!i47_applet_restore(&a,.1,1.2,false));
    printf("PASS %u Pythagoras fixtures; worst normalized area error %.17g\n",fixtures,worst_area);
    puts("PASS squares, right angle, area identity, pointer capture, constrained leg drags");
    return 0;
}
