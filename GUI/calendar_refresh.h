#ifndef EPD_CALENDAR_REFRESH_H
#define EPD_CALENDAR_REFRESH_H
#include "GUI.h"
/* A successfully enqueued period, not the last timer tick. Queue-full retries
 * on the next second instead of losing the midnight refresh for 24 hours. */
typedef struct { uint32_t slot; display_mode_t mode; bool accepted; } cal_refresh_state_t;
static inline uint32_t cal_refresh_slot(display_mode_t mode, uint32_t timestamp) {
    return timestamp/(mode==MODE_CLOCK ? 60u : 86400u);
}
static inline bool cal_refresh_due(const cal_refresh_state_t *state, display_mode_t mode,
                                  uint32_t timestamp, bool synced, bool force) {
    if (force) return true;
    if (!synced || (mode!=MODE_CALENDAR && mode!=MODE_CLOCK)) return false;
    return !state->accepted || state->mode!=mode || state->slot!=cal_refresh_slot(mode,timestamp);
}
static inline void cal_refresh_accepted(cal_refresh_state_t *state, display_mode_t mode, uint32_t timestamp) {
    state->slot=cal_refresh_slot(mode,timestamp); state->mode=mode; state->accepted=true;
}
#endif
