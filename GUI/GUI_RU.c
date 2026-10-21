/* Russian renderer. Hardware drivers and BLE packet layout remain upstream-compatible. */
#include "GUI.h"
#include "calendar.h"
#include "fonts_ru.h"
#include <stdio.h>
#include <string.h>
#ifdef CALENDAR_LAYOUT_CHECK
#include <assert.h>
#endif

static void text(Adafruit_GFX *g, int x, int y, uint16_t fg, const uint8_t *font, const char *s) {
    GFX_setFont(g,font);
#ifdef CALENDAR_LAYOUT_CHECK
    assert(x>=0 && x+GFX_getUTF8Width(g,s)<=g->_width);
    assert(y-GFX_getFontAscent(g)>=0 && y-GFX_getFontDescent(g)<=g->_height);
#endif
    GFX_setTextColor(g,fg,GFX_WHITE); GFX_setCursor(g,x,y); GFX_printf(g,"%s",s);
}
static void centered(Adafruit_GFX *g, int x, int y, int w, uint16_t fg, uint16_t bg, const char *s) {
#ifdef CALENDAR_LAYOUT_CHECK
    assert(GFX_getUTF8Width(g,s)<=w);
#endif
    GFX_setTextColor(g,fg,bg);
    GFX_setCursor(g,x+(w-GFX_getUTF8Width(g,s))/2,y); GFX_printf(g,"%s",s);
}
static void calendar(Adafruit_GFX *g, const gui_data_t *data, cal_date_t d) {
    const uint16_t accent=data->color>=2 ? GFX_RED : GFX_BLACK;
    const int width=data->width, height=data->height, margin=10;
    char buf[96];
    snprintf(buf,sizeof(buf),"%s %u",cal_month_names[d.month-1],d.year);
    text(g,margin,24,GFX_BLACK,u8g2_font_ru12,buf);
    snprintf(buf,sizeof(buf),"%u",d.day);
    text(g,margin,56,accent,u8g2_font_helvB18_tn,buf);
    text(g,52,53,GFX_BLACK,u8g2_font_ru12,cal_week_names[cal_weekday(d.year,d.month,d.day)]);
    snprintf(buf,sizeof(buf),"%u.%uV",data->voltage/1000,(data->voltage%1000)/100);
    GFX_setFont(g,u8g2_font_ru9);
    text(g,width-margin-GFX_getUTF8Width(g,buf),53,GFX_BLACK,u8g2_font_ru9,buf);
    const int top=68, grid=92, bottom=height-39;
    static const char *const labels[]={"Пн","Вт","Ср","Чт","Пт","Сб","Вс"};
    GFX_setFont(g,u8g2_font_ru9);
    for (int c=0;c<7;c++) {
        int x=margin+(width-2*margin)*c/7;
        int cw=margin+(width-2*margin)*(c+1)/7-x;
        GFX_fillRect(g,x,top,cw,21,c>=5?accent:GFX_BLACK);
        centered(g,x,top+15,cw,GFX_WHITE,c>=5?accent:GFX_BLACK,labels[c]);
    }
    int first=cal_weekday(d.year,d.month,1);
    int days=cal_month_days(d.year,d.month);
    int rows=(first+days+6)/7;
    GFX_setFont(g,u8g2_font_helvB14_tn);
    for (int day=1;day<=days;day++) {
        int cell=first+day-1, col=cell%7, row=cell/7;
        int x=margin+(width-2*margin)*col/7;
        int cw=margin+(width-2*margin)*(col+1)/7-x;
        int y=grid+(bottom-grid)*row/rows;
        int ch=grid+(bottom-grid)*(row+1)/rows-y;
        cal_day_kind_t kind=cal_day_kind(d.year,d.month,day);
        uint16_t fg=kind==CAL_WORKDAY?GFX_BLACK:accent;
        uint16_t bg=GFX_WHITE;
        if (day==d.day) {
            bg=accent; fg=GFX_WHITE;
            GFX_fillRoundRect(g,x+5,y+2,cw-10,ch-4,5,bg);
        }
        snprintf(buf,sizeof(buf),"%d",day);
        centered(g,x,y+(ch+GFX_getFontAscent(g))/2-1,cw,fg,bg,buf);
        /* Dot = statutory holiday; underline = transferred day off, including B/W. */
        if (kind==CAL_HOLIDAY) GFX_fillCircle(g,x+cw-9,y+7,2,fg);
        if (kind==CAL_TRANSFER_OFF) GFX_drawFastHLine(g,x+cw/2-7,y+ch-5,14,fg);
    }
    GFX_drawFastHLine(g,margin,bottom+2,width-2*margin,GFX_BLACK);
    uint16_t wy;
    unsigned week=cal_iso_week(d,&wy);
    snprintf(buf,sizeof(buf),"Неделя %u / %u%s",week,wy,cal_transfers_known(d.year)?"":"  * переносы не заданы");
    text(g,margin,height-22,GFX_BLACK,u8g2_font_ru9,buf);
    cal_date_t next;
    const char *name=cal_next_holiday(d,&next);
    if (name) {
        snprintf(buf,sizeof(buf),"%02u.%02u  %s",next.day,next.month,name);
        if (GFX_getUTF8Width(g,buf)<=width-2*margin)
            text(g,margin,height-6,accent,u8g2_font_ru9,buf);
    }
}
static void clock_view(Adafruit_GFX *g,const gui_data_t *data,cal_date_t d) {
    char s[80];
    snprintf(s,sizeof(s),"%02u.%02u.%u  %s",d.day,d.month,d.year,cal_week_names[cal_weekday(d.year,d.month,d.day)]);
    text(g,10,30,GFX_BLACK,u8g2_font_ru12,s);
    snprintf(s,sizeof(s),"%02u:%02u",d.hour,d.minute);
    GFX_setFont(g,u8g2_font_helvB18_tn);
    centered(g,0,data->height/2,data->width,GFX_BLACK,GFX_WHITE,s);
    text(g,10,data->height-15,GFX_BLACK,u8g2_font_ru9,"Обновление раз в минуту");
}
void DrawGUI(gui_data_t *data,buffer_callback callback,void *user) {
    if (!data || data->width<200 || data->height<160) return;
    cal_date_t d=cal_from_timestamp(data->timestamp);
    Adafruit_GFX g;
    int ph=(__HEAP_SIZE-512)/((data->width+7)/8);
    if (ph<2) return;
    if (data->color==2) GFX_begin_3c(&g,data->width,data->height,ph);
    else if (data->color==3) GFX_begin_4c(&g,data->width,data->height,ph);
    else GFX_begin(&g,data->width,data->height,ph);
    if (!g.buffer) return;
    GFX_firstPage(&g);
    do {
        GFX_fillScreen(&g,GFX_WHITE);
        if (data->time_valid && data->mode==MODE_CALENDAR) calendar(&g,data,d);
        else if (data->time_valid && data->mode==MODE_CLOCK) clock_view(&g,data,d);
        /* A reset loses RTC civil time; never present a guessed date as current. */
        if (!data->time_valid) {
            GFX_fillRect(&g,15,110,data->width-30,70,GFX_WHITE);
            GFX_drawRect(&g,15,110,data->width-30,70,GFX_BLACK);
            text(&g,25,138,GFX_BLACK,u8g2_font_ru12,"Синхронизируйте время");
            text(&g,25,162,GFX_BLACK,u8g2_font_ru9,"Подключите Bluetooth в браузере");
        }
    } while (GFX_nextPage(&g,callback,user));
    GFX_end(&g);
}
