// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    I47Applet a; if (!i47_applet_init(&a)) return 1;
    i47_applet_size(&a,576,1152,160); a.debug=true;
    uint32_t *pixels=calloc(576*1152,sizeof *pixels); if (!pixels) return 1;
    i47_applet_render(&a,pixels,576);
    FILE *file=fopen(argv[1],"wb"); if (!file) return 1;
    fprintf(file,"P6\n576 1152\n255\n");
    for (int i=0;i<576*1152;++i) {
        unsigned char rgb[3]={(unsigned char)pixels[i],(unsigned char)(pixels[i]>>8),(unsigned char)(pixels[i]>>16)};
        if (fwrite(rgb,1,3,file)!=3) return 1;
    }
    int result=fclose(file); free(pixels); return result!=0;
}
