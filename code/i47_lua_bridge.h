// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNE_I47_LUA_BRIDGE_H
#define BYRNE_I47_LUA_BRIDGE_H

#include "proposition47.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct I47LuaRuntime I47LuaRuntime;

typedef enum {
    I47_COLOUR_BLACK,
    I47_COLOUR_RED,
    I47_COLOUR_BLUE,
    I47_COLOUR_YELLOW,
    I47_COLOUR_PAPER
} I47ColourRole;

typedef struct {
    double model_centre_x, model_centre_y;
    double screen_centre_x, screen_centre_y;
    double scale;
    double checks_left, checks_top, checks_right, checks_bottom;
    double brand_y, title_y, subtitle_y, instruction_y, legend_y, debug_y;
    char brand[32];
    char title[32];
    char subtitle[64];
    char instruction[64];
    char equation[64];
    char motion[64];
    char checks_on[24];
    char checks_off[24];
    I47ColourRole hypotenuse_square_colour;
    I47ColourRole blue_square_colour;
    I47ColourRole yellow_square_colour;
    I47ColourRole blue_leg_colour;
    I47ColourRole yellow_leg_colour;
    I47ColourRole hypotenuse_colour;
    I47ColourRole proof_colour;
} I47Presentation;

bool i47_lua_create(I47LuaRuntime **out, const char *source, size_t length,
                    char *error, size_t error_size);
void i47_lua_destroy(I47LuaRuntime *runtime);
bool i47_lua_sync(I47LuaRuntime *runtime,
                  double *blue_length, double *yellow_length, double *rotation,
                  bool *debug, double *maximum_area_error,
                  PythagorasConstruction *construction);
bool i47_lua_restore(I47LuaRuntime *runtime, double blue_length, double yellow_length,
                     bool debug, bool *accepted);
bool i47_lua_toggle_checks(I47LuaRuntime *runtime, bool *debug);
bool i47_lua_drag(I47LuaRuntime *runtime, int captured, double model_x, double model_y,
                  bool *changed);
bool i47_lua_presentation(I47LuaRuntime *runtime, int width, int height,
                          I47Presentation *presentation);
const char *i47_lua_last_error(const I47LuaRuntime *runtime);

#endif
