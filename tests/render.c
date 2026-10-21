#include "GUI.h"
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
    gui_data_t d={.mode=MODE_CALENDAR,.width=width,.height=height,.color=color,.timestamp=strtoul(argv[1],0,10),.time_valid=true,.voltage=3000};
    if (argc>6) d.time_valid=atoi(argv[6]);
    if (argc>7) d.mode=atoi(argv[7]);
    DrawGUI(&d,page,NULL); assert(pages>0);
    FILE *f=fopen(argv[3],"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);fwrite(pixels,3,width*height,f);fclose(f);
    /* The actual decoder must locate every Russian glyph in both fonts. */
    Adafruit_GFX g={0};
    for (int sz=0;sz<2;sz++) {
        GFX_setFont(&g,sz?u8g2_font_ru12:u8g2_font_ru9);
        for (unsigned cp=0x410;cp<=0x44f;cp++) assert(u8g2_GetGlyphWidth(&g.u8g2,cp)>0);
        assert(u8g2_GetGlyphWidth(&g.u8g2,0x401)>0 && u8g2_GetGlyphWidth(&g.u8g2,0x451)>0);
        int advance=u8g2_GetGlyphWidth(&g.u8g2,0x41f);
        u8g2_GetGlyphWidth(&g.u8g2,0x43d);
        int expected=advance+g.u8g2.font_decode.glyph_width+g.u8g2.glyph_x_offset;
        assert(GFX_getUTF8Width(&g,"Пн")==expected);
    }
    return 0;
}
