#ifndef EPD_CALENDAR_H
#define EPD_CALENDAR_H
#include <stdint.h>
#include <stdbool.h>
/* Civil local time; no libc timezone, DST, allocation or lunar tables. */
typedef struct { uint16_t year; uint8_t month, day, hour, minute, second; } cal_date_t;
typedef enum { CAL_WORKDAY, CAL_WEEKEND, CAL_HOLIDAY, CAL_TRANSFER_OFF } cal_day_kind_t;
uint8_t cal_month_days(uint16_t year, uint8_t month);
uint8_t cal_weekday(uint16_t year, uint8_t month, uint8_t day); /* Monday = 0 */
cal_date_t cal_from_timestamp(uint32_t local_seconds);
uint32_t cal_to_timestamp(cal_date_t date);
uint8_t cal_iso_week(cal_date_t date, uint16_t *week_year);
const char *cal_holiday(uint8_t month, uint8_t day);
bool cal_transfers_known(uint16_t year);
cal_day_kind_t cal_day_kind(uint16_t year, uint8_t month, uint8_t day);
const char *cal_next_holiday(cal_date_t date, cal_date_t *next);
extern const char *const cal_month_names[12];
extern const char *const cal_week_names[7];
#endif
