#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>
#include <stdbool.h>

uint32_t timestamp(void);
bool time_is_synced(void);
void set_timestamp(uint32_t timestamp);
void sleep_mode_enter(void);
void app_feed_wdt(void);

#endif
