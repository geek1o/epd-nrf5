#include "GUI.h"
#include "calendar.h"
#include "fonts_ru.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
static unsigned char pixels[880*528*3];
static int width,height,color,pages;
static void page(void *ctx,uint8_t *black,uint8_t *red,uint16_t x,uint16_t y,uint16_t w,uint16_t h) {
    (void)ctx; assert(x==0 && w==width && y+h<=height); pages++;
    for (int row=0;row<h;row++) for (int col=0;col<w;col++) {
        int c=1;
        if (color==3) c=(black[row*(w/4)+col/4]>>(6-2*(col%4)))&3;
        else if (!(black[row*(w/8)+col/8]&(128>>(col%8)))) c=0;
        else if (red && !(red[row*(w/8)+col/8]&(128>>(col%8)))) c=3;
        unsigned char *p=&pixels[((row+y)*width+col)*3];
        p[0]=c ? 255:0; p[1]=c==1 || c==2 ? 255:0; p[2]=c==1 ? 255:0;
    }
}
int main(int argc,char **argv) {
    assert(argc>=4);
    color=atoi(argv[2]);width=argc>4?atoi(argv[4]):400;height=argc>5?atoi(argv[5]):300;
    assert(width<=880 && height<=528);
    gui_data_t d={.mode=MODE_CALENDAR,.width=width,.height=height,.color=color,.timestamp=strtoul(argv[1],0,10),.time_valid=true,.voltage=2900,.temperature=27};
    if (argc>6) d.time_valid=atoi(argv[6]);
    if (argc>7) d.mode=atoi(argv[7]);
    DrawGUI(&d,page,NULL); assert(pages>0);
    cal_date_t date=cal_from_timestamp(d.timestamp);
    if (d.mode==MODE_CALENDAR && width==400 && height==300 && d.time_valid &&
        date.year==2026 && date.month==11 && date.day==3) {
        /* Nov 2026 has six rows: today is filled, Nov 4 has only an outline. */
        const int marked[][2]={{52,109},{100,100}};
        for (unsigned i=0;i<2;i++) {
            unsigned char *p=&pixels[(marked[i][1]*width+marked[i][0])*3];
            assert(p[0]==(color>=2?255:0) && p[1]==0 && p[2]==0);
        }
        unsigned char *inside=&pixels[(109*width+88)*3];
        assert(inside[0]==255 && inside[1]==255 && inside[2]==255);
    }
    if (d.mode==MODE_CLOCK && width==400 && height==300 && d.time_valid) {
        /* The reference's time area is black, with two solid 12x12 squares. */
        for (int yy=69;yy<231;yy++) for (int xx=30;xx<370;xx++) {
            unsigned char *p=&pixels[(yy*width+xx)*3];
            assert(!(p[0]==255 && p[1]==0 && p[2]==0));
        }
        for (int square=0;square<2;square++) for (int yy=0;yy<12;yy++) for (int xx=0;xx<12;xx++) {
            unsigned char *p=&pixels[((116+square*56+yy)*width+200+xx)*3];
            assert(p[0]==0 && p[1]==0 && p[2]==0);
        }
    }
    FILE *f=fopen(argv[3],"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);fwrite(pixels,3,width*height,f);fclose(f);
    /* The actual decoder must locate every Russian glyph in both fonts. */
    Adafruit_GFX g={0};
    const uint8_t *fonts[]={u8g2_font_ru11,u8g2_font_ru13,u8g2_font_ru16};
    for (int sz=0;sz<3;sz++) {
        GFX_setFont(&g,fonts[sz]);
        for (unsigned cp=0x410;cp<=0x44f;cp++) assert(u8g2_GetGlyphWidth(&g.u8g2,cp)>0);
        assert(u8g2_GetGlyphWidth(&g.u8g2,0x401)>0 && u8g2_GetGlyphWidth(&g.u8g2,0x451)>0);
        int advance=u8g2_GetGlyphWidth(&g.u8g2,0x41f);
        u8g2_GetGlyphWidth(&g.u8g2,0x43d);
        int expected=advance+g.u8g2.font_decode.glyph_width+g.u8g2.glyph_x_offset;
        assert(GFX_getUTF8Width(&g,"Пн")==expected);
    }
    GFX_setFont(&g,u8g2_font_helvB18_tn);
    for (int cp=48;cp<=57;cp++) assert(u8g2_GetGlyphWidth(&g.u8g2,cp)>0);
    return 0;
}
