#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

/* ── Select your RTC here ── */
#define RTC_DS3231   0
#define RTC_DS1307   1
#define RTC_DS1302   2
#define RTC_PCF8563  3

#define RTC_TYPE  RTC_DS1307   /* <-- change this line only */

typedef struct {
    int second, minute, hour;
    int day, month, year;
    int weekday; /* 0=Sunday */
} rtc_datetime_t;

int  ds3231_init(void);
int  ds3231_get_time(rtc_datetime_t *dt);
int  ds3231_set_time(const rtc_datetime_t *dt);

#endif