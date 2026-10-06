// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_I47_APPLET_H
#define BYRNE_I47_APPLET_H
#include "proposition47.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct { double left,top,right,bottom; } I47AppletRect;
typedef struct {
    double blue_length;
    double yellow_length;
    double rotation;
    PythagorasConstruction construction;
    double maximum_area_error;
    int width,height,density,captured,pointer_id;
    double down_x,down_y,original_length;
    bool moved,debug,blocked;
    unsigned drag_updates;
} I47Applet;

bool i47_applet_init(I47Applet *applet);
bool i47_applet_restore(I47Applet *applet,double blue_length,double yellow_length,bool debug);
void i47_applet_size(I47Applet *applet,int width,int height,int density);
Point i47_applet_to_screen(const I47Applet *applet,Point point);
I47AppletRect i47_applet_checks_bounds(const I47Applet *applet);
void i47_applet_down(I47Applet *applet,int pointer_id,double x,double y);
void i47_applet_move(I47Applet *applet,int pointer_id,double x,double y);
void i47_applet_cancel(I47Applet *applet);
void i47_applet_render(const I47Applet *applet,uint32_t *pixels,int stride);

#endif
