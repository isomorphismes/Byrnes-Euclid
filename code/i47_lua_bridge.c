// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_lua_bridge.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct I47LuaRuntime {
    lua_State *state;
    int module_ref;
    int proposition_ref;
    char error[192];
};

static void copy_error(char *destination,size_t size,const char *message) {
    if (!destination||size==0) return;
    snprintf(destination,size,"%s",message?message:"unknown Lua error");
}

static void set_error(I47LuaRuntime *runtime,const char *context,const char *message) {
    if (!runtime) return;
    snprintf(runtime->error,sizeof runtime->error,"%s: %s",context,message?message:"unknown error");
}

static void set_stack_error(I47LuaRuntime *runtime,const char *context) {
    const char *message=runtime&&runtime->state?lua_tostring(runtime->state,-1):NULL;
    set_error(runtime,context,message);
    if (runtime&&runtime->state) lua_pop(runtime->state,1);
}

static void push_number_field(lua_State *state,const char *name,double value) {
    lua_pushnumber(state,value);
    lua_setfield(state,-2,name);
}

static void push_point(lua_State *state,Point point) {
    lua_createtable(state,0,2);
    push_number_field(state,"x",point.x);
    push_number_field(state,"y",point.y);
}

static void push_square(lua_State *state,const ByrneSquare *square) {
    lua_createtable(state,4,0);
    for (int index=0;index<4;++index) {
        push_point(state,square->vertex[index]);
        lua_rawseti(state,-2,index+1);
    }
}

static int lua_geometry_construct(lua_State *state) {
    double blue_length=luaL_checknumber(state,1);
    double yellow_length=luaL_checknumber(state,2);
    double rotation=luaL_checknumber(state,3);
    PythagorasConstruction construction;
    GeometryStatus status=construct_pythagoras((Point){0,0},blue_length,yellow_length,rotation,&construction);
    if (status!=GEOMETRY_OK) {
        lua_pushnil(state);
        lua_pushinteger(state,status);
        return 2;
    }

    lua_createtable(state,0,16);
    push_point(state,construction.right_angle); lua_setfield(state,-2,"right_angle");
    push_point(state,construction.blue_end); lua_setfield(state,-2,"blue_end");
    push_point(state,construction.yellow_end); lua_setfield(state,-2,"yellow_end");
    push_square(state,&construction.blue_square); lua_setfield(state,-2,"blue_square");
    push_square(state,&construction.yellow_square); lua_setfield(state,-2,"yellow_square");
    push_square(state,&construction.red_square); lua_setfield(state,-2,"red_square");
    push_point(state,construction.hypotenuse_foot); lua_setfield(state,-2,"hypotenuse_foot");
    push_point(state,construction.hypotenuse_far_cut); lua_setfield(state,-2,"hypotenuse_far_cut");
    push_number_field(state,"blue_length",construction.blue_length);
    push_number_field(state,"yellow_length",construction.yellow_length);
    push_number_field(state,"red_length",construction.red_length);
    push_number_field(state,"blue_area",construction.blue_area);
    push_number_field(state,"yellow_area",construction.yellow_area);
    push_number_field(state,"red_area",construction.red_area);
    push_number_field(state,"right_angle_error",construction.right_angle_error);
    push_number_field(state,"square_error",construction.square_error);
    push_number_field(state,"area_error",construction.area_error);
    return 1;
}

static void register_geometry(lua_State *state) {
    lua_createtable(state,0,1);
    lua_pushcfunction(state,lua_geometry_construct);
    lua_setfield(state,-2,"construct");
    lua_setglobal(state,"geometry");
}

static bool push_module_function(I47LuaRuntime *runtime,const char *name) {
    lua_State *state=runtime->state;
    lua_rawgeti(state,LUA_REGISTRYINDEX,runtime->module_ref);
    if (!lua_istable(state,-1)) {
        lua_pop(state,1);
        set_error(runtime,"Lua module","registry reference is not a table");
        return false;
    }
    lua_getfield(state,-1,name);
    lua_remove(state,-2);
    if (!lua_isfunction(state,-1)) {
        lua_pop(state,1);
        set_error(runtime,"Lua module",name);
        return false;
    }
    return true;
}

bool i47_lua_create(I47LuaRuntime **out,const char *source,size_t length,
                    char *error,size_t error_size) {
    if (!out||!source||length==0) {
        copy_error(error,error_size,"invalid Lua source");
        return false;
    }
    *out=NULL;
    I47LuaRuntime *runtime=calloc(1,sizeof *runtime);
    if (!runtime) {
        copy_error(error,error_size,"out of memory creating Lua runtime");
        return false;
    }
    runtime->module_ref=LUA_NOREF;
    runtime->proposition_ref=LUA_NOREF;
    runtime->state=luaL_newstate();
    if (!runtime->state) {
        copy_error(error,error_size,"luaL_newstate failed");
        free(runtime);
        return false;
    }

    luaL_requiref(runtime->state,"_G",luaopen_base,1);
    lua_pop(runtime->state,1);
    luaL_requiref(runtime->state,LUA_MATHLIBNAME,luaopen_math,1);
    lua_pop(runtime->state,1);
    register_geometry(runtime->state);

    if (luaL_loadbufferx(runtime->state,source,length,"@book1_prop47.lua","t")!=LUA_OK) {
        set_stack_error(runtime,"load book1_prop47.lua");
        copy_error(error,error_size,runtime->error);
        i47_lua_destroy(runtime);
        return false;
    }
    if (lua_pcall(runtime->state,0,1,0)!=LUA_OK) {
        set_stack_error(runtime,"execute book1_prop47.lua");
        copy_error(error,error_size,runtime->error);
        i47_lua_destroy(runtime);
        return false;
    }
    if (!lua_istable(runtime->state,-1)) {
        lua_pop(runtime->state,1);
        set_error(runtime,"execute book1_prop47.lua","module did not return a table");
        copy_error(error,error_size,runtime->error);
        i47_lua_destroy(runtime);
        return false;
    }
    runtime->module_ref=luaL_ref(runtime->state,LUA_REGISTRYINDEX);

    if (!push_module_function(runtime,"new")) {
        copy_error(error,error_size,runtime->error);
        i47_lua_destroy(runtime);
        return false;
    }
    if (lua_pcall(runtime->state,0,1,0)!=LUA_OK) {
        set_stack_error(runtime,"I.47 new");
        copy_error(error,error_size,runtime->error);
        i47_lua_destroy(runtime);
        return false;
    }
    if (!lua_istable(runtime->state,-1)) {
        lua_pop(runtime->state,1);
        set_error(runtime,"I.47 new","did not return state table");
        copy_error(error,error_size,runtime->error);
        i47_lua_destroy(runtime);
        return false;
    }
    runtime->proposition_ref=luaL_ref(runtime->state,LUA_REGISTRYINDEX);
    runtime->error[0]='\0';
    *out=runtime;
    return true;
}

void i47_lua_destroy(I47LuaRuntime *runtime) {
    if (!runtime) return;
    if (runtime->state) lua_close(runtime->state);
    free(runtime);
}

static bool read_number_field(lua_State *state,int index,const char *name,double *value) {
    int absolute=lua_absindex(state,index);
    lua_getfield(state,absolute,name);
    bool ok=lua_isnumber(state,-1);
    double result=ok?lua_tonumber(state,-1):NAN;
    lua_pop(state,1);
    if (!ok||!isfinite(result)) return false;
    *value=result;
    return true;
}

static bool read_bool_field(lua_State *state,int index,const char *name,bool *value) {
    int absolute=lua_absindex(state,index);
    lua_getfield(state,absolute,name);
    bool ok=lua_isboolean(state,-1);
    if (ok) *value=lua_toboolean(state,-1)!=0;
    lua_pop(state,1);
    return ok;
}

static bool read_string_field(lua_State *state,int index,const char *name,char *out,size_t capacity) {
    int absolute=lua_absindex(state,index);
    lua_getfield(state,absolute,name);
    size_t length=0;
    const char *value=lua_tolstring(state,-1,&length);
    bool ok=value&&length<capacity;
    if (ok) {
        memcpy(out,value,length);
        out[length]='\0';
    }
    lua_pop(state,1);
    return ok;
}

static bool read_point(lua_State *state,int index,Point *point) {
    if (!lua_istable(state,index)) return false;
    return read_number_field(state,index,"x",&point->x)&&read_number_field(state,index,"y",&point->y);
}

static bool read_point_field(lua_State *state,int index,const char *name,Point *point) {
    int absolute=lua_absindex(state,index);
    lua_getfield(state,absolute,name);
    bool ok=read_point(state,-1,point);
    lua_pop(state,1);
    return ok;
}

static bool read_square_field(lua_State *state,int index,const char *name,ByrneSquare *square) {
    int absolute=lua_absindex(state,index);
    lua_getfield(state,absolute,name);
    if (!lua_istable(state,-1)) { lua_pop(state,1); return false; }
    int table=lua_absindex(state,-1);
    for (int vertex=0;vertex<4;++vertex) {
        lua_rawgeti(state,table,vertex+1);
        bool ok=read_point(state,-1,&square->vertex[vertex]);
        lua_pop(state,1);
        if (!ok) { lua_pop(state,1); return false; }
    }
    lua_pop(state,1);
    return true;
}

static bool read_construction(lua_State *state,int index,PythagorasConstruction *construction) {
    PythagorasConstruction value={0};
    if (!read_point_field(state,index,"right_angle",&value.right_angle)||
        !read_point_field(state,index,"blue_end",&value.blue_end)||
        !read_point_field(state,index,"yellow_end",&value.yellow_end)||
        !read_square_field(state,index,"blue_square",&value.blue_square)||
        !read_square_field(state,index,"yellow_square",&value.yellow_square)||
        !read_square_field(state,index,"red_square",&value.red_square)||
        !read_point_field(state,index,"hypotenuse_foot",&value.hypotenuse_foot)||
        !read_point_field(state,index,"hypotenuse_far_cut",&value.hypotenuse_far_cut)||
        !read_number_field(state,index,"blue_length",&value.blue_length)||
        !read_number_field(state,index,"yellow_length",&value.yellow_length)||
        !read_number_field(state,index,"red_length",&value.red_length)||
        !read_number_field(state,index,"blue_area",&value.blue_area)||
        !read_number_field(state,index,"yellow_area",&value.yellow_area)||
        !read_number_field(state,index,"red_area",&value.red_area)||
        !read_number_field(state,index,"right_angle_error",&value.right_angle_error)||
        !read_number_field(state,index,"square_error",&value.square_error)||
        !read_number_field(state,index,"area_error",&value.area_error)) return false;
    value.status=GEOMETRY_OK;
    *construction=value;
    return true;
}

bool i47_lua_sync(I47LuaRuntime *runtime,
                  double *blue_length,double *yellow_length,double *rotation,
                  bool *debug,double *maximum_area_error,
                  PythagorasConstruction *construction) {
    if (!runtime||!runtime->state||!blue_length||!yellow_length||!rotation||
        !debug||!maximum_area_error||!construction) return false;
    lua_State *state=runtime->state;
    int top=lua_gettop(state);
    lua_rawgeti(state,LUA_REGISTRYINDEX,runtime->proposition_ref);
    bool ok=lua_istable(state,-1)&&
        read_number_field(state,-1,"blue_length",blue_length)&&
        read_number_field(state,-1,"yellow_length",yellow_length)&&
        read_number_field(state,-1,"rotation",rotation)&&
        read_bool_field(state,-1,"debug",debug)&&
        read_number_field(state,-1,"maximum_area_error",maximum_area_error);
    if (ok) {
        lua_getfield(state,-1,"construction");
        ok=lua_istable(state,-1)&&read_construction(state,-1,construction);
        lua_pop(state,1);
    }
    lua_settop(state,top);
    if (!ok) set_error(runtime,"I.47 sync","state table violated the C/Lua contract");
    return ok;
}

bool i47_lua_restore(I47LuaRuntime *runtime,double blue_length,double yellow_length,
                     bool debug,bool *accepted) {
    if (!runtime||!accepted) return false;
    lua_State *state=runtime->state; int top=lua_gettop(state);
    if (!push_module_function(runtime,"restore")) { lua_settop(state,top); return false; }
    lua_rawgeti(state,LUA_REGISTRYINDEX,runtime->proposition_ref);
    lua_pushnumber(state,blue_length); lua_pushnumber(state,yellow_length); lua_pushboolean(state,debug);
    if (lua_pcall(state,4,1,0)!=LUA_OK) {
        set_stack_error(runtime,"I.47 restore"); lua_settop(state,top); return false;
    }
    if (!lua_isboolean(state,-1)) {
        set_error(runtime,"I.47 restore","did not return boolean"); lua_settop(state,top); return false;
    }
    *accepted=lua_toboolean(state,-1)!=0; lua_settop(state,top); return true;
}

bool i47_lua_toggle_checks(I47LuaRuntime *runtime,bool *debug) {
    if (!runtime||!debug) return false;
    lua_State *state=runtime->state; int top=lua_gettop(state);
    if (!push_module_function(runtime,"toggle_checks")) { lua_settop(state,top); return false; }
    lua_rawgeti(state,LUA_REGISTRYINDEX,runtime->proposition_ref);
    if (lua_pcall(state,1,1,0)!=LUA_OK) {
        set_stack_error(runtime,"I.47 toggle_checks"); lua_settop(state,top); return false;
    }
    if (!lua_isboolean(state,-1)) {
        set_error(runtime,"I.47 toggle_checks","did not return boolean"); lua_settop(state,top); return false;
    }
    *debug=lua_toboolean(state,-1)!=0; lua_settop(state,top); return true;
}

bool i47_lua_drag(I47LuaRuntime *runtime,int captured,double model_x,double model_y,
                  bool *changed) {
    if (!runtime||!changed) return false;
    lua_State *state=runtime->state; int top=lua_gettop(state);
    if (!push_module_function(runtime,"drag")) { lua_settop(state,top); return false; }
    lua_rawgeti(state,LUA_REGISTRYINDEX,runtime->proposition_ref);
    lua_pushinteger(state,captured); lua_pushnumber(state,model_x); lua_pushnumber(state,model_y);
    if (lua_pcall(state,4,1,0)!=LUA_OK) {
        set_stack_error(runtime,"I.47 drag"); lua_settop(state,top); return false;
    }
    if (!lua_isboolean(state,-1)) {
        set_error(runtime,"I.47 drag","did not return boolean"); lua_settop(state,top); return false;
    }
    *changed=lua_toboolean(state,-1)!=0; lua_settop(state,top); return true;
}

static bool read_colour(const char *name,I47ColourRole *role) {
    if (strcmp(name,"black")==0) *role=I47_COLOUR_BLACK;
    else if (strcmp(name,"red")==0) *role=I47_COLOUR_RED;
    else if (strcmp(name,"blue")==0) *role=I47_COLOUR_BLUE;
    else if (strcmp(name,"yellow")==0) *role=I47_COLOUR_YELLOW;
    else if (strcmp(name,"paper")==0) *role=I47_COLOUR_PAPER;
    else return false;
    return true;
}

static bool read_colour_field(lua_State *state,int index,const char *name,I47ColourRole *role) {
    char value[16];
    return read_string_field(state,index,name,value,sizeof value)&&read_colour(value,role);
}

bool i47_lua_presentation(I47LuaRuntime *runtime,int width,int height,
                          I47Presentation *presentation) {
    if (!runtime||!presentation||width<=0||height<=0) return false;
    lua_State *state=runtime->state; int top=lua_gettop(state);
    if (!push_module_function(runtime,"presentation")) { lua_settop(state,top); return false; }
    lua_rawgeti(state,LUA_REGISTRYINDEX,runtime->proposition_ref);
    lua_pushinteger(state,width); lua_pushinteger(state,height);
    if (lua_pcall(state,3,1,0)!=LUA_OK) {
        set_stack_error(runtime,"I.47 presentation"); lua_settop(state,top); return false;
    }
    if (!lua_istable(state,-1)) {
        set_error(runtime,"I.47 presentation","did not return table"); lua_settop(state,top); return false;
    }
    int root=lua_absindex(state,-1);
    I47Presentation value={0};
    bool ok=
        read_number_field(state,root,"model_centre_x",&value.model_centre_x)&&
        read_number_field(state,root,"model_centre_y",&value.model_centre_y)&&
        read_number_field(state,root,"screen_centre_x",&value.screen_centre_x)&&
        read_number_field(state,root,"screen_centre_y",&value.screen_centre_y)&&
        read_number_field(state,root,"scale",&value.scale)&&value.scale>0;

    lua_getfield(state,root,"checks");
    ok=ok&&lua_istable(state,-1)&&
        read_number_field(state,-1,"left",&value.checks_left)&&
        read_number_field(state,-1,"top",&value.checks_top)&&
        read_number_field(state,-1,"right",&value.checks_right)&&
        read_number_field(state,-1,"bottom",&value.checks_bottom);
    lua_pop(state,1);

    lua_getfield(state,root,"y");
    ok=ok&&lua_istable(state,-1)&&
        read_number_field(state,-1,"brand",&value.brand_y)&&
        read_number_field(state,-1,"title",&value.title_y)&&
        read_number_field(state,-1,"subtitle",&value.subtitle_y)&&
        read_number_field(state,-1,"instruction",&value.instruction_y)&&
        read_number_field(state,-1,"legend",&value.legend_y)&&
        read_number_field(state,-1,"debug",&value.debug_y);
    lua_pop(state,1);

    lua_getfield(state,root,"labels");
    ok=ok&&lua_istable(state,-1)&&
        read_string_field(state,-1,"brand",value.brand,sizeof value.brand)&&
        read_string_field(state,-1,"title",value.title,sizeof value.title)&&
        read_string_field(state,-1,"subtitle",value.subtitle,sizeof value.subtitle)&&
        read_string_field(state,-1,"instruction",value.instruction,sizeof value.instruction)&&
        read_string_field(state,-1,"equation",value.equation,sizeof value.equation)&&
        read_string_field(state,-1,"motion",value.motion,sizeof value.motion)&&
        read_string_field(state,-1,"checks_on",value.checks_on,sizeof value.checks_on)&&
        read_string_field(state,-1,"checks_off",value.checks_off,sizeof value.checks_off);
    lua_pop(state,1);

    lua_getfield(state,root,"colours");
    ok=ok&&lua_istable(state,-1)&&
        read_colour_field(state,-1,"hypotenuse_square",&value.hypotenuse_square_colour)&&
        read_colour_field(state,-1,"blue_square",&value.blue_square_colour)&&
        read_colour_field(state,-1,"yellow_square",&value.yellow_square_colour)&&
        read_colour_field(state,-1,"blue_leg",&value.blue_leg_colour)&&
        read_colour_field(state,-1,"yellow_leg",&value.yellow_leg_colour)&&
        read_colour_field(state,-1,"hypotenuse",&value.hypotenuse_colour)&&
        read_colour_field(state,-1,"proof",&value.proof_colour);
    lua_pop(state,1);

    lua_settop(state,top);
    if (!ok) {
        set_error(runtime,"I.47 presentation","table violated the C/Lua contract");
        return false;
    }
    *presentation=value;
    return true;
}

const char *i47_lua_last_error(const I47LuaRuntime *runtime) {
    return runtime&&runtime->error[0]?runtime->error:"";
}
