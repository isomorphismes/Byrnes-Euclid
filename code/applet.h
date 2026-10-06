// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_APPLET_H
#define BYRNE_APPLET_H
#include "proposition1.h"
#include <stdint.h>
typedef struct { double left, top, right, bottom; } AppletRect;
typedef struct {
    Circle *circle;
    double angles[2];
    Construction construction;
    Point reference_centre;
    double maximum_drift;
    int width, height, density, captured, pointer_id;
    double down_x, down_y, down_angle, original_angle;
    bool moved, debug, blocked;
    unsigned drag_updates;
} Applet;
bool applet_init(Applet *applet);
void applet_size(Applet *applet, int width, int height, int density);
Point applet_to_screen(const Applet *applet, Point point);
AppletRect applet_checks_bounds(const Applet *applet);
void applet_down(Applet *applet, int pointer_id, double x, double y);
void applet_move(Applet *applet, int pointer_id, double x, double y);
void applet_cancel(Applet *applet);
void applet_render(const Applet *applet, uint32_t *pixels, int stride);
#endif
