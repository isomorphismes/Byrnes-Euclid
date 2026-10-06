// SPDX-License-Identifier: GPL-3.0-or-later
#include "applet.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    Applet applet; if (!applet_init(&applet)) return 1;
    applet_size(&applet,576,1152,160); applet.debug=true;
    uint32_t *pixels=calloc(576*1152,sizeof *pixels); if (!pixels) return 1;
    applet_render(&applet,pixels,576);
    FILE *file=fopen(argv[1],"wb"); if (!file) return 1;
    fprintf(file,"P6\n576 1152\n255\n");
    for (int index=0;index<576*1152;++index) {
        unsigned char rgb[3]={(unsigned char)pixels[index],(unsigned char)(pixels[index]>>8),(unsigned char)(pixels[index]>>16)};
        if (fwrite(rgb,1,3,file)!=3) return 1;
    }
    int result=fclose(file); free(pixels); circle_destroy(applet.circle); return result!=0;
}
