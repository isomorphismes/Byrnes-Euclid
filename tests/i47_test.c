// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); exit(1); } } while(0)

static double distance(Point a,Point b) { return hypot(a.x-b.x,a.y-b.y); }
static unsigned fixtures;
static double worst_area;

static char *read_file(const char *path,size_t *length) {
    FILE *file=fopen(path,"rb"); if (!file) return NULL;
    if (fseek(file,0,SEEK_END)!=0) { fclose(file); return NULL; }
    long size=ftell(file); if (size<=0||fseek(file,0,SEEK_SET)!=0) { fclose(file); return NULL; }
    char *content=malloc((size_t)size); if (!content) { fclose(file); return NULL; }
    if (fread(content,1,(size_t)size,file)!=(size_t)size) { free(content); fclose(file); return NULL; }
    fclose(file); *length=(size_t)size; return content;
}

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

int main(int argc,char **argv) {
    CHECK(argc==2);
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

    size_t lua_length=0; char *lua_source=read_file(argv[1],&lua_length); CHECK(lua_source);
    I47Applet a; CHECK(i47_applet_init(&a,lua_source,lua_length)); free(lua_source);
    i47_applet_size(&a,576,1152,160);
    CHECK(a.runtime_ok);
    CHECK(strcmp(a.presentation.title,"BOOK I.47")==0);
    CHECK(strcmp(a.presentation.instruction,"DRAG THE BLUE OR YELLOW ENDPOINT")==0);
    CHECK(a.presentation.hypotenuse_square_colour==I47_COLOUR_RED);
    CHECK(a.presentation.blue_square_colour==I47_COLOUR_BLUE);
    CHECK(a.presentation.yellow_square_colour==I47_COLOUR_BLACK);

    Point blue=i47_applet_to_screen(&a,a.construction.blue_end);
    i47_applet_down(&a,7,blue.x,blue.y); CHECK(a.captured==0);
    i47_applet_move(&a,8,blue.x+100,blue.y); CHECK(a.drag_updates==0);
    i47_applet_move(&a,7,blue.x+4,blue.y); CHECK(a.drag_updates==0);
    Point target=i47_applet_to_screen(&a,(Point){1.30,0});
    i47_applet_move(&a,7,target.x,target.y);
    CHECK(a.drag_updates==1); CHECK(fabs(a.blue_length-1.30)<1e-12);
    CHECK(a.construction.area_error<1e-12);
    i47_applet_cancel(&a); CHECK(a.captured==-1);

    CHECK(!a.debug); i47_applet_down(&a,1,480,70); CHECK(a.debug); CHECK(a.captured==-1);
    i47_applet_down(&a,1,480,70); CHECK(!a.debug);
    CHECK(i47_applet_restore(&a,.8,1.2,true)); CHECK(a.debug); CHECK(fabs(a.blue_length-.8)<1e-12);
    CHECK(!i47_applet_restore(&a,.1,1.2,false));

    /* Lua owns the clamping rule: drag far past the endpoint and it caps at 1.35. */
    Point blue_again=i47_applet_to_screen(&a,a.construction.blue_end);
    i47_applet_down(&a,9,blue_again.x,blue_again.y); CHECK(a.captured==0);
    Point beyond=i47_applet_to_screen(&a,(Point){3.0,0});
    i47_applet_move(&a,9,beyond.x,beyond.y);
    CHECK(fabs(a.blue_length-1.35)<1e-12);
    CHECK(a.runtime_ok);

    i47_applet_close(&a);
    printf("PASS %u Pythagoras fixtures; worst normalized area error %.17g\n",fixtures,worst_area);
    puts("PASS Lua state/presentation, C geometry, pointer capture, constrained leg drags");
    return 0;
}
