// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_I47_APPLET_H
#define BYRNE_I47_APPLET_H
#include "i47_lua_bridge.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct { double left,top,right,bottom; } I47AppletRect;
typedef struct {
    I47LuaRuntime *lua;
    I47Presentation presentation;
    double blue_length;
    double yellow_length;
    double rotation;
    PythagorasConstruction construction;
    double maximum_area_error;
    int width,height,density,captured,pointer_id;
    double down_x,down_y;
    bool moved,debug,blocked,runtime_ok;
    unsigned drag_updates;

    /* Compact width×height copy of the static Byrne page. It deliberately
       excludes state-dependent text; every ANativeWindow buffer still gets a
       complete row-wise copy before dynamic geometry is drawn. */
    uint32_t *static_pixels;
    int static_width,static_height;
    unsigned static_rebuilds;

    char error[192];
} I47Applet;

bool i47_applet_init(I47Applet *applet,const char *lua_source,size_t lua_length);
void i47_applet_close(I47Applet *applet);
bool i47_applet_restore(I47Applet *applet,double blue_length,double yellow_length,bool debug);
void i47_applet_size(I47Applet *applet,int width,int height,int density);
Point i47_applet_to_screen(const I47Applet *applet,Point point);
I47AppletRect i47_applet_checks_bounds(const I47Applet *applet);
void i47_applet_down(I47Applet *applet,int pointer_id,double x,double y);
void i47_applet_move(I47Applet *applet,int pointer_id,double x,double y);
void i47_applet_cancel(I47Applet *applet);
void i47_applet_render(I47Applet *applet,uint32_t *pixels,int stride);
const char *i47_applet_error(const I47Applet *applet);

#endif
