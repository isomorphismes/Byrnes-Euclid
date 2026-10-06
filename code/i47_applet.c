// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include "touch_contract.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static void remember_runtime_error(I47Applet *applet,const char *context) {
    const char *detail=applet->lua?i47_lua_last_error(applet->lua):"";
    snprintf(applet->error,sizeof applet->error,"%s%s%s",context,
             detail&&detail[0]?": ":"",detail&&detail[0]?detail:"");
    applet->runtime_ok=false;
}

static bool sync_from_lua(I47Applet *applet) {
    if (!applet->lua) return false;
    if (!i47_lua_sync(applet->lua,&applet->blue_length,&applet->yellow_length,
                      &applet->rotation,&applet->debug,&applet->maximum_area_error,
                      &applet->construction)) {
        remember_runtime_error(applet,"Lua state sync failed");
        return false;
    }
    applet->runtime_ok=true;
    return true;
}

static bool refresh_presentation(I47Applet *applet) {
    if (applet->width<=0||applet->height<=0) return true;
    if (!i47_lua_presentation(applet->lua,applet->width,applet->height,&applet->presentation)) {
        remember_runtime_error(applet,"Lua presentation failed");
        return false;
    }
    return true;
}

Point i47_applet_to_screen(const I47Applet *applet,Point point) {
    const I47Presentation *p=&applet->presentation;
    return (Point){
        p->screen_centre_x+(point.x-p->model_centre_x)*p->scale,
        p->screen_centre_y-(point.y-p->model_centre_y)*p->scale
    };
}

static Point from_screen(const I47Applet *applet,double x,double y) {
    const I47Presentation *p=&applet->presentation;
    return (Point){
        p->model_centre_x+(x-p->screen_centre_x)/p->scale,
        p->model_centre_y-(y-p->screen_centre_y)/p->scale
    };
}

bool i47_applet_init(I47Applet *applet,const char *lua_source,size_t lua_length) {
    if (!applet) return false;
    memset(applet,0,sizeof *applet);
    applet->captured=-1; applet->pointer_id=-1;
    if (!i47_lua_create(&applet->lua,lua_source,lua_length,applet->error,sizeof applet->error)) return false;
    if (!sync_from_lua(applet)) {
        i47_applet_close(applet);
        return false;
    }
    return true;
}

void i47_applet_close(I47Applet *applet) {
    if (!applet) return;
    i47_lua_destroy(applet->lua);
    applet->lua=NULL;
    applet->runtime_ok=false;
}

bool i47_applet_restore(I47Applet *applet,double blue_length,double yellow_length,bool debug) {
    if (!applet||!applet->lua) return false;
    bool accepted=false;
    if (!i47_lua_restore(applet->lua,blue_length,yellow_length,debug,&accepted)) {
        remember_runtime_error(applet,"Lua restore failed");
        return false;
    }
    if (!accepted) return false;
    if (!sync_from_lua(applet)) return false;
    return refresh_presentation(applet);
}

void i47_applet_size(I47Applet *applet,int width,int height,int density) {
    if (!applet) return;
    applet->width=width; applet->height=height; applet->density=density;
    i47_applet_cancel(applet);
    if (width>0&&height>0&&applet->lua) (void)refresh_presentation(applet);
}

void i47_applet_cancel(I47Applet *applet) {
    if (!applet) return;
    applet->captured=-1; applet->pointer_id=-1; applet->moved=false;
}

I47AppletRect i47_applet_checks_bounds(const I47Applet *applet) {
    const I47Presentation *p=&applet->presentation;
    return (I47AppletRect){p->checks_left,p->checks_top,p->checks_right,p->checks_bottom};
}

void i47_applet_down(I47Applet *applet,int pointer_id,double x,double y) {
    i47_applet_cancel(applet);
    if (!applet->runtime_ok||applet->blocked||applet->width<=0||applet->height<=0) return;
    I47AppletRect checks=i47_applet_checks_bounds(applet);
    if (x>=checks.left&&x<=checks.right&&y>=checks.top&&y<=checks.bottom) {
        bool debug=false;
        if (!i47_lua_toggle_checks(applet->lua,&debug)||!sync_from_lua(applet)) {
            remember_runtime_error(applet,"Lua checks toggle failed");
            return;
        }
        (void)debug;
        return;
    }
    Point handles[2]={i47_applet_to_screen(applet,applet->construction.blue_end),
                      i47_applet_to_screen(applet,applet->construction.yellow_end)};
    double best=fmax(touch_target_radius_pixels(applet->density),applet->width*.055);
    for (int index=0;index<2;++index) {
        double distance=hypot(x-handles[index].x,y-handles[index].y);
        if (distance<=best) { best=distance; applet->captured=index; }
    }
    if (applet->captured<0) return;
    applet->pointer_id=pointer_id; applet->down_x=x; applet->down_y=y;
}

void i47_applet_move(I47Applet *applet,int pointer_id,double x,double y) {
    if (!applet->runtime_ok||applet->blocked||applet->captured<0||pointer_id!=applet->pointer_id) return;
    if (!applet->moved&&!touch_drag_threshold_exceeded((float)(x-applet->down_x),(float)(y-applet->down_y))) return;
    Point cursor=from_screen(applet,x,y);
    bool changed=false;
    if (!i47_lua_drag(applet->lua,applet->captured,cursor.x,cursor.y,&changed)) {
        remember_runtime_error(applet,"Lua drag failed");
        return;
    }
    applet->moved=true;
    if (!changed) return;
    if (!sync_from_lua(applet)) return;
    ++applet->drag_updates;
}

const char *i47_applet_error(const I47Applet *applet) {
    return applet&&applet->error[0]?applet->error:"";
}
