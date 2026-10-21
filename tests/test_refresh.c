#include "calendar_refresh.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    cal_refresh_state_t s={0};
    assert(!cal_refresh_due(&s,MODE_CALENDAR,86400,false,false));
    assert(cal_refresh_due(&s,MODE_CALENDAR,86399,true,true));
    cal_refresh_accepted(&s,MODE_CALENDAR,86399);
    assert(!cal_refresh_due(&s,MODE_CALENDAR,86399,true,false));
    /* A full scheduler queue must not consume midnight. */
    assert(cal_refresh_due(&s,MODE_CALENDAR,86400,true,false));
    assert(cal_refresh_due(&s,MODE_CALENDAR,86401,true,false));
    cal_refresh_accepted(&s,MODE_CALENDAR,86401);
    for (uint32_t t=86402;t<172800;t++) assert(!cal_refresh_due(&s,MODE_CALENDAR,t,true,false));
    assert(cal_refresh_due(&s,MODE_CALENDAR,172800,true,false));
    assert(!cal_refresh_due(&s,MODE_PICTURE,172800,true,false));
    assert(cal_refresh_due(&s,MODE_CLOCK,86401,true,false));
    cal_refresh_accepted(&s,MODE_CLOCK,86401);
    assert(!cal_refresh_due(&s,MODE_CLOCK,86459,true,false));
    assert(cal_refresh_due(&s,MODE_CLOCK,86460,true,false));
    puts("Refresh: midnight, full queue retry, no repeated daily refresh, clock and picture modes passed");
}
