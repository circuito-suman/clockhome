#include "max7219.h"
#include "custom_font.h"
#include "bflb_spi.h"
#include "bflb_gpio.h"
#include "bflb_mtimer.h"
#include <string.h>
#include <stdio.h>

/* ── Pins ── */
#define CS_PIN   3
#define NUM_MOD  4

/* MAX7219 registers */
#define REG_NOOP         0x00
#define REG_DIGIT(n)    (0x01 + (n))
#define REG_DECODE_MODE  0x09
#define REG_INTENSITY    0x0A
#define REG_SCAN_LIMIT   0x0B
#define REG_SHUTDOWN     0x0C
#define REG_DISPLAY_TEST 0x0F

static struct bflb_device_s *spi_dev;
static struct bflb_device_s *gpio_dev;

/* ── SPI helpers ── */
static void cs_low(void)  { bflb_gpio_reset(gpio_dev, CS_PIN); }
static void cs_high(void) { bflb_gpio_set(gpio_dev,   CS_PIN); }

/* Send [reg, data] to every cascaded module in one latch */
static void send_all(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];
    cs_low();
    for (int i = 0; i < NUM_MOD; i++) {
        buf[0] = reg;
        buf[1] = data;
        bflb_spi_poll_exchange(spi_dev, buf, NULL, 2);
    }
    cs_high();
}

/* Send a different command to each module in one latch (right-to-left) */
static void send_cascade(const uint8_t regs[NUM_MOD],
                         const uint8_t data[NUM_MOD])
{
    uint8_t buf[2];
    cs_low();
    for (int i = NUM_MOD - 1; i >= 0; i--) {
        buf[0] = regs[i];
        buf[1] = data[i];
        bflb_spi_poll_exchange(spi_dev, buf, NULL, 2);
    }
    cs_high();
}

/* ── Init ── */
void max7219_init(int brightness)
{
    gpio_dev = bflb_device_get_by_name("gpio");
    spi_dev  = bflb_device_get_by_name("spi0");

    bflb_gpio_init(gpio_dev, CS_PIN,
                   GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
    cs_high();

    struct bflb_spi_config_s spi_cfg = {
        .freq              = 10000000,
        .role              = SPI_ROLE_MASTER,
        .mode              = SPI_MODE0,
        .data_width        = SPI_DATA_WIDTH_8BIT,
        
        .tx_fifo_threshold = 0,
        .rx_fifo_threshold = 0,
    };
    bflb_spi_init(spi_dev, &spi_cfg);

    /* MAX7219 startup */
    send_all(REG_DISPLAY_TEST, 0);
    send_all(REG_DECODE_MODE,  0);
    send_all(REG_SCAN_LIMIT,   7);
    send_all(REG_SHUTDOWN,     1);
    max7219_set_brightness(brightness);
    max7219_clear();

    send_all(REG_DISPLAY_TEST, 1);
    bflb_mtimer_delay_ms(300);
    send_all(REG_DISPLAY_TEST, 0);
}

void max7219_set_brightness(int level)
{
    if (level < 0)  level = 0;
    if (level > 15) level = 15;
    send_all(REG_INTENSITY, (uint8_t)level);
}

void max7219_clear(void)
{
    for (int row = 0; row < 8; row++)
        send_all(REG_DIGIT(row), 0x00);
}

/* ── Framebuffer ── */
/* cols[0..7]=module0(rightmost=seconds), cols[8..15]=mod1, etc. */
static uint8_t fb[32];

static void fb_clear(void) { memset(fb, 0, sizeof(fb)); }

static void fb_flush(void)
{
    uint8_t regs[NUM_MOD], data[NUM_MOD];
    for (int row = 0; row < 8; row++) {
        for (int m = 0; m < NUM_MOD; m++) {
            regs[m] = REG_DIGIT(row);
            data[m] = fb[m * 8 + row];
        }
        send_cascade(regs, data);
    }
}

/* ── Font lookup ──
 * MD_Parola flat array format:
 *   Each char entry: [width_byte, col0, col1, ... colN]
 *   width_byte == 0 means "no glyph" (1 byte entry, skip)
 *   Entries are stored from char index 0 sequentially.
 */
static const uint8_t *find_glyph(const uint8_t *font, uint8_t c)
{
    const uint8_t *p = font;
    for (uint8_t i = 0; i < c; i++) {
        uint8_t w = *p;
        p += (w == 0) ? 1 : (1 + w);
    }
    return p;
}

/* Draw one char from a font into fb[] starting at col_start.
 * max_col limits how far right we draw.
 * Returns next column position. */
static int draw_char(const uint8_t *font, char c,
                     int col_start, int max_col)
{
    if ((uint8_t)c < 0x20) return col_start;
    const uint8_t *glyph = find_glyph(font, (uint8_t)c);
    uint8_t width = glyph[0];
    if (width == 0) return col_start + 2; /* no glyph, advance by space */
    for (int i = 0; i < width && (col_start + i) < max_col; i++) {
        fb[col_start + i] = glyph[1 + i];
    }
    return col_start + width + 1; /* +1 inter-char gap */
}

/* ── Public display functions ── */

void max7219_show_time(const char *hhmm, const char *ss, bool blink_colon)
{
    fb_clear();

    /* Zone 0 (cols 0-7, module 0 = rightmost): seconds in 3x5 font */
    int col = 1; /* small left margin */
    for (int i = 0; ss[i] && col < 8; i++)
        col = draw_char(customFont3x5, ss[i], col, 8);

    /* Zone 1 (cols 8-31, modules 1-3): HH:MM in 4x7 font */
    col = 9;
    for (int i = 0; hhmm[i] && col < 32; i++) {
        char ch = hhmm[i];
        if (ch == '.' && blink_colon) ch = ' ';
        col = draw_char(customFont4x7, ch, col, 32);
    }

    fb_flush();
}

void max7219_scroll_text(const char *msg, int speed_ms)
{
    int len = strlen(msg);

    /* Render message into a wide scroll buffer */
    uint8_t scroll[192];
    memset(scroll, 0, sizeof(scroll));

    int col = 0;
    for (int i = 0; i < len && col < (int)(sizeof(scroll) - 10); i++) {
        uint8_t c = (uint8_t)msg[i];
        if (c < 0x20) continue;
        const uint8_t *glyph = find_glyph(customFont4x7, c);
        uint8_t w = glyph[0];
        if (w == 0) { col += 2; continue; }
        for (int j = 0; j < w && col < (int)sizeof(scroll); j++)
            scroll[col++] = glyph[1 + j];
        scroll[col++] = 0; /* gap */
    }
    int msg_cols = col;

    /* Scroll left: window of 32 columns slides across scroll[] */
    for (int start = -32; start < msg_cols; start++) {
        fb_clear();
        for (int c = 0; c < 32; c++) {
            int src = start + c;
            fb[c] = (src >= 0 && src < msg_cols) ? scroll[src] : 0;
        }
        fb_flush();
        bflb_mtimer_delay_ms(speed_ms);
    }
}
void max7219_write_raw(const uint8_t cols[32])
{
    memcpy(fb, cols, 32);
    fb_flush();
}

void max7219_blink_zone(int zone, bool on)
{
    /* zone 0 = cols 0-7, zone 1 = cols 8-31 */
    int start = (zone == 0) ? 0 : 8;
    int end   = (zone == 0) ? 8 : 32;
    if (!on) {
        for (int i = start; i < end; i++) fb[i] = 0;
        fb_flush();
    }
}

void max7219_scroll_down_zone0(const char *new_ss, int speed_ms)
{
    /* Render new seconds into a temp buffer */
    uint8_t new_zone[8] = {0};
    int col = 1;
    for (int i = 0; new_ss[i] && col < 8; i++) {
        if ((uint8_t)new_ss[i] < 0x20) continue;
        const uint8_t *glyph = find_glyph(customFont3x5, (uint8_t)new_ss[i]);
        uint8_t w = glyph[0];
        for (int j = 0; j < w && col < 8; j++)
            new_zone[col++] = glyph[1 + j];
        col++; /* gap */
    }

    /* Scroll down: shift rows downward one step at a time */
    for (int shift = 1; shift <= 8; shift++) {
        for (int c = 0; c < 8; c++) {
            /* current content shifts down, new content comes from top */
            fb[c] = (fb[c] >> shift) | (new_zone[c] << (8 - shift));
        }
        fb_flush();
        bflb_mtimer_delay_ms(speed_ms);
    }
}