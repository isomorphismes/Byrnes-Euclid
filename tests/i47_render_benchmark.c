// SPDX-License-Identifier: GPL-3.0-or-later
#define _POSIX_C_SOURCE 200809L
#include "i47_applet.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static char *read_file(const char *path,size_t *length) {
    FILE *file=fopen(path,"rb"); if (!file) return NULL;
    if (fseek(file,0,SEEK_END)!=0) { fclose(file); return NULL; }
    long size=ftell(file); if (size<=0||fseek(file,0,SEEK_SET)!=0) { fclose(file); return NULL; }
    char *content=malloc((size_t)size); if (!content) { fclose(file); return NULL; }
    if (fread(content,1,(size_t)size,file)!=(size_t)size) { free(content); fclose(file); return NULL; }
    fclose(file); *length=(size_t)size; return content;
}

static double milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC,&now)!=0) return 0;
    return (double)now.tv_sec*1000.0+(double)now.tv_nsec/1000000.0;
}

int main(int argc,char **argv) {
    if (argc!=2) return 2;
    size_t lua_length=0; char *lua_source=read_file(argv[1],&lua_length); if (!lua_source) return 1;
    I47Applet a; if (!i47_applet_init(&a,lua_source,lua_length)) { free(lua_source); return 1; }
    free(lua_source);

    const int width=720,height=1600,frames=180;
    i47_applet_size(&a,width,height,320);
    size_t count=(size_t)width*(size_t)height;
    uint32_t *pixels=calloc(count,sizeof *pixels); if (!pixels) { i47_applet_close(&a); return 1; }

    Point blue=i47_applet_to_screen(&a,a.construction.blue_end);
    i47_applet_down(&a,7,blue.x,blue.y);
    if (a.captured!=0) { free(pixels); i47_applet_close(&a); return 1; }

    /* Warm the static page cache and code paths before measuring drag frames. */
    for (int frame=0;frame<5;++frame) i47_applet_render(&a,pixels,width);

    double start=milliseconds();
    for (int frame=0;frame<frames;++frame) {
        double phase=(double)(frame%90)/89.0;
        double length=(frame/90)%2==0 ? .40+.90*phase : 1.30-.90*phase;
        Point target=i47_applet_to_screen(&a,(Point){length,0});
        i47_applet_move(&a,7,target.x,target.y);
        i47_applet_render(&a,pixels,width);
    }
    double elapsed=milliseconds()-start;

    uint64_t checksum=0;
    for (size_t i=0;i<count;i+=997) checksum=checksum*1315423911u+pixels[i];
    printf("I47_RENDER_BENCHMARK frames=%d width=%d height=%d milliseconds=%.3f frames_per_second=%.3f checksum=%llu\n",
           frames,width,height,elapsed,frames*1000.0/elapsed,(unsigned long long)checksum);

    free(pixels); i47_applet_close(&a);
    return elapsed<=0;
}
