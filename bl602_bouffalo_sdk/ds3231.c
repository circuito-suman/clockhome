#include "ds3231.h"
#include "bflb_i2c.h"
#include "bflb_gpio.h"
#include "bflb_mtimer.h"
#include <string.h>

/* ── Per-RTC constants ── */
#if   RTC_TYPE == RTC_DS3231
  #define RTC_ADDR        0x68
  #define USE_I2C         1

#elif RTC_TYPE == RTC_DS1307
  #define RTC_ADDR        0x68
  #define USE_I2C         1

#elif RTC_TYPE == RTC_PCF8563
  #define RTC_ADDR        0x51
  #define USE_I2C         1

#elif RTC_TYPE == RTC_DS1302
  #define USE_I2C         0
  /* DS1302 uses its own 3-wire protocol, not I2C */
  /* Assign GPIO pins for DS1302 */
  #define DS1302_CLK_PIN  6
  #define DS1302_DAT_PIN  7
  #define DS1302_CE_PIN   8
#endif

/* ── Shared helpers ── */
static uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }


#if USE_I2C

static struct bflb_device_s *i2c_dev;

static int i2c_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    struct bflb_i2c_msg_s msgs[2] = {
        { .addr = RTC_ADDR, .flags = 0,        .buffer = &reg, .length = 1   },
        { .addr = RTC_ADDR, .flags = I2C_M_READ,.buffer = buf,  .length = len },
    };
    return bflb_i2c_transfer(i2c_dev, msgs, 2);
}

static int i2c_write_regs(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t tmp[16];
    tmp[0] = reg;
    memcpy(tmp + 1, buf, len);
    struct bflb_i2c_msg_s msg = {
        .addr = RTC_ADDR, .flags = 0,
        .buffer = tmp, .length = (uint16_t)(len + 1),
    };
    return bflb_i2c_transfer(i2c_dev, &msg, 1);
}

/* ── Init ── */
int ds3231_init(void)
{
    i2c_dev = bflb_device_get_by_name("i2c0");
    bflb_i2c_init(i2c_dev, 100000);

#if RTC_TYPE == RTC_DS1307
    /* DS1307: clear CH (clock halt) bit 7 in register 0x00
       if set the oscillator is stopped */
    uint8_t sec;
    if (i2c_read_regs(0x00, &sec, 1) < 0) return -1;
    if (sec & 0x80) {
        sec &= 0x7F;
        i2c_write_regs(0x00, &sec, 1);
    }
#endif

    uint8_t tmp;
    return i2c_read_regs(0x00, &tmp, 1);
}

/* ── Get time ── */
int ds3231_get_time(rtc_datetime_t *dt)
{
    uint8_t raw[7];

#if RTC_TYPE == RTC_DS3231 || RTC_TYPE == RTC_DS1307
    /* Register map 0x00-0x06:
       [0]=sec [1]=min [2]=hr [3]=wday [4]=day [5]=mon [6]=yr */
    if (i2c_read_regs(0x00, raw, 7) < 0) return -1;

    dt->second  = bcd2dec(raw[0] & 0x7F);
    dt->minute  = bcd2dec(raw[1] & 0x7F);
    dt->hour    = bcd2dec(raw[2] & 0x3F); /* force 24hr */
    dt->weekday = (raw[3] & 0x07) - 1;    /* 1-based → 0-based */
    dt->day     = bcd2dec(raw[4] & 0x3F);
    dt->month   = bcd2dec(raw[5] & 0x1F);
    dt->year    = bcd2dec(raw[6]) + 2000;

#elif RTC_TYPE == RTC_PCF8563
    /* PCF8563 register map 0x02-0x08:
       [0]=sec [1]=min [2]=hr [3]=day [4]=wday [5]=mon [6]=yr
       note: weekday and day are swapped vs DS3231 */
    if (i2c_read_regs(0x02, raw, 7) < 0) return -1;

    dt->second  = bcd2dec(raw[0] & 0x7F);
    dt->minute  = bcd2dec(raw[1] & 0x7F);
    dt->hour    = bcd2dec(raw[2] & 0x3F);
    dt->day     = bcd2dec(raw[3] & 0x3F);
    dt->weekday = (raw[4] & 0x07);        /* already 0=Sunday */
    dt->month   = bcd2dec(raw[5] & 0x1F);
    /* PCF8563 year is 0-99, century bit is in month register bit 7 */
    dt->year    = bcd2dec(raw[6]) + 2000;
#endif

    return 0;
}

/* ── Set time ── */
int ds3231_set_time(const rtc_datetime_t *dt)
{
#if RTC_TYPE == RTC_DS3231 || RTC_TYPE == RTC_DS1307
    uint8_t raw[7] = {
        dec2bcd(dt->second),
        dec2bcd(dt->minute),
        dec2bcd(dt->hour),
        (uint8_t)(dt->weekday + 1),  /* 0-based → 1-based */
        dec2bcd(dt->day),
        dec2bcd(dt->month),
        dec2bcd(dt->year - 2000),
    };
    return i2c_write_regs(0x00, raw, 7);

#elif RTC_TYPE == RTC_PCF8563
    uint8_t raw[7] = {
        dec2bcd(dt->second),
        dec2bcd(dt->minute),
        dec2bcd(dt->hour),
        dec2bcd(dt->day),
        (uint8_t)(dt->weekday & 0x07),
        dec2bcd(dt->month),
        dec2bcd(dt->year - 2000),
    };
    return i2c_write_regs(0x02, raw, 7);
#endif
}

//  * DS1302 — 3-wire bit-bang path

#else /* !USE_I2C */

static struct bflb_device_s *gpio_dev;

static void ds1302_ce_high(void)  { bflb_gpio_set(gpio_dev,   DS1302_CE_PIN);  }
static void ds1302_ce_low(void)   { bflb_gpio_reset(gpio_dev, DS1302_CE_PIN);  }
static void ds1302_clk_high(void) { bflb_gpio_set(gpio_dev,   DS1302_CLK_PIN); }
static void ds1302_clk_low(void)  { bflb_gpio_reset(gpio_dev, DS1302_CLK_PIN); }

static void ds1302_write_byte(uint8_t byte)
{
    bflb_gpio_init(gpio_dev, DS1302_DAT_PIN,
                   GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN);
    for (int i = 0; i < 8; i++) {
        if (byte & 0x01)
            bflb_gpio_set(gpio_dev, DS1302_DAT_PIN);
        else
            bflb_gpio_reset(gpio_dev, DS1302_DAT_PIN);
        ds1302_clk_high();
        ds1302_clk_low();
        byte >>= 1;
    }
}

static uint8_t ds1302_read_byte(void)
{
    bflb_gpio_init(gpio_dev, DS1302_DAT_PIN,
                   GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (bflb_gpio_read(gpio_dev, DS1302_DAT_PIN))
            byte |= 0x80;
        ds1302_clk_high();
        ds1302_clk_low();
    }
    return byte;
}

static void ds1302_write_reg(uint8_t reg, uint8_t val)
{
    ds1302_ce_high();
    ds1302_write_byte(reg & 0xFE); /* clear read bit */
    ds1302_write_byte(val);
    ds1302_ce_low();
}

static uint8_t ds1302_read_reg(uint8_t reg)
{
    ds1302_ce_high();
    ds1302_write_byte(reg | 0x01); /* set read bit */
    uint8_t val = ds1302_read_byte();
    ds1302_ce_low();
    return val;
}

int ds3231_init(void)
{
    gpio_dev = bflb_device_get_by_name("gpio");

    bflb_gpio_init(gpio_dev, DS1302_CLK_PIN,
                   GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN);
    bflb_gpio_init(gpio_dev, DS1302_CE_PIN,
                   GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN);

    ds1302_ce_low();
    ds1302_clk_low();

    /* Disable write protect */
    ds1302_write_reg(0x8E, 0x00);

    /* Clear clock halt bit (CH) in seconds register 0x80 */
    uint8_t sec = ds1302_read_reg(0x81);
    if (sec & 0x80) {
        sec &= 0x7F;
        ds1302_write_reg(0x80, sec);
    }

    return 0;
}

int ds3231_get_time(rtc_datetime_t *dt)
{
    /* DS1302 burst read: command 0xBF reads all 8 clock registers */
    ds1302_ce_high();
    ds1302_write_byte(0xBF);
    uint8_t raw[8];
    for (int i = 0; i < 8; i++)
        raw[i] = ds1302_read_byte();
    ds1302_ce_low();

    dt->second  = bcd2dec(raw[0] & 0x7F);
    dt->minute  = bcd2dec(raw[1] & 0x7F);
    dt->hour    = bcd2dec(raw[2] & 0x3F);
    dt->day     = bcd2dec(raw[3] & 0x3F);
    dt->month   = bcd2dec(raw[4] & 0x1F);
    dt->weekday = (raw[5] & 0x07) - 1;
    dt->year    = bcd2dec(raw[6]) + 2000;
    return 0;
}

int ds3231_set_time(const rtc_datetime_t *dt)
{
    /* DS1302 burst write: command 0xBE writes all 8 clock registers */
    ds1302_ce_high();
    ds1302_write_byte(0xBE);
    ds1302_write_byte(dec2bcd(dt->second));
    ds1302_write_byte(dec2bcd(dt->minute));
    ds1302_write_byte(dec2bcd(dt->hour));
    ds1302_write_byte(dec2bcd(dt->day));
    ds1302_write_byte(dec2bcd(dt->month));
    ds1302_write_byte((uint8_t)(dt->weekday + 1));
    ds1302_write_byte(dec2bcd(dt->year - 2000));
    ds1302_write_byte(0x00); /* write protect off */
    ds1302_ce_low();
    return 0;
}

#endif /* USE_I2C */