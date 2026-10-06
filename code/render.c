// SPDX-License-Identifier: GPL-3.0-or-later
#include "applet.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Source palette: byrnebook.cls byred/byblue/byyellow, rounded to 8-bit RGB.
   Android RGBA bytes are little-endian; host previews use the same buffer. */
#define RGB(red,green,blue) (0xff000000u|((uint32_t)(blue)<<16)|((uint32_t)(green)<<8)|(red))
static const uint32_t PAPER=RGB(250,246,231), BLACK=RGB(0,0,0),
    RED=RGB(217,77,26), BLUE=RGB(38,89,153), YELLOW=RGB(242,179,26);
typedef struct { uint32_t *pixels; int width,height,stride; } Canvas;
static void pixel(Canvas canvas,int x,int y,uint32_t colour,double coverage) {
    if (x<0 || x>=canvas.width || y<0 || y>=canvas.height || coverage<=0) return;
    coverage=fmin(1,coverage); uint32_t old=canvas.pixels[y*canvas.stride+x], mixed=0xff000000u;
    for (int shift=0;shift<24;shift+=8) {
        unsigned component=(unsigned)(((old>>shift)&255)*(1-coverage)+((colour>>shift)&255)*coverage+.5);
        mixed|=component<<shift;
    }
    canvas.pixels[y*canvas.stride+x]=mixed;
}
static void disk(Canvas canvas,Point centre,double radius,uint32_t colour) {
    int left=(int)fmax(0,floor(centre.x-radius-1)), right=(int)fmin(canvas.width-1,ceil(centre.x+radius+1));
    int top=(int)fmax(0,floor(centre.y-radius-1)), bottom=(int)fmin(canvas.height-1,ceil(centre.y+radius+1));
    for (int y=top;y<=bottom;++y) for (int x=left;x<=right;++x)
        pixel(canvas,x,y,colour,radius+.5-hypot(x+.5-centre.x,y+.5-centre.y));
}
static void stroke(Canvas canvas,Point first,Point second,double width,uint32_t colour,bool dashed) {
    double dx=second.x-first.x, dy=second.y-first.y, square=dx*dx+dy*dy, length=sqrt(square);
    int left=(int)fmax(0,floor(fmin(first.x,second.x)-width)), right=(int)fmin(canvas.width-1,ceil(fmax(first.x,second.x)+width));
    int top=(int)fmax(0,floor(fmin(first.y,second.y)-width)), bottom=(int)fmin(canvas.height-1,ceil(fmax(first.y,second.y)+width));
    for (int y=top;y<=bottom;++y) for (int x=left;x<=right;++x) {
        double parameter=square>0 ? fmax(0,fmin(1,((x+.5-first.x)*dx+(y+.5-first.y)*dy)/square)) : 0;
        if (dashed && ((int)(parameter*length/fmax(6,width*3))%2)) continue;
        double distance=hypot(x+.5-first.x-parameter*dx,y+.5-first.y-parameter*dy);
        pixel(canvas,x,y,colour,width*.5+.5-distance);
    }
}
static void ring(Canvas canvas,Point centre,double radius,double width,uint32_t colour) {
    int left=(int)fmax(0,floor(centre.x-radius-width)),right=(int)fmin(canvas.width-1,ceil(centre.x+radius+width));
    int top=(int)fmax(0,floor(centre.y-radius-width)),bottom=(int)fmin(canvas.height-1,ceil(centre.y+radius+width));
    for (int y=top;y<=bottom;++y) for (int x=left;x<=right;++x)
        pixel(canvas,x,y,colour,width*.5+.5-fabs(hypot(x+.5-centre.x,y+.5-centre.y)-radius));
}
/* Small fixed font for instructions/debug only. Diagram objects carry colour,
   not coordinate labels. Each byte is one column of a 5x7 uppercase glyph. */
static const unsigned char glyphs[][5]={
    {126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
    {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
    {65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
    {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
    {62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
    {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
    {3,4,120,4,3},{97,81,73,69,67},
    {62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},
    {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
    {54,73,73,73,54},{6,73,73,41,30}
};
static void text(Canvas canvas,int x,int y,int size,const char *message,uint32_t colour) {
    for (;*message;++message,x+=size*6) {
        unsigned char columns[5]={0}; unsigned char letter=(unsigned char)*message;
        if (letter>='a' && letter<='z') letter-=32;
        if (letter>='A' && letter<='Z') memcpy(columns,glyphs[letter-'A'],5);
        else if (letter>='0' && letter<='9') memcpy(columns,glyphs[26+letter-'0'],5);
        else if (letter=='.') columns[2]=64;
        else if (letter==':') columns[2]=20;
        else if (letter=='-') memset(columns,8,5);
        else if (letter=='+') { memset(columns,8,5); columns[2]=62; }
        else if (letter=='=') memset(columns,20,5);
        else if (letter=='/') { columns[0]=64;columns[1]=32;columns[2]=16;columns[3]=8;columns[4]=4; }
        for (int column=0;column<5;++column) for (int row=0;row<7;++row) if (columns[column]&(1<<row))
            for (int sy=0;sy<size;++sy) for (int sx=0;sx<size;++sx) pixel(canvas,x+column*size+sx,y+row*size+sy,colour,1);
    }
}
static void centre_text(Canvas canvas,int y,int size,const char *message,uint32_t colour) {
    text(canvas,(canvas.width-(int)strlen(message)*size*6)/2,y,size,message,colour);
}
void applet_render(const Applet *applet,uint32_t *pixels,int stride) {
    if (!pixels || applet->width<=0 || applet->height<=0 || stride<applet->width) return;
    Canvas canvas={pixels,applet->width,applet->height,stride};
    for (int y=0;y<canvas.height;++y) for (int x=0;x<canvas.width;++x) pixels[y*stride+x]=PAPER;
    double unit=canvas.width/576.0, line_width=fmax(2,4*unit);
    int font=(int)fmax(1,floor(canvas.width/250.0)), title=(int)fmax(1,floor(canvas.width/155.0));
    centre_text(canvas,(int)(canvas.height*.04),font,"BYRNE / EUCLID",BLACK);
    centre_text(canvas,(int)(canvas.height*.08),title,"BOOK III.1",BLACK);
    centre_text(canvas,(int)(canvas.height*.135),font,"FIND THE CENTRE OF A CIRCLE",BLACK);
    Point circle_centre=applet_to_screen(applet,(Point){0,0});
    double radius=point_distance(circle_centre,applet_to_screen(applet,circle_boundary(applet->circle,0)));
    ring(canvas,circle_centre,radius,line_width,BLUE);
    const Construction *construction=&applet->construction;
    if (construction->status==GEOMETRY_OK) {
        Point start=applet_to_screen(applet,construction->chord[0]), finish=applet_to_screen(applet,construction->chord[1]);
        Point middle=applet_to_screen(applet,construction->chord_midpoint);
        Point first_cut=applet_to_screen(applet,construction->intersections[0]), second_cut=applet_to_screen(applet,construction->intersections[1]);
        Point centre=applet_to_screen(applet,construction->constructed_centre);
        Point direction=construction->perpendicular.direction;
        Point extended_first={middle.x-direction.x*radius*2.5,middle.y-direction.y*radius*2.5};
        Point extended_second={middle.x+direction.x*radius*2.5,middle.y+direction.y*radius*2.5};
        /* Clip the extended line to the diagram's vertical band. */
        if (fabs(direction.y)>1e-9) {
            double upper=canvas.height*.19,lower=canvas.height*.64;
            double first=(upper-middle.y)/direction.y,second=(lower-middle.y)/direction.y;
            double low=fmax(-radius*2.5,fmin(first,second)),high=fmin(radius*2.5,fmax(first,second));
            extended_first=(Point){middle.x+direction.x*low,middle.y+direction.y*low};
            extended_second=(Point){middle.x+direction.x*high,middle.y+direction.y*high};
        }
        stroke(canvas,extended_first,extended_second,line_width*.55,BLACK,true);
        stroke(canvas,first_cut,second_cut,line_width,BLACK,false);
        stroke(canvas,start,middle,line_width,RED,false);
        stroke(canvas,middle,finish,line_width,RED,true);
        Point chord_direction={(finish.x-start.x)/point_distance(start,finish),(finish.y-start.y)/point_distance(start,finish)};
        double corner=14*unit;
        Point corner_first={middle.x+chord_direction.x*corner,middle.y+chord_direction.y*corner};
        Point corner_second={corner_first.x+direction.x*corner,corner_first.y+direction.y*corner};
        Point corner_third={middle.x+direction.x*corner,middle.y+direction.y*corner};
        stroke(canvas,corner_first,corner_second,line_width*.6,BLUE,false);
        stroke(canvas,corner_second,corner_third,line_width*.6,BLUE,false);
        ring(canvas,first_cut,8*unit,line_width,BLUE); ring(canvas,second_cut,8*unit,line_width,BLUE);
        disk(canvas,start,12*unit,PAPER); disk(canvas,start,8*unit,RED);
        disk(canvas,finish,12*unit,PAPER); disk(canvas,finish,8*unit,RED);
        disk(canvas,middle,7*unit,BLACK); disk(canvas,middle,5*unit,YELLOW);
        /* Centre: yellow annulus and black cross; chord midpoint: yellow disk.
           Both remain legible when a diametral chord makes them coincide. */
        ring(canvas,centre,13*unit,5*unit,YELLOW);
        stroke(canvas,(Point){centre.x-6*unit,centre.y},(Point){centre.x+6*unit,centre.y},2*unit,BLACK,false);
        stroke(canvas,(Point){centre.x,centre.y-6*unit},(Point){centre.x,centre.y+6*unit},2*unit,BLACK,false);
    }
    centre_text(canvas,(int)(canvas.height*.665),font,"DRAG EITHER RED ENDPOINT",BLACK);
    int left=(int)(canvas.width*.07), row=(int)(canvas.height*.04), base=(int)(canvas.height*.72);
    stroke(canvas,(Point){left,base+font*3},(Point){left+32*unit,base+font*3},line_width,RED,false);
    text(canvas,left+(int)(48*unit),base,font,"1  BISECT THE RED CHORD",BLACK);
    disk(canvas,(Point){left+16*unit,base+row+font*3},5*unit,YELLOW);
    text(canvas,left+(int)(48*unit),base+row,font,"2  DRAW THE PERPENDICULAR",BLACK);
    ring(canvas,(Point){left+16*unit,base+row*2+font*3},7*unit,3*unit,BLUE);
    text(canvas,left+(int)(48*unit),base+row*2,font,"3  BISECT THE BLACK DIAMETER",BLACK);
    centre_text(canvas,base+row*3,font,"THE YELLOW RING STAYS FIXED",BLACK);
    if (applet->debug) {
        int top=(int)(canvas.height*.87);
        stroke(canvas,(Point){left,top-8*unit},(Point){canvas.width-left,top-8*unit},1,BLACK,false);
        char message[100];
        snprintf(message,sizeof message,"%s MID %.1E DOT %.1E",geometry_status_name(construction->status),construction->midpoint_error,construction->perpendicular_error);
        text(canvas,left,top,font,message,BLACK);
        snprintf(message,sizeof message,"RADIUS %.1E DRIFT %.1E",construction->boundary_error,applet->maximum_drift);
        text(canvas,left,top+font*10,font,message,BLACK);
        snprintf(message,sizeof message,"EQUAL %.1E UPDATES %u",construction->equal_radius_error,applet->drag_updates);
        text(canvas,left,top+font*20,font,message,BLACK);
    } else if (construction->status!=GEOMETRY_OK) {
        centre_text(canvas,(int)(canvas.height*.88),font,geometry_status_name(construction->status),RED);
    }
    centre_text(canvas,(int)(canvas.height*.952),font,applet->debug ? "TAP HERE TO HIDE CHECKS" : "TAP HERE FOR NUMERICAL CHECKS",BLACK);
}
