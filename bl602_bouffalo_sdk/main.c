#include "bflb_mtimer.h"
#include "board.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "encoder.h"
#include "max7219.h"
#include "ds3231.h"

/* ── Display/setting state — mirrors original exactly ── */
typedef enum { MODE_NORMAL = 0, MODE_TIME_SETTING, MODE_DATE_SETTING } display_mode_t;
typedef enum { TIME_HOUR = 0, TIME_MINUTE, TIME_SECOND }              time_sub_t;
typedef enum { DATE_DAY = 0, DATE_MONTH, DATE_YEAR }                  date_sub_t;

static display_mode_t currentMode  = MODE_NORMAL;
static time_sub_t     timeSubMode  = TIME_HOUR;
static date_sub_t     dateSubMode  = DATE_DAY;

static int tempHour, tempMinute, tempSecond;
static int tempDay, tempMonth, tempYear;

static const int BRIGHTNESS = 6;

/* ── Helpers ── */
static void load_current_time(void)
{
    rtc_datetime_t dt;
    if (ds3231_get_time(&dt) == 0) {
        tempHour   = dt.hour;
        tempMinute = dt.minute;
        tempSecond = dt.second;
        tempDay    = dt.day;
        tempMonth  = dt.month;
        tempYear   = dt.year;
    } else {
        tempHour = 12; tempMinute = 0; tempSecond = 0;
        tempDay  = 1;  tempMonth  = 1; tempYear   = 2024;
    }
}

static void save_settings(void)
{
    rtc_datetime_t dt = {
        .hour    = tempHour,   .minute = tempMinute, .second = tempSecond,
        .day     = tempDay,    .month  = tempMonth,  .year   = tempYear,
        .weekday = 0,
    };
    ds3231_set_time(&dt);
    printf("Settings saved: %02d:%02d:%02d  %04d-%02d-%02d\r\n",
           tempHour, tempMinute, tempSecond, tempYear, tempMonth, tempDay);

    max7219_scroll_text("Settings Updated", 50);
}

/* ── Main ── */
int main(void)
{
    board_init();
    printf("LED Matrix Clock starting...\r\n");

    /* Init peripherals */
    if (ds3231_init() < 0)
        printf("DS3231 init failed!\r\n");

    max7219_init(BRIGHTNESS);
    encoder_init();
    load_current_time();

    /* Timing */
    uint64_t last_second_ms  = bflb_mtimer_get_time_ms();
    uint64_t last_display_ms = bflb_mtimer_get_time_ms();
    uint64_t btn_press_start = 0;
    bool     long_press_seen = false;
    bool     colon_blink     = false;
    bool     setting_blink   = false;
    uint64_t blink_timer     = bflb_mtimer_get_time_ms();

    char hhmm[8] = "00:00";
    char ss[4]   = "00";

    /* Immediately fetch time */
    {
        rtc_datetime_t dt;
        if (ds3231_get_time(&dt) == 0) {
            snprintf(hhmm, sizeof(hhmm), "%02d:%02d", dt.hour, dt.minute);
            snprintf(ss,   sizeof(ss),   "%02d", dt.second);
        }
    }

    while (1) {
        uint64_t now_ms = bflb_mtimer_get_time_ms();

        /* ── Blink timer (500 ms) ── */
        if (now_ms - blink_timer >= 500) {
            blink_timer   = now_ms;
            colon_blink   = !colon_blink;
            setting_blink = !setting_blink;
        }

        /* ── Encoder ── */
        if (encoder_has_changed()) {
            int val = encoder_get_value();
            switch (currentMode) {
                case MODE_TIME_SETTING:
                    if      (timeSubMode == TIME_HOUR)   tempHour   = val;
                    else if (timeSubMode == TIME_MINUTE) tempMinute = val;
                    else                                 tempSecond = val;
                    break;
                case MODE_DATE_SETTING:
                    if      (dateSubMode == DATE_DAY)   tempDay   = val;
                    else if (dateSubMode == DATE_MONTH) tempMonth = val;
                    else                                tempYear  = val;
                    break;
                default: break;
            }
        }

        /* ── Button short press ── */
        if (encoder_was_pressed()) {
            btn_press_start = now_ms;
            long_press_seen = false;

            switch (currentMode) {
                case MODE_NORMAL:
                    max7219_scroll_text("Time Change", 50);
                    currentMode = MODE_TIME_SETTING;
                    timeSubMode = TIME_HOUR;
                    load_current_time();
                    encoder_set_bounds(0, 23);
                    encoder_set_value(tempHour);
                    break;

                case MODE_TIME_SETTING:
                    if (timeSubMode == TIME_HOUR) {
                        timeSubMode = TIME_MINUTE;
                        encoder_set_bounds(0, 59);
                        encoder_set_value(tempMinute);
                    } else if (timeSubMode == TIME_MINUTE) {
                        timeSubMode = TIME_SECOND;
                        encoder_set_bounds(0, 59);
                        encoder_set_value(tempSecond);
                    } else {
                        max7219_scroll_text("Date Change", 50);
                        max7219_scroll_text("Day", 50);
                        currentMode = MODE_DATE_SETTING;
                        dateSubMode = DATE_DAY;
                        encoder_set_bounds(1, 31);
                        encoder_set_value(tempDay);
                    }
                    break;

                case MODE_DATE_SETTING:
                    if (dateSubMode == DATE_DAY) {
                        max7219_scroll_text("Month", 50);
                        dateSubMode = DATE_MONTH;
                        encoder_set_bounds(1, 12);
                        encoder_set_value(tempMonth);
                    } else if (dateSubMode == DATE_MONTH) {
                        max7219_scroll_text("Year", 50);
                        dateSubMode = DATE_YEAR;
                        encoder_set_bounds(2020, 2050);
                        encoder_set_value(tempYear);
                    } else {
                        save_settings();
                        currentMode = MODE_NORMAL;
                    }
                    break;
            }
        }

        /* ── Long press (5 s) — save immediately ── */
        if (encoder_is_pressed() && btn_press_start > 0) {
            if ((now_ms - btn_press_start) >= 5000 && !long_press_seen) {
                long_press_seen = true;
                if (currentMode != MODE_NORMAL) {
                    save_settings();
                    currentMode = MODE_NORMAL;
                    printf("Long press - saved\r\n");
                }
            }
        }

        /* ── 1-second RTC tick ── */
        if (now_ms - last_second_ms >= 1000) {
            last_second_ms = now_ms;

            if (currentMode == MODE_NORMAL) {
                rtc_datetime_t dt;
                if (ds3231_get_time(&dt) == 0) {
                    snprintf(hhmm, sizeof(hhmm), "%02d.%02d", dt.hour, dt.minute);
                    //snprintf(ss,   sizeof(ss),   "%02d", dt.second);
                    char new_ss[4];
                    snprintf(new_ss, sizeof(new_ss), "%02d", dt.second);
                    max7219_scroll_down_zone0(new_ss, 10);  /* MODE_3 scroll-down effect */
                    snprintf(ss, sizeof(ss), "%02d", dt.second);

                    /* Scroll date at second 10, matching original */
                    if (dt.second == 10) {
                        const char *days[]   = {"Sunday","Monday","Tuesday","Wednesday",
                                                 "Thursday","Friday","Saturday"};
                        const char *months[] = {"January","February","March","April",
                                                 "May","June","July","August",
                                                 "September","October","November","December"};
                        char date_str[64];
                        snprintf(date_str, sizeof(date_str), "%s, %d %s %d",
                                 days[dt.weekday], dt.day,
                                 months[dt.month - 1], dt.year);
                        max7219_scroll_text(date_str, 50);
                    }
                }
            }
        }

        /* ── Display update (~100 ms) ── */
        if (now_ms - last_display_ms >= 100) {
            last_display_ms = now_ms;

            if (currentMode == MODE_NORMAL) {
                max7219_show_time(hhmm, ss, colon_blink);

            } else if (currentMode == MODE_TIME_SETTING) {
                /* Blink the field being edited */
                char disp_hm[8], disp_ss[4];
                if (timeSubMode == TIME_HOUR)
                    snprintf(disp_hm, sizeof(disp_hm), "%s:%02d",
                             setting_blink ? "  " : (snprintf(disp_hm,3,"%02d",tempHour), disp_hm),
                             tempMinute);
                else if (timeSubMode == TIME_MINUTE)
                    snprintf(disp_hm, sizeof(disp_hm), "%02d:%s",
                             tempHour,
                             setting_blink ? "  " : (snprintf(disp_ss,3,"%02d",tempMinute), disp_ss));
                else
                    snprintf(disp_hm, sizeof(disp_hm), "%02d:%02d", tempHour, tempMinute);

                snprintf(disp_ss, sizeof(disp_ss), "%s",
                         (timeSubMode == TIME_SECOND && setting_blink) ? "  " :
                         (snprintf(disp_ss,3,"%02d",tempSecond), disp_ss));

                max7219_show_time(disp_hm, disp_ss, false);

            } else if (currentMode == MODE_DATE_SETTING) {
                char buf[8] = "    ";
                if (!setting_blink) {
                    if      (dateSubMode == DATE_DAY)   snprintf(buf, sizeof(buf), "%02d", tempDay);
                    else if (dateSubMode == DATE_MONTH) snprintf(buf, sizeof(buf), "%02d", tempMonth);
                    else                                snprintf(buf, sizeof(buf), "%4d", tempYear);
                }
                /* Show centred across all 4 modules */
                max7219_show_time(buf, "  ", false);
            }
        }

        bflb_mtimer_delay_ms(10);
    }
}