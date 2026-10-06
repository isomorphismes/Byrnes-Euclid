// SPDX-License-Identifier: GPL-3.0-or-later
#include "applet.h"
#include <android/log.h>
#include <android/input.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <string.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"ByrneNative",__VA_ARGS__)
typedef struct { double angles[2]; bool debug; } SavedState;
typedef struct { Applet applet; bool ready, dirty, focused; } Engine;
static void render(struct android_app *app) {
    Engine *engine=app->userData;
    if (!app->window || !engine->ready) return;
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(app->window,&buffer,NULL)!=0) return;
    if (buffer.width!=engine->applet.width || buffer.height!=engine->applet.height)
        applet_size(&engine->applet,buffer.width,buffer.height,AConfiguration_getDensity(app->config));
    applet_render(&engine->applet,buffer.bits,buffer.stride);
    int result=ANativeWindow_unlockAndPost(app->window);
    if (!result) LOG("frame presented %dx%d updates=%u status=%s drift=%.3g",buffer.width,buffer.height,
                     engine->applet.drag_updates,geometry_status_name(engine->applet.construction.status),engine->applet.maximum_drift);
    engine->dirty=false;
}
static int32_t input(struct android_app *app,AInputEvent *event) {
    Engine *engine=app->userData; Applet *applet=&engine->applet;
    if (AInputEvent_getType(event)!=AINPUT_EVENT_TYPE_MOTION) return 0;
    int action=AMotionEvent_getAction(event)&AMOTION_EVENT_ACTION_MASK;
    if (action==AMOTION_EVENT_ACTION_POINTER_DOWN) {
        applet_cancel(applet); applet->blocked=true;
    } else if (action==AMOTION_EVENT_ACTION_UP || action==AMOTION_EVENT_ACTION_CANCEL) {
        applet_cancel(applet); applet->blocked=false;
    } else if (action==AMOTION_EVENT_ACTION_DOWN) {
        applet->blocked=false;
        applet_down(applet,AMotionEvent_getPointerId(event,0),AMotionEvent_getX(event,0),AMotionEvent_getY(event,0));
    } else if (action==AMOTION_EVENT_ACTION_MOVE) {
        unsigned before=applet->drag_updates;
        for (size_t index=0;index<AMotionEvent_getPointerCount(event);++index)
            applet_move(applet,AMotionEvent_getPointerId(event,index),AMotionEvent_getX(event,index),AMotionEvent_getY(event,index));
        if (applet->drag_updates!=before) LOG("drag updated chord; centre=(%.17g,%.17g)",
                  applet->construction.constructed_centre.x,applet->construction.constructed_centre.y);
    }
    engine->dirty=true; return 1;
}
static void command(struct android_app *app,int32_t command) {
    Engine *engine=app->userData;
    switch(command) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            ANativeWindow_setBuffersGeometry(app->window,0,0,WINDOW_FORMAT_RGBA_8888);
            applet_size(&engine->applet,ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window),AConfiguration_getDensity(app->config));
            engine->ready=true; engine->dirty=true;
        } break;
    case APP_CMD_TERM_WINDOW: engine->ready=false; applet_cancel(&engine->applet); break;
    case APP_CMD_GAINED_FOCUS: engine->focused=true; engine->dirty=true; break;
    case APP_CMD_LOST_FOCUS: engine->focused=false; applet_cancel(&engine->applet); break;
    case APP_CMD_WINDOW_RESIZED: case APP_CMD_CONFIG_CHANGED: case APP_CMD_WINDOW_REDRAW_NEEDED:
        if (app->window) applet_size(&engine->applet,ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window),AConfiguration_getDensity(app->config));
        engine->dirty=true; break;
    case APP_CMD_SAVE_STATE:
        app->savedState=malloc(sizeof(SavedState));
        if (app->savedState) {
            SavedState state={{engine->applet.angles[0],engine->applet.angles[1]},engine->applet.debug};
            memcpy(app->savedState,&state,sizeof state); app->savedStateSize=sizeof state;
        } break;
    }
}
void android_main(struct android_app *app) {
    Engine engine={0}; app->userData=&engine;
    if (!applet_init(&engine.applet)) { LOG("FAIL geometry initialization"); ANativeActivity_finish(app->activity); return; }
    if (app->savedState && app->savedStateSize==sizeof(SavedState)) {
        SavedState state; memcpy(&state,app->savedState,sizeof state);
        Construction result;
        if (construct_centre(engine.applet.circle,circle_boundary(engine.applet.circle,state.angles[0]),circle_boundary(engine.applet.circle,state.angles[1]),&result)==GEOMETRY_OK) {
            memcpy(engine.applet.angles,state.angles,sizeof state.angles);
            engine.applet.construction=result; engine.applet.debug=state.debug;
        }
    }
    app->onAppCmd=command; app->onInputEvent=input;
    LOG("native entry; C software renderer; III.1");
    while (!app->destroyRequested) {
        int events; struct android_poll_source *source=NULL;
        int timeout=engine.ready && engine.dirty ? 0 : -1;
        int result=ALooper_pollOnce(timeout,NULL,&events,(void**)&source);
        if (result>=0 && source) source->process(app,source);
        if (app->destroyRequested) break;
        if (engine.ready && engine.dirty) render(app);
    }
    circle_destroy(engine.applet.circle);
}
