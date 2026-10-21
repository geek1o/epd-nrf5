#include "calendar.h"
#include <stddef.h>
const char *const cal_month_names[12] = {"ЯНВАРЬ", "ФЕВРАЛЬ", "МАРТ", "АПРЕЛЬ", "МАЙ", "ИЮНЬ", "ИЮЛЬ", "АВГУСТ", "СЕНТЯБРЬ", "ОКТЯБРЬ", "НОЯБРЬ", "ДЕКАБРЬ"};
const char *const cal_week_names[7] = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};
static bool leap(uint16_t y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
uint8_t cal_month_days(uint16_t y, uint8_t m) {
    static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return m >= 1 && m <= 12 ? days[m-1] + (m == 2 && leap(y)) : 0;
}
/* Days from 1970, supported civil range 1970..2106 (32-bit protocol). */
static uint32_t days_before(uint16_t y, uint8_t m, uint8_t d) {
    uint32_t n = d-1;
    for (uint16_t i=1970; i<y; ++i) n += leap(i) ? 366 : 365;
    for (uint8_t i=1; i<m; ++i) n += cal_month_days(y,i);
    return n;
}
uint8_t cal_weekday(uint16_t y, uint8_t m, uint8_t d) { return (days_before(y,m,d)+3)%7; }
cal_date_t cal_from_timestamp(uint32_t t) {
    cal_date_t d = {1970,1,1,0,0,0};
    uint32_t n=t/86400;
    d.hour=t%86400/3600; d.minute=t%3600/60; d.second=t%60;
    while (n >= (uint32_t)(leap(d.year)?366:365)) n -= leap(d.year++)?366:365;
    while (n >= cal_month_days(d.year,d.month)) n -= cal_month_days(d.year,d.month++);
    d.day=n+1; return d;
}
uint32_t cal_to_timestamp(cal_date_t d) { return days_before(d.year,d.month,d.day)*86400 + d.hour*3600u + d.minute*60u + d.second; }
static uint8_t weeks(uint16_t y) { uint8_t w=cal_weekday(y,1,1); return w==3 || (w==2 && leap(y)) ? 53 : 52; }
uint8_t cal_iso_week(cal_date_t d, uint16_t *wy) {
    int doy=days_before(d.year,d.month,d.day)-days_before(d.year,1,1)+1;
    int w=(doy-(cal_weekday(d.year,d.month,d.day)+1)+10)/7;
    uint16_t y=d.year;
    if (w<1) { --y; w=weeks(y); } else if (w>weeks(y)) { ++y; w=1; }
    if (wy) *wy=y;
    return w;
}
const char *cal_holiday(uint8_t m, uint8_t d) {
    if (m==1 && d>=1 && d<=8) return d==7 ? "Рождество" : "Новогодние каникулы";
    if (m==2 && d==23) return "День защитника Отечества";
    if (m==3 && d==8) return "Международный женский день";
    if (m==5 && d==1) return "Праздник Весны и Труда";
    if (m==5 && d==9) return "День Победы";
    if (m==6 && d==12) return "День России";
    if (m==11 && d==4) return "День народного единства";
    return NULL;
}
/* Verified decrees and Article 112; see docs/holidays_ru.md. Five-day workweek. */
typedef struct { uint16_t year, md; cal_day_kind_t kind; } day_override_t;
static const day_override_t overrides[] = {
    {2025,0x0502,CAL_TRANSFER_OFF}, {2025,0x0508,CAL_TRANSFER_OFF},
    {2025,0x060d,CAL_TRANSFER_OFF}, {2025,0x0b03,CAL_TRANSFER_OFF},
    {2025,0x0c1f,CAL_TRANSFER_OFF}, {2025,0x0b01,CAL_WORKDAY},
    {2026,0x0109,CAL_TRANSFER_OFF}, {2026,0x0309,CAL_TRANSFER_OFF},
    {2026,0x050b,CAL_TRANSFER_OFF}, {2026,0x0c1f,CAL_TRANSFER_OFF},
    {2027,0x0214,CAL_WORKDAY}, {2027,0x0216,CAL_TRANSFER_OFF},
    {2027,0x0503,CAL_TRANSFER_OFF}, {2027,0x050a,CAL_TRANSFER_OFF},
    {2027,0x060e,CAL_TRANSFER_OFF}, {2027,0x0b05,CAL_TRANSFER_OFF},
    {2027,0x0c1f,CAL_TRANSFER_OFF},
};
bool cal_transfers_known(uint16_t y) { return y>=2025 && y<=2027; }
cal_day_kind_t cal_day_kind(uint16_t y, uint8_t m, uint8_t d) {
    if (cal_holiday(m,d)) return CAL_HOLIDAY;
    uint16_t md=((uint16_t)m<<8)|d;
    for (unsigned i=0;i<sizeof(overrides)/sizeof(overrides[0]);i++)
        if (overrides[i].year==y && overrides[i].md==md) return overrides[i].kind;
    return cal_weekday(y,m,d)>=5 ? CAL_WEEKEND : CAL_WORKDAY;
}
const char *cal_next_holiday(cal_date_t d, cal_date_t *next) {
    d.hour=d.minute=d.second=0;
    for (unsigned i=0;i<367;i++) {
        const char *s=cal_holiday(d.month,d.day);
        if (s) { if (next) *next=d; return s; }
        if (++d.day>cal_month_days(d.year,d.month)) { d.day=1; if (++d.month>12) { d.month=1; ++d.year; } }
    }
    return NULL;
}
