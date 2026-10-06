// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include "touch_contract.h"
#include <math.h>
#include <string.h>

#define I47_MIN_LEG 0.35
#define I47_MAX_LEG 1.35

static double model_scale(const I47Applet *applet) {
    double horizontal=applet->width*.90/4.35;
    double vertical=applet->height*.48/4.35;
    return fmin(horizontal,vertical);
}
static Point model_centre(void) { return (Point){.675,.675}; }
static Point screen_centre(const I47Applet *applet) { return (Point){applet->width*.5,applet->height*.415}; }
Point i47_applet_to_screen(const I47Applet *applet,Point point) {
    Point mc=model_centre(),sc=screen_centre(applet); double s=model_scale(applet);
    return (Point){sc.x+(point.x-mc.x)*s,sc.y-(point.y-mc.y)*s};
}
static Point from_screen(const I47Applet *applet,double x,double y) {
    Point mc=model_centre(),sc=screen_centre(applet); double s=model_scale(applet);
    return (Point){mc.x+(x-sc.x)/s,mc.y-(y-sc.y)/s};
}
static bool recompute(I47Applet *applet) {
    if (construct_pythagoras((Point){0,0},applet->blue_length,applet->yellow_length,
                             applet->rotation,&applet->construction)!=GEOMETRY_OK) return false;
    applet->maximum_area_error=fmax(applet->maximum_area_error,applet->construction.area_error);
    return true;
}
static double clamp_leg(double length) { return fmax(I47_MIN_LEG,fmin(I47_MAX_LEG,length)); }

bool i47_applet_init(I47Applet *applet) {
    if (!applet) return false;
    memset(applet,0,sizeof *applet);
    applet->blue_length=1.10; applet->yellow_length=.74; applet->rotation=0;
    applet->captured=-1; applet->pointer_id=-1;
    return recompute(applet);
}
bool i47_applet_restore(I47Applet *applet,double blue_length,double yellow_length,bool debug) {
    if (!applet||!isfinite(blue_length)||!isfinite(yellow_length)||
        blue_length<I47_MIN_LEG||blue_length>I47_MAX_LEG||
        yellow_length<I47_MIN_LEG||yellow_length>I47_MAX_LEG) return false;
    applet->blue_length=blue_length; applet->yellow_length=yellow_length; applet->debug=debug;
    return recompute(applet);
}
void i47_applet_size(I47Applet *applet,int width,int height,int density) {
    applet->width=width; applet->height=height; applet->density=density; i47_applet_cancel(applet);
}
void i47_applet_cancel(I47Applet *applet) { applet->captured=-1; applet->pointer_id=-1; applet->moved=false; }
I47AppletRect i47_applet_checks_bounds(const I47Applet *applet) {
    return (I47AppletRect){applet->width*.72,applet->height*.026,applet->width*.97,applet->height*.105};
}
void i47_applet_down(I47Applet *applet,int pointer_id,double x,double y) {
    i47_applet_cancel(applet);
    if (applet->blocked||applet->width<=0||applet->height<=0) return;
    I47AppletRect checks=i47_applet_checks_bounds(applet);
    if (x>=checks.left&&x<=checks.right&&y>=checks.top&&y<=checks.bottom) { applet->debug=!applet->debug; return; }
    Point handles[2]={i47_applet_to_screen(applet,applet->construction.blue_end),
                      i47_applet_to_screen(applet,applet->construction.yellow_end)};
    double best=fmax(touch_target_radius_pixels(applet->density),applet->width*.055);
    for (int index=0;index<2;++index) {
        double distance=hypot(x-handles[index].x,y-handles[index].y);
        if (distance<=best) { best=distance; applet->captured=index; }
    }
    if (applet->captured<0) return;
    applet->pointer_id=pointer_id; applet->down_x=x; applet->down_y=y;
    applet->original_length=applet->captured==0?applet->blue_length:applet->yellow_length;
}
void i47_applet_move(I47Applet *applet,int pointer_id,double x,double y) {
    if (applet->blocked||applet->captured<0||pointer_id!=applet->pointer_id) return;
    if (!applet->moved&&!touch_drag_threshold_exceeded((float)(x-applet->down_x),(float)(y-applet->down_y))) return;
    Point cursor=from_screen(applet,x,y);
    Point unit=applet->captured==0?(Point){cos(applet->rotation),sin(applet->rotation)}:
                                      (Point){-sin(applet->rotation),cos(applet->rotation)};
    double projected=cursor.x*unit.x+cursor.y*unit.y;
    double previous=applet->captured==0?applet->blue_length:applet->yellow_length;
    double updated=clamp_leg(projected);
    if (fabs(updated-previous)<1e-12) { applet->moved=true; return; }
    if (applet->captured==0) applet->blue_length=updated; else applet->yellow_length=updated;
    if (!recompute(applet)) {
        if (applet->captured==0) applet->blue_length=previous; else applet->yellow_length=previous;
        recompute(applet); return;
    }
    applet->moved=true; ++applet->drag_updates;
}
