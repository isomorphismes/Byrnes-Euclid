// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path,size_t *length) {
    FILE *file=fopen(path,"rb"); if (!file) return NULL;
    if (fseek(file,0,SEEK_END)!=0) { fclose(file); return NULL; }
    long size=ftell(file); if (size<=0||fseek(file,0,SEEK_SET)!=0) { fclose(file); return NULL; }
    char *content=malloc((size_t)size); if (!content) { fclose(file); return NULL; }
    if (fread(content,1,(size_t)size,file)!=(size_t)size) { free(content); fclose(file); return NULL; }
    fclose(file); *length=(size_t)size; return content;
}

int main(int argc,char **argv) {
    if (argc!=3) return 2;
    size_t lua_length=0; char *lua_source=read_file(argv[1],&lua_length); if (!lua_source) return 1;
    I47Applet a; if (!i47_applet_init(&a,lua_source,lua_length)) { free(lua_source); return 1; }
    free(lua_source);
    i47_applet_size(&a,576,1152,160);
    if (!i47_applet_restore(&a,a.blue_length,a.yellow_length,true)) { i47_applet_close(&a); return 1; }
    uint32_t *pixels=calloc(576*1152,sizeof *pixels); if (!pixels) { i47_applet_close(&a); return 1; }
    i47_applet_render(&a,pixels,576);
    FILE *file=fopen(argv[2],"wb"); if (!file) { free(pixels); i47_applet_close(&a); return 1; }
    fprintf(file,"P6\n576 1152\n255\n");
    for (int i=0;i<576*1152;++i) {
        unsigned char rgb[3]={(unsigned char)pixels[i],(unsigned char)(pixels[i]>>8),(unsigned char)(pixels[i]>>16)};
        if (fwrite(rgb,1,3,file)!=3) { fclose(file); free(pixels); i47_applet_close(&a); return 1; }
    }
    int result=fclose(file); free(pixels); i47_applet_close(&a); return result!=0;
}
