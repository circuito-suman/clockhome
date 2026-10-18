#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>
#include <stdbool.h>


void max7219_init(int brightness);
void max7219_set_brightness(int level); /* 0–15 */
void max7219_clear(void);
void max7219_write_raw(const uint8_t cols[32]); /* 4 modules × 8 cols */

/* High-level: render time string */
void max7219_show_time(const char *hhmm,   /* "HH:MM" or "HH.MM" */
                       const char *ss,     /* "SS" */
                       bool       blink_colon);

/* Scrolling text — blocks until done */
void max7219_scroll_text(const char *msg, int speed_ms);

/* Blink a zone (for setting mode) */
void max7219_blink_zone(int zone, bool on); /* zone 0=seconds, 1=hhmm */

/* Scroll down effect for seconds zone — call once per second */
void max7219_scroll_down_zone0(const char *new_ss, int speed_ms);

#endif