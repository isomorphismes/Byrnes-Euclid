// SPDX-License-Identifier: GPL-3.0-or-later
#include "i47_applet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RGB(red,green,blue) (0xff000000u|((uint32_t)(blue)<<16)|((uint32_t)(green)<<8)|(red))
static const uint32_t PAPER=RGB(250,246,231),BLACK=RGB(0,0,0),
    RED=RGB(217,77,26),BLUE=RGB(38,89,153),YELLOW=RGB(242,179,26);
typedef struct { uint32_t *pixels; int width,height,stride; } Canvas;

static uint32_t palette(I47ColourRole role) {
    switch(role) {
    case I47_COLOUR_RED: return RED;
    case I47_COLOUR_BLUE: return BLUE;
    case I47_COLOUR_YELLOW: return YELLOW;
    case I47_COLOUR_PAPER: return PAPER;
    case I47_COLOUR_BLACK: default: return BLACK;
    }
}

static void pixel(Canvas c,int x,int y,uint32_t colour,double coverage) {
    if (x<0||x>=c.width||y<0||y>=c.height||coverage<=0) return;
    coverage=fmin(1,coverage); uint32_t old=c.pixels[y*c.stride+x],mixed=0xff000000u;
    for (int shift=0;shift<24;shift+=8) {
        unsigned component=(unsigned)(((old>>shift)&255)*(1-coverage)+((colour>>shift)&255)*coverage+.5);
        mixed|=component<<shift;
    }
    c.pixels[y*c.stride+x]=mixed;
}

static void covered_distance_pixel(Canvas c,int x,int y,uint32_t colour,
                                   double inner,double outer,double dx,double dy) {
    double square=dx*dx+dy*dy;
    if (square>=outer*outer) return;
    if (inner>0&&square<=inner*inner) {
        pixel(c,x,y,colour,1);
        return;
    }
    pixel(c,x,y,colour,outer-hypot(dx,dy));
}

static void disk(Canvas c,Point centre,double radius,uint32_t colour) {
    double inner=radius-.5,outer=radius+.5;
    int left=(int)fmax(0,floor(centre.x-radius-1)),right=(int)fmin(c.width-1,ceil(centre.x+radius+1));
    int top=(int)fmax(0,floor(centre.y-radius-1)),bottom=(int)fmin(c.height-1,ceil(centre.y+radius+1));
    for (int y=top;y<=bottom;++y) for (int x=left;x<=right;++x)
        covered_distance_pixel(c,x,y,colour,inner,outer,x+.5-centre.x,y+.5-centre.y);
}

static void stroke_candidate(Canvas c,Point first,double dx,double dy,double square,double length,
                             double width,uint32_t colour,bool dashed,int x,int y) {
    double t=square>0?fmax(0,fmin(1,((x+.5-first.x)*dx+(y+.5-first.y)*dy)/square)):0;
    if (dashed&&((int)(t*length/fmax(6,width*3))%2)) return;
    double nearest_x=first.x+t*dx,nearest_y=first.y+t*dy;
    double inner=width*.5-.5,outer=width*.5+.5;
    covered_distance_pixel(c,x,y,colour,inner,outer,x+.5-nearest_x,y+.5-nearest_y);
}

static void stroke(Canvas c,Point first,Point second,double width,uint32_t colour,bool dashed) {
    double dx=second.x-first.x,dy=second.y-first.y,square=dx*dx+dy*dy,length=sqrt(square);
    double outer=width*.5+.5;
    if (square==0) {
        disk(c,first,width*.5,colour);
        return;
    }

    if (fabs(dx)>=fabs(dy)) {
        int left=(int)fmax(0,floor(fmin(first.x,second.x)-outer-1));
        int right=(int)fmin(c.width-1,ceil(fmax(first.x,second.x)+outer+1));
        double interior_span=outer*length/fabs(dx)+1;
        for (int x=left;x<=right;++x) {
            double axis_t=(x+.5-first.x)/dx;
            double centre_y,span;
            if (axis_t<0) { centre_y=first.y; span=outer+1; }
            else if (axis_t>1) { centre_y=second.y; span=outer+1; }
            else { centre_y=first.y+axis_t*dy; span=interior_span; }
            int top=(int)fmax(0,floor(centre_y-span-1));
            int bottom=(int)fmin(c.height-1,ceil(centre_y+span+1));
            for (int y=top;y<=bottom;++y)
                stroke_candidate(c,first,dx,dy,square,length,width,colour,dashed,x,y);
        }
    } else {
        int top=(int)fmax(0,floor(fmin(first.y,second.y)-outer-1));
        int bottom=(int)fmin(c.height-1,ceil(fmax(first.y,second.y)+outer+1));
        double interior_span=outer*length/fabs(dy)+1;
        for (int y=top;y<=bottom;++y) {
            double axis_t=(y+.5-first.y)/dy;
            double centre_x,span;
            if (axis_t<0) { centre_x=first.x; span=outer+1; }
            else if (axis_t>1) { centre_x=second.x; span=outer+1; }
            else { centre_x=first.x+axis_t*dx; span=interior_span; }
            int left=(int)fmax(0,floor(centre_x-span-1));
            int right=(int)fmin(c.width-1,ceil(centre_x+span+1));
            for (int x=left;x<=right;++x)
                stroke_candidate(c,first,dx,dy,square,length,width,colour,dashed,x,y);
        }
    }
}

static double edge(Point a,Point b,double x,double y) { return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x); }
static void fill_triangle(Canvas c,Point a,Point b,Point d,uint32_t colour) {
    int left=(int)fmax(0,floor(fmin(a.x,fmin(b.x,d.x)))),right=(int)fmin(c.width-1,ceil(fmax(a.x,fmax(b.x,d.x))));
    int top=(int)fmax(0,floor(fmin(a.y,fmin(b.y,d.y)))),bottom=(int)fmin(c.height-1,ceil(fmax(a.y,fmax(b.y,d.y))));
    double orientation=edge(a,b,d.x,d.y);
    for (int y=top;y<=bottom;++y) for (int x=left;x<=right;++x) {
        double e0=edge(a,b,x+.5,y+.5),e1=edge(b,d,x+.5,y+.5),e2=edge(d,a,x+.5,y+.5);
        if ((orientation>=0&&e0>=0&&e1>=0&&e2>=0)||(orientation<0&&e0<=0&&e1<=0&&e2<=0)) pixel(c,x,y,colour,1);
    }
}
static void fill_quad(Canvas c,const Point p[4],uint32_t colour) {
    fill_triangle(c,p[0],p[1],p[2],colour); fill_triangle(c,p[0],p[2],p[3],colour);
}
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
static void text(Canvas c,int x,int y,int size,const char *message,uint32_t colour) {
    for (;*message;++message,x+=size*6) {
        unsigned char columns[5]={0}; unsigned char letter=(unsigned char)*message;
        if (letter>='a'&&letter<='z') letter-=32;
        if (letter>='A'&&letter<='Z') memcpy(columns,glyphs[letter-'A'],5);
        else if (letter>='0'&&letter<='9') memcpy(columns,glyphs[26+letter-'0'],5);
        else if (letter=='.') columns[2]=64;
        else if (letter==':') columns[2]=20;
        else if (letter=='-') memset(columns,8,5);
        else if (letter=='+') { memset(columns,8,5); columns[2]=62; }
        else if (letter=='=') memset(columns,20,5);
        else if (letter=='/') { columns[0]=64;columns[1]=32;columns[2]=16;columns[3]=8;columns[4]=4; }
        for (int col=0;col<5;++col) for (int row=0;row<7;++row) if (columns[col]&(1<<row))
            for (int sy=0;sy<size;++sy) for (int sx=0;sx<size;++sx) pixel(c,x+col*size+sx,y+row*size+sy,colour,1);
    }
}
static void centre_text(Canvas c,int y,int size,const char *message,uint32_t colour) {
    text(c,(c.width-(int)strlen(message)*size*6)/2,y,size,message,colour);
}
static void screen_square(const I47Applet *a,const ByrneSquare *square,Point out[4]) {
    for (int i=0;i<4;++i) out[i]=i47_applet_to_screen(a,square->vertex[i]);
}
static void outline_quad(Canvas c,const Point p[4],double width,uint32_t colour) {
    for (int i=0;i<4;++i) stroke(c,p[i],p[(i+1)%4],width,colour,false);
}
static void fill_paper(Canvas c) {
    for (int x=0;x<c.width;++x) c.pixels[x]=PAPER;
    for (int y=1;y<c.height;++y)
        memcpy(c.pixels+y*c.stride,c.pixels,(size_t)c.width*sizeof *c.pixels);
}
static void draw_post_dynamic_overlay(Canvas c,const I47Applet *a,double unit,int font) {
    const I47Presentation *view=&a->presentation;
    centre_text(c,(int)view->instruction_y,font,view->instruction,BLACK);
    int left=(int)(c.width*.08),base=(int)view->legend_y;
    Point swatch[4]={{left,base},{left+26*unit,base},{left+26*unit,base+26*unit},{left,base+26*unit}};
    fill_quad(c,swatch,palette(view->hypotenuse_square_colour));
    text(c,left+(int)(42*unit),base,font,view->equation,BLACK);
    text(c,left+(int)(42*unit),base+font*11,font,view->motion,BLACK);
}
static void draw_static_page(Canvas c,const I47Applet *a) {
    fill_paper(c);
    const I47Presentation *view=&a->presentation;
    double unit=c.width/576.0;
    int font=(int)fmax(1,floor(c.width/250.0)),title=(int)fmax(1,floor(c.width/155.0));
    centre_text(c,(int)view->brand_y,font,view->brand,BLACK);
    centre_text(c,(int)view->title_y,title,view->title,BLACK);
    centre_text(c,(int)view->subtitle_y,font,view->subtitle,BLACK);
    I47AppletRect checks=i47_applet_checks_bounds(a);
    Point tl={checks.left,checks.top},tr={checks.right,checks.top},bl={checks.left,checks.bottom},br={checks.right,checks.bottom};
    stroke(c,tl,tr,1.5*unit,BLACK,false);stroke(c,tr,br,1.5*unit,BLACK,false);
    stroke(c,br,bl,1.5*unit,BLACK,false);stroke(c,bl,tl,1.5*unit,BLACK,false);
    draw_post_dynamic_overlay(c,a,unit,font);
}
static bool ensure_static_page(I47Applet *a) {
    if (a->static_pixels&&a->static_width==a->width&&a->static_height==a->height) return true;
    free(a->static_pixels);
    a->static_pixels=NULL; a->static_width=0; a->static_height=0;
    if (a->width<=0||a->height<=0) return false;
    size_t count=(size_t)a->width*(size_t)a->height;
    if (a->width>0&&count/(size_t)a->width!=(size_t)a->height) return false;
    if (count>SIZE_MAX/sizeof(uint32_t)) return false;
    uint32_t *pixels=malloc(count*sizeof *pixels);
    if (!pixels) return false;
    Canvas cached={pixels,a->width,a->height,a->width};
    draw_static_page(cached,a);
    a->static_pixels=pixels;
    a->static_width=a->width;
    a->static_height=a->height;
    ++a->static_rebuilds;
    return true;
}

void i47_applet_render(I47Applet *a,uint32_t *pixels,int stride) {
    if (!pixels||!a||!a->runtime_ok||a->width<=0||a->height<=0||stride<a->width) return;
    Canvas c={pixels,a->width,a->height,stride};
    if (ensure_static_page(a)) {
        for (int y=0;y<c.height;++y)
            memcpy(c.pixels+y*c.stride,a->static_pixels+(size_t)y*a->width,(size_t)a->width*sizeof *c.pixels);
    } else {
        draw_static_page(c,a);
    }

    const I47Presentation *view=&a->presentation;
    double unit=c.width/576.0,line=fmax(2,4*unit);
    int font=(int)fmax(1,floor(c.width/250.0));
    I47AppletRect checks=i47_applet_checks_bounds(a);
    const char *caption=a->debug?view->checks_on:view->checks_off;
    text(c,(int)((checks.left+checks.right-strlen(caption)*font*6)*.5),(int)((checks.top+checks.bottom-font*7)*.5),font,caption,BLACK);

    const PythagorasConstruction *p=&a->construction;
    if (p->status==GEOMETRY_OK) {
        Point red[4],blue[4],yellow[4];
        screen_square(a,&p->red_square,red);screen_square(a,&p->blue_square,blue);screen_square(a,&p->yellow_square,yellow);
        fill_quad(c,red,palette(view->hypotenuse_square_colour));
        fill_quad(c,blue,palette(view->blue_square_colour));
        fill_quad(c,yellow,palette(view->yellow_square_colour));
        outline_quad(c,red,line*.55,palette(view->proof_colour));
        outline_quad(c,blue,line*.55,palette(view->proof_colour));
        outline_quad(c,yellow,line*.55,palette(view->proof_colour));
        Point right=i47_applet_to_screen(a,p->right_angle),be=i47_applet_to_screen(a,p->blue_end),ye=i47_applet_to_screen(a,p->yellow_end);
        fill_triangle(c,right,be,ye,PAPER);
        stroke(c,right,be,line,palette(view->blue_leg_colour),false);
        stroke(c,right,ye,line,palette(view->yellow_leg_colour),false);
        stroke(c,be,ye,line,palette(view->hypotenuse_colour),false);
        Point far_cut=i47_applet_to_screen(a,p->hypotenuse_far_cut);
        stroke(c,right,far_cut,line*.65,palette(view->proof_colour),false);
        stroke(c,right,red[3],line*.55,palette(view->proof_colour),false);
        stroke(c,right,red[2],line*.55,palette(view->proof_colour),false);
        double marker=14*unit;
        double blue_length=hypot(be.x-right.x,be.y-right.y);
        double yellow_length=hypot(ye.x-right.x,ye.y-right.y);
        Point ub={(be.x-right.x)/blue_length,(be.y-right.y)/blue_length};
        Point uy={(ye.x-right.x)/yellow_length,(ye.y-right.y)/yellow_length};
        Point m1={right.x+ub.x*marker,right.y+ub.y*marker};
        Point m2={m1.x+uy.x*marker,m1.y+uy.y*marker};
        Point m3={right.x+uy.x*marker,right.y+uy.y*marker};
        stroke(c,m1,m2,line*.55,palette(view->proof_colour),false);
        stroke(c,m2,m3,line*.55,palette(view->proof_colour),false);
        disk(c,be,12*unit,PAPER);disk(c,be,8*unit,palette(view->blue_leg_colour));
        disk(c,ye,12*unit,PAPER);disk(c,ye,8*unit,palette(view->yellow_leg_colour));
    }

    /* Preserve the original painter's order if geometry ever reaches this band. */
    draw_post_dynamic_overlay(c,a,unit,font);

    if (a->debug) {
        int left=(int)(c.width*.08),top=(int)view->debug_y; char message[100];
        stroke(c,(Point){left,top-8*unit},(Point){c.width-left,top-8*unit},1,palette(view->proof_colour),false);
        snprintf(message,sizeof message,"AREA %.1E  SQUARE %.1E",p->area_error,p->square_error); text(c,left,top,font,message,BLACK);
        snprintf(message,sizeof message,"RIGHT %.1E  MAX %.1E",p->right_angle_error,a->maximum_area_error); text(c,left,top+font*10,font,message,BLACK);
        snprintf(message,sizeof message,"LEGS %.3f %.3f  UPDATES %u",a->blue_length,a->yellow_length,a->drag_updates); text(c,left,top+font*20,font,message,BLACK);
    }
}
