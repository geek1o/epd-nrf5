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
static uint8_t batt_cal(uint16_t voltage) {
    uint16_t adc_sample = (voltage * 2047) / 3600;
    if (adc_sample > 1705)
        return 100;
    else if (adc_sample <= 1705 && adc_sample > 1584)
        return 28 + (uint8_t)(((((adc_sample - 1584) << 16) / (1705 - 1584)) * 72) >> 16);
    else if (adc_sample <= 1584 && adc_sample > 1360)
        return 4 + (uint8_t)(((((adc_sample - 1360) << 16) / (1584 - 1360)) * 24) >> 16);
    else if (adc_sample <= 1360 && adc_sample > 1136)
        return (uint8_t)(((((adc_sample - 1136) << 16) / (1360 - 1136)) * 4) >> 16);
    else
        return 0;
}

/* Same indicator in calendar and clock; voltage is measured, fill is approximate. */
static void battery(Adafruit_GFX *g,int right,int top,uint16_t voltage) {
    char value[20];
    snprintf(value,sizeof(value),"%u,%u В",voltage/1000,(voltage%1000)/100);
    GFX_setFont(g,u8g2_font_ru11);
    text(g,right-24-GFX_getUTF8Width(g,value),top+9,GFX_BLACK,u8g2_font_ru11,value);
    int bx=right-20;
    GFX_drawRect(g,bx,top,20,10,GFX_BLACK);
    GFX_fillRect(g,bx+20,top+4,2,2,GFX_BLACK);
    GFX_fillRect(g,bx+2,top+2,16*batt_cal(voltage)/100,6,GFX_BLACK);
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
    battery(g,width-12,12,data->voltage);
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
            GFX_fillCircle(g,cx,cy,16,bg);
        } else if (kind==CAL_HOLIDAY) {
            GFX_drawCircle(g,cx,cy,16,accent);
        }
        snprintf(buf,sizeof(buf),"%d",day);
        GFX_setFontMode(g,1);
        centered(g,x,cy+7,cw,fg,bg,buf);
        GFX_setFontMode(g,0);
        if (kind==CAL_HOLIDAY && day==d.day) GFX_fillCircle(g,cx+11,cy-10,1,GFX_WHITE);
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
    unsigned ordinal=cal_day_of_year(d),left=cal_days_left_in_year(d);
    if (cal_transfers_known(d.year))
        snprintf(buf,sizeof(buf),"День %u/%u  До конца года: %u",ordinal,ordinal+left,left);
    else
        snprintf(buf,sizeof(buf),"День %u/%u  Переносы не заданы",ordinal,ordinal+left);
    GFX_setFont(g,u8g2_font_ru11);
#ifdef CALENDAR_LAYOUT_CHECK
    assert(width-10-GFX_getUTF8Width(g,buf)>100);
#endif
    text(g,width-10-GFX_getUTF8Width(g,buf),height-5,GFX_BLACK,u8g2_font_ru11,buf);
}
// clang-format off
/* Routine to Draw Large 7-Segment formated number
   Contributed by William Zaggle.

   int n - The number to be displayed
   int xLoc = The x location of the upper left corner of the number
   int yLoc = The y location of the upper left corner of the number
   int cS = The size of the number.
   fC is the foreground color of the number
   bC is the background color of the number (prevents having to clear previous space)
   nD is the number of digit spaces to occupy (must include space for minus sign for numbers < 0).

   width: nD*(11*cS+2)-2*cS
   height: 20*cS+4

   https://forum.arduino.cc/t/fast-7-segment-number-display-for-tft/296619/4
*/
static void Draw7Number(Adafruit_GFX *gfx, int16_t n, uint16_t xLoc, uint16_t yLoc, int16_t cS, uint16_t fC, uint16_t bC, int16_t nD) {
    uint16_t num=abs(n),i,t,w,col,h,a,b,j=1,d=0,S2=5*cS,S3=2*cS,S4=7*cS,x1=cS+1,x2=S3+S2+1,y1=yLoc+x1,y3=yLoc+S3+S4+1;
    uint16_t seg[7][3]={{x1,yLoc,1},{x2,y1,0},{x2,y3+x1,0},{x1,(2*y3)-yLoc,1},{0,y3+x1,0},{0,y1,0},{x1,y3,1}};
    uint8_t nums[12]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x00,0x40},c=(c=abs(cS))>10?10:(c<1)?1:c,cnt=(cnt=abs(nD))>10?10:(cnt<1)?1:cnt;
    for (xLoc+=cnt*(d=S2+(3*S3)+2);cnt>0;cnt--){
      for (i=(num>9)?num%10:((!cnt)&&(n<0))?11:((nD<0)&&(!num))?10:num,xLoc-=d,num/=10,j=0;j<7;++j){
        col=(nums[i]&(1<<j))?fC:bC;
        if (seg[j][2])for(w=S2,t=seg[j][1]+S3,h=seg[j][1]+cS,a=xLoc+seg[j][0]+cS,b=seg[j][1];b<h;b++,a--,w+=2)GFX_drawFastHLine(gfx,a,b,w,col);
        else for(w=S4,t=xLoc+seg[j][0]+S3,h=xLoc+seg[j][0]+cS,b=xLoc+seg[j][0],a=seg[j][1]+cS;b<h;b++,a--,w+=2)GFX_drawFastVLine(gfx,b,a,w,col);
        for (;b<t;b++,a++,w-=2)seg[j][2]?GFX_drawFastHLine(gfx,a,b,w,col):GFX_drawFastVLine(gfx,b,a,w,col);
        }
    }
}
// clang-format on

/* Preserve upstream's segment geometry, scale and square colon from the reference. */
static void clock_time(Adafruit_GFX *g,cal_date_t d,int x,int y,int scale) {
    Draw7Number(g,d.hour,x,y,scale,GFX_BLACK,GFX_WHITE,2);
    x += 2*(11*scale+2)-2*scale+2*scale;
    GFX_fillRect(g,x,y+9*scale/2+1,2*scale,2*scale,GFX_BLACK);
    GFX_fillRect(g,x,y+27*scale/2+3,2*scale,2*scale,GFX_BLACK);
    x += 4*scale;
    Draw7Number(g,d.minute,x,y,scale,GFX_BLACK,GFX_WHITE,2);
}
static void clock_view(Adafruit_GFX *g,const gui_data_t *data,cal_date_t d) {
    static const char *const months[]={"января","февраля","марта","апреля","мая","июня","июля","августа","сентября","октября","ноября","декабря"};
    uint16_t accent=data->color>=2 ? GFX_RED : GFX_BLACK;
    int width=data->width,height=data->height,padding=height>=400?100:40;
    char s[80];
    snprintf(s,sizeof(s),"%02u",d.day);
    text(g,padding,36,accent,u8g2_font_helvB18_tn,s);
    int date_x=g->tx;
    snprintf(s,sizeof(s)," %s ",months[d.month-1]);
    text(g,date_x,36,GFX_BLACK,u8g2_font_ru16,s);
    date_x=g->tx;
    snprintf(s,sizeof(s),"%u",d.year);
    text(g,date_x,36,accent,u8g2_font_helvB18_tn,s);
#ifdef CALENDAR_LAYOUT_CHECK
    assert(g->tx<width-padding-70);
#endif
    text(g,padding,58,GFX_BLACK,u8g2_font_ru13,cal_week_names[cal_weekday(d.year,d.month,d.day)]);
    battery(g,width-padding,25,data->voltage);
    snprintf(s,sizeof(s),"%d°C",data->temperature);
    GFX_setFont(g,u8g2_font_ru13);
    text(g,width-padding-GFX_getUTF8Width(g,s),58,GFX_BLACK,u8g2_font_ru13,s);
    GFX_drawFastHLine(g,padding-10,68,width-2*(padding-10),GFX_BLACK);
    int scale=height/45;
    if (scale>10) scale=10;
    int time_width=2*(2*(11*scale+2)-2*scale)+4*scale;
    int time_height=20*scale+4;
    clock_time(g,d,(width-time_width)/2,(height-time_height)/2,scale);
    GFX_drawFastHLine(g,padding-10,height-68,width-2*(padding-10),GFX_BLACK);
    snprintf(s,sizeof(s),"%u год",d.year);
    text(g,padding,height-38,GFX_BLACK,u8g2_font_ru13,s);
    snprintf(s,sizeof(s),"%u-я неделя",cal_iso_week(d,NULL));
    text(g,padding,height-18,GFX_BLACK,u8g2_font_ru13,s);
    cal_date_t next,today=d;today.hour=today.minute=today.second=0;
    if (cal_next_holiday(d,&next)) {
        const char *name=holiday_label(next);
        GFX_setFont(g,u8g2_font_ru13);
        text(g,width-padding-GFX_getUTF8Width(g,name),height-38,accent,u8g2_font_ru13,name);
        unsigned days=(cal_to_timestamp(next)-cal_to_timestamp(today))/86400;
        unsigned last=days%10, last_two=days%100;
        const char *unit=last==1 && last_two!=11?"день":last>=2 && last<=4 && (last_two<12 || last_two>14)?"дня":"дней";
        if (!days) snprintf(s,sizeof(s),"Сегодня");
        else snprintf(s,sizeof(s),"%s %u %s",last==1 && last_two!=11?"Остался":"Осталось",days,unit);
        text(g,width-padding-GFX_getUTF8Width(g,s),height-18,GFX_BLACK,u8g2_font_ru13,s);
    }
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
