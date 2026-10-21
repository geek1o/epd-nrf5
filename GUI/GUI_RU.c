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
    if (GFX_getUTF8Width(g,s)>w) fprintf(stderr,"Text too wide: %s (%d > %d)\n",s,GFX_getUTF8Width(g,s),w);
    assert(GFX_getUTF8Width(g,s)<=w);
#endif
    GFX_setTextColor(g,fg,bg);
    GFX_setCursor(g,x+(w-GFX_getUTF8Width(g,s))/2,y); GFX_printf(g,"%s",s);
}
static const char *holiday_label(cal_date_t d) {
    switch (d.month) {
        case 1: return d.day==7 ? "Рождество" : "Новый год";
        case 2: return "День защитника";
        case 3: return "8 Марта";
        case 5: return d.day==9 ? "День Победы" : "Праздник труда";
        case 6: return "День России";
        default: return "День единства";
    }
}
static void calendar(Adafruit_GFX *g, const gui_data_t *data, cal_date_t d) {
    const uint16_t accent=data->color>=2 ? GFX_RED : GFX_BLACK;
    const int width=data->width, height=data->height;
    const int divider=width*2/3+6, grid_width=divider-20;
    const int side_x=divider+12, side_width=width-side_x-10;
    const int grid_top=65, bottom=height-27;
    char buf[80];
    snprintf(buf,sizeof(buf),"%s %u",cal_month_names[d.month-1],d.year);
    text(g,10,22,GFX_BLACK,u8g2_font_ru16,buf);
    snprintf(buf,sizeof(buf),"%u,%u В",data->voltage/1000,(data->voltage%1000)/100);
    GFX_setFont(g,u8g2_font_ru11);
    text(g,width-10-GFX_getUTF8Width(g,buf),21,GFX_BLACK,u8g2_font_ru11,buf);
    GFX_drawFastHLine(g,10,31,width-20,GFX_BLACK);
    GFX_drawFastVLine(g,divider,41,bottom-41,GFX_BLACK);
    static const char *const labels[]={"Пн","Вт","Ср","Чт","Пт","Сб","Вс"};
    GFX_setFont(g,u8g2_font_ru13);
    for (int col=0;col<7;col++) {
        int x=10+grid_width*col/7, cw=10+grid_width*(col+1)/7-x;
        centered(g,x,51,cw,col>=5?accent:GFX_BLACK,GFX_WHITE,labels[col]);
    }
    GFX_drawFastHLine(g,10,59,grid_width,GFX_BLACK);
    int first=cal_weekday(d.year,d.month,1), days=cal_month_days(d.year,d.month);
    int rows=(first+days+6)/7;
    GFX_setFont(g,u8g2_font_ru_numbers20);
    for (int day=1;day<=days;day++) {
        int cell=first+day-1, col=cell%7, row=cell/7;
        int x=10+grid_width*col/7, cw=10+grid_width*(col+1)/7-x;
        int y=grid_top+(bottom-grid_top)*row/rows;
        int ch=grid_top+(bottom-grid_top)*(row+1)/rows-y;
        int cx=x+cw/2, cy=y+ch/2;
        cal_day_kind_t kind=cal_day_kind(d.year,d.month,day);
        uint16_t fg=kind==CAL_WORKDAY?GFX_BLACK:accent, bg=GFX_WHITE;
        if (day==d.day) {
            bg=accent; fg=GFX_WHITE;
            GFX_fillRoundRect(g,cx-16,cy-14,32,29,4,bg);
        }
        snprintf(buf,sizeof(buf),"%d",day);
        centered(g,x,cy+7,cw,fg,bg,buf);
        if (kind==CAL_HOLIDAY) GFX_fillCircle(g,day==d.day?cx+12:x+cw-4,cy-11,1,fg);
        if (kind==CAL_TRANSFER_OFF) GFX_drawFastHLine(g,cx-7,cy+12,14,fg);
    }
    GFX_setFont(g,u8g2_font_ru11);
    centered(g,side_x,48,side_width,GFX_BLACK,GFX_WHITE,"СЕГОДНЯ");
    snprintf(buf,sizeof(buf),"%u",d.day);
    GFX_setFont(g,u8g2_font_ru_numbers48);
    centered(g,side_x,102,side_width,accent,GFX_WHITE,buf);
    GFX_setFont(g,u8g2_font_ru13);
    centered(g,side_x,124,side_width,GFX_BLACK,GFX_WHITE,cal_week_names[cal_weekday(d.year,d.month,d.day)]);
    GFX_drawFastHLine(g,side_x,136,side_width,GFX_BLACK);
    GFX_setFont(g,u8g2_font_ru11);
    centered(g,side_x,154,side_width,GFX_BLACK,GFX_WHITE,"БЛИЖАЙШИЕ");
    cal_date_t cursor=d, next, today=d;
    today.hour=today.minute=today.second=0;
    for (int i=0;i<3;i++) {
        if (!cal_next_holiday(cursor,&next)) break;
        int y=177+i*(bottom-157)/3;
        snprintf(buf,sizeof(buf),"%02u.%02u",next.day,next.month);
        text(g,side_x,y,GFX_BLACK,u8g2_font_ru13,buf);
        unsigned remaining=(cal_to_timestamp(next)-cal_to_timestamp(today))/86400;
        snprintf(buf,sizeof(buf),remaining ? "%u дн." : "сегодня",remaining);
        GFX_setFont(g,u8g2_font_ru11);
        text(g,width-10-GFX_getUTF8Width(g,buf),y,accent,u8g2_font_ru11,buf);
        text(g,side_x,y+16,GFX_BLACK,u8g2_font_ru11,holiday_label(next));
        cursor=cal_from_timestamp(cal_to_timestamp(next)+86400);
    }
    GFX_drawFastHLine(g,10,height-20,width-20,GFX_BLACK);
    uint16_t wy; unsigned week=cal_iso_week(d,&wy);
    snprintf(buf,sizeof(buf),"НЕДЕЛЯ %02u",week);
    text(g,10,height-5,GFX_BLACK,u8g2_font_ru11,buf);
    const char *note=cal_transfers_known(d.year) ? "Точка: праздник / черта: перенос" : "Переносы на этот год не заданы";
    GFX_setFont(g,u8g2_font_ru11);
    text(g,width-10-GFX_getUTF8Width(g,note),height-5,GFX_BLACK,u8g2_font_ru11,note);
}
static void clock_view(Adafruit_GFX *g,const gui_data_t *data,cal_date_t d) {
    char s[80];
    snprintf(s,sizeof(s),"%02u.%02u.%u  %s",d.day,d.month,d.year,cal_week_names[cal_weekday(d.year,d.month,d.day)]);
    text(g,10,30,GFX_BLACK,u8g2_font_ru16,s);
    snprintf(s,sizeof(s),"%02u:%02u",d.hour,d.minute);
    GFX_setFont(g,u8g2_font_ru_numbers48);
    centered(g,0,data->height/2,data->width,GFX_BLACK,GFX_WHITE,s);
    text(g,10,data->height-15,GFX_BLACK,u8g2_font_ru13,"Обновление раз в минуту");
}
void DrawGUI(gui_data_t *data,buffer_callback callback,void *user) {
    if (!data || data->width<400 || data->height<300) return;
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
            text(&g,25,138,GFX_BLACK,u8g2_font_ru16,"Синхронизируйте время");
            text(&g,25,162,GFX_BLACK,u8g2_font_ru13,"Подключите Bluetooth в браузере");
        }
    } while (GFX_nextPage(&g,callback,user));
    GFX_end(&g);
}
