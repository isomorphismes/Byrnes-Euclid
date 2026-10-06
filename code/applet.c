// SPDX-License-Identifier: GPL-3.0-or-later
#include "applet.h"
#include "touch_contract.h"
#include <math.h>
#include <string.h>
static double scale(const Applet *applet) { return fmin(applet->width*.39, applet->height*.235); }
static Point from_screen(const Applet *applet, double x, double y) {
    return (Point){(x-applet->width*.5)/scale(applet), (y-applet->height*.405)/scale(applet)};
}
Point applet_to_screen(const Applet *applet, Point point) {
    return (Point){applet->width*.5+point.x*scale(applet), applet->height*.405+point.y*scale(applet)};
}
static void recompute(Applet *applet) {
    construct_centre(applet->circle, circle_boundary(applet->circle, applet->angles[0]),
                     circle_boundary(applet->circle, applet->angles[1]), &applet->construction);
    if (applet->construction.status == GEOMETRY_OK)
        applet->maximum_drift = fmax(applet->maximum_drift, point_distance(applet->reference_centre, applet->construction.constructed_centre));
}
bool applet_init(Applet *applet) {
    memset(applet, 0, sizeof *applet);
    applet->circle = circle_create((Point){0,0}, 1);
    if (!applet->circle) return false;
    applet->angles[0] = 3.5; applet->angles[1] = .85;
    applet->captured = -1; applet->pointer_id = -1;
    recompute(applet);
    applet->reference_centre = applet->construction.constructed_centre;
    applet->maximum_drift = 0;
    return applet->construction.status == GEOMETRY_OK;
}
void applet_size(Applet *applet, int width, int height, int density) {
    applet->width=width; applet->height=height; applet->density=density;
    applet_cancel(applet);
}
void applet_cancel(Applet *applet) {
    applet->captured=-1; applet->pointer_id=-1; applet->moved=false;
}
AppletRect applet_checks_bounds(const Applet *applet) {
    return (AppletRect){applet->width*.72,applet->height*.026,
                         applet->width*.97,applet->height*.11};
}
void applet_down(Applet *applet, int pointer_id, double x, double y) {
    applet_cancel(applet);
    if (applet->blocked || applet->width<=0 || applet->height<=0) return;
    AppletRect checks=applet_checks_bounds(applet);
    if (x>=checks.left && x<=checks.right && y>=checks.top && y<=checks.bottom) {
        applet->debug=!applet->debug; return;
    }
    double best=fmax(touch_target_radius_pixels(applet->density), applet->width*.055);
    for (int index=0; index<2; ++index) {
        Point endpoint=applet_to_screen(applet, circle_boundary(applet->circle, applet->angles[index]));
        double distance=hypot(x-endpoint.x,y-endpoint.y);
        if (distance<=best) { best=distance; applet->captured=index; }
    }
    if (applet->captured<0) return;
    applet->pointer_id=pointer_id;
    applet->down_x=x; applet->down_y=y;
    applet->original_angle=applet->angles[applet->captured];
    if (!circle_cursor_angle(applet->circle, from_screen(applet,x,y), &applet->down_angle)) applet_cancel(applet);
}
void applet_move(Applet *applet, int pointer_id, double x, double y) {
    if (applet->blocked || applet->captured<0 || pointer_id!=applet->pointer_id) return;
    if (!applet->moved && !touch_drag_threshold_exceeded((float)(x-applet->down_x),(float)(y-applet->down_y))) return;
    double cursor_angle;
    if (!circle_cursor_angle(applet->circle, from_screen(applet,x,y), &cursor_angle)) return;
    applet->moved=true;
    double angle=applet->original_angle+remainder(cursor_angle-applet->down_angle,6.283185307179586);
    double other=applet->angles[1-applet->captured], separation=remainder(angle-other,6.283185307179586);
    /* Maintain a proper, finger-visible chord; crossing endpoints remains possible. */
    if (fabs(separation)<.08) angle=other+copysign(.08,separation==0 ? 1 : separation);
    applet->angles[applet->captured]=angle;
    recompute(applet); ++applet->drag_updates;
}
