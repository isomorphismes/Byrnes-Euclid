// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/input.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <string.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"ByrneI47",__VA_ARGS__)

typedef struct { double blue_length,yellow_length; bool debug; } SavedState;
typedef struct { I47Applet applet; bool ready,dirty,focused; } Engine;

static bool init_from_lua_asset(struct android_app *app,I47Applet *applet) {
    if (!app->activity||!app->activity->assetManager) return false;
    AAsset *asset=AAssetManager_open(app->activity->assetManager,"book1_prop47.lua",AASSET_MODE_STREAMING);
    if (!asset) return false;
    off_t length=AAsset_getLength(asset);
    if (length<=0) { AAsset_close(asset); return false; }
    char *source=malloc((size_t)length);
    if (!source) { AAsset_close(asset); return false; }
    size_t offset=0;
    while (offset<(size_t)length) {
        int amount=AAsset_read(asset,source+offset,(size_t)length-offset);
        if (amount<=0) { free(source); AAsset_close(asset); return false; }
        offset+=(size_t)amount;
    }
    bool ok=i47_applet_init(applet,source,(size_t)length);
    free(source); AAsset_close(asset);
    return ok;
}

static void render(struct android_app *app) {
    Engine *engine=app->userData;
    if (!app->window||!engine->ready) return;
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(app->window,&buffer,NULL)!=0) return;
    if (buffer.width!=engine->applet.width||buffer.height!=engine->applet.height)
        i47_applet_size(&engine->applet,buffer.width,buffer.height,AConfiguration_getDensity(app->config));
    i47_applet_render(&engine->applet,buffer.bits,buffer.stride);
    int result=ANativeWindow_unlockAndPost(app->window);
    if (!result) LOG("frame presented %dx%d updates=%u area_error=%.3g",buffer.width,buffer.height,
                     engine->applet.drag_updates,engine->applet.construction.area_error);
    engine->dirty=false;
}

static int32_t input(struct android_app *app,AInputEvent *event) {
    Engine *engine=app->userData; I47Applet *applet=&engine->applet;
    if (AInputEvent_getType(event)!=AINPUT_EVENT_TYPE_MOTION) return 0;
    int action=AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_MASK;
    if (action==AMOTION_EVENT_ACTION_POINTER_DOWN) {
        i47_applet_cancel(applet); applet->blocked=true;
    } else if (action==AMOTION_EVENT_ACTION_UP||action==AMOTION_EVENT_ACTION_CANCEL) {
        i47_applet_cancel(applet); applet->blocked=false;
    } else if (action==AMOTION_EVENT_ACTION_DOWN) {
        applet->blocked=false;
        i47_applet_down(applet,AMotionEvent_getPointerId(event,0),AMotionEvent_getX(event,0),AMotionEvent_getY(event,0));
    } else if (action==AMOTION_EVENT_ACTION_MOVE) {
        unsigned before=applet->drag_updates;
        for (size_t index=0;index<AMotionEvent_getPointerCount(event);++index)
            i47_applet_move(applet,AMotionEvent_getPointerId(event,index),AMotionEvent_getX(event,index),AMotionEvent_getY(event,index));
        if (applet->drag_updates!=before) LOG("Lua leg drag blue=%.6f yellow=%.6f",applet->blue_length,applet->yellow_length);
        if (!applet->runtime_ok) LOG("Lua runtime failure: %s",i47_applet_error(applet));
    }
    engine->dirty=true; return 1;
}

static void command(struct android_app *app,int32_t command) {
    Engine *engine=app->userData;
    switch(command) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            ANativeWindow_setBuffersGeometry(app->window,0,0,WINDOW_FORMAT_RGBA_8888);
            i47_applet_size(&engine->applet,ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window),AConfiguration_getDensity(app->config));
            engine->ready=true; engine->dirty=true;
        } break;
    case APP_CMD_TERM_WINDOW: engine->ready=false; i47_applet_cancel(&engine->applet); break;
    case APP_CMD_GAINED_FOCUS: engine->focused=true; engine->dirty=true; break;
    case APP_CMD_LOST_FOCUS: engine->focused=false; i47_applet_cancel(&engine->applet); break;
    case APP_CMD_WINDOW_RESIZED: case APP_CMD_CONFIG_CHANGED: case APP_CMD_WINDOW_REDRAW_NEEDED:
        if (app->window) i47_applet_size(&engine->applet,ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window),AConfiguration_getDensity(app->config));
        engine->dirty=true; break;
    case APP_CMD_SAVE_STATE:
        app->savedState=malloc(sizeof(SavedState));
        if (app->savedState) {
            SavedState state={engine->applet.blue_length,engine->applet.yellow_length,engine->applet.debug};
            memcpy(app->savedState,&state,sizeof state); app->savedStateSize=sizeof state;
        } break;
    }
}

void android_main(struct android_app *app) {
    Engine engine={0}; app->userData=&engine;
    if (!init_from_lua_asset(app,&engine.applet)) {
        LOG("FAIL I.47 Lua initialization: %s",i47_applet_error(&engine.applet));
        ANativeActivity_finish(app->activity);
        return;
    }
    if (app->savedState&&app->savedStateSize==sizeof(SavedState)) {
        SavedState state; memcpy(&state,app->savedState,sizeof state);
        if (!i47_applet_restore(&engine.applet,state.blue_length,state.yellow_length,state.debug))
            LOG("saved Lua proposition state rejected");
    }
    app->onAppCmd=command; app->onInputEvent=input;
    LOG("native entry; C raster + Lua proposition; I.47");
    while (!app->destroyRequested) {
        int events; struct android_poll_source *source=NULL;
        int timeout=engine.ready&&engine.dirty?0:-1;
        int result=ALooper_pollOnce(timeout,NULL,&events,(void**)&source);
        if (result>=0&&source) source->process(app,source);
        if (app->destroyRequested) break;
        if (engine.ready&&engine.dirty) render(app);
    }
    i47_applet_close(&engine.applet);
}
