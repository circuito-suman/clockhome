#include "encoder.h"
#include "bflb_gpio.h"
#include "bflb_mtimer.h"
#include <stdbool.h>

#define ENC_A_PIN   10
#define ENC_B_PIN   11
#define ENC_BTN_PIN 12

static struct bflb_device_s *gpio_dev;
static volatile int  _value     = 0;
static int           _min       = 0, _max = 100;
static volatile bool _changed   = false;
static volatile bool _btn_event = false;
static int _last_a, _last_b;
static uint64_t _last_enc_ms = 0, _last_btn_ms = 0;

static void encoder_isr(uint8_t pin)
{
    uint64_t now = bflb_mtimer_get_time_ms();

    if (pin == ENC_A_PIN || pin == ENC_B_PIN) {
        if (now - _last_enc_ms < 10) return;
        _last_enc_ms = now;

        int a = bflb_gpio_read(gpio_dev, ENC_A_PIN);
        int b = bflb_gpio_read(gpio_dev, ENC_B_PIN);

        int delta = 0;
        if      (_last_a==0&&_last_b==1) { if(a==1&&b==1) delta=-1; else if(a==0&&b==0) delta=+1; }
        else if (_last_a==1&&_last_b==1) { if(a==0&&b==1) delta=-1; else if(a==1&&b==0) delta=+1; }
        else if (_last_a==1&&_last_b==0) { if(a==1&&b==1) delta=-1; else if(a==0&&b==0) delta=+1; }
        else if (_last_a==0&&_last_b==0) { if(a==1&&b==0) delta=-1; else if(a==0&&b==1) delta=+1; }

        if (delta) {
            int v = _value + delta;
            _value = v < _min ? _min : v > _max ? _max : v;
            _changed = true;
        }
        _last_a = a;
        _last_b = b;
    }

    if (pin == ENC_BTN_PIN) {
        if (now - _last_btn_ms < 50) return;
        _last_btn_ms = now;
        if (!bflb_gpio_read(gpio_dev, ENC_BTN_PIN)) {
            _btn_event = true;
        }
    }
}

void encoder_init(void)
{
    gpio_dev = bflb_device_get_by_name("gpio");

    bflb_gpio_init(gpio_dev, ENC_A_PIN,   GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);
    bflb_gpio_init(gpio_dev, ENC_B_PIN,   GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);
    bflb_gpio_init(gpio_dev, ENC_BTN_PIN, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);

    _last_a = bflb_gpio_read(gpio_dev, ENC_A_PIN);
    _last_b = bflb_gpio_read(gpio_dev, ENC_B_PIN);

    /* Configure interrupt trigger mode */
    bflb_gpio_int_init(gpio_dev, ENC_A_PIN,   GPIO_INT_TRIG_MODE_SYNC_FALLING_EDGE);
    bflb_gpio_int_init(gpio_dev, ENC_B_PIN,   GPIO_INT_TRIG_MODE_SYNC_FALLING_EDGE);
    bflb_gpio_int_init(gpio_dev, ENC_BTN_PIN, GPIO_INT_TRIG_MODE_SYNC_FALLING_EDGE);

    /* Attach callbacks */
    bflb_gpio_irq_attach(ENC_A_PIN,   encoder_isr);
    bflb_gpio_irq_attach(ENC_B_PIN,   encoder_isr);
    bflb_gpio_irq_attach(ENC_BTN_PIN, encoder_isr);

    /* Unmask = enable interrupts (mask=false means enabled) */
    bflb_gpio_int_mask(gpio_dev, ENC_A_PIN,   false);
    bflb_gpio_int_mask(gpio_dev, ENC_B_PIN,   false);
    bflb_gpio_int_mask(gpio_dev, ENC_BTN_PIN, false);
}

bool encoder_has_changed(void)
{
    if (_changed) { _changed = false; return true; }
    return false;
}

int  encoder_get_value(void)            { return _value; }
void encoder_set_value(int v)           { _value = v < _min ? _min : v > _max ? _max : v; }
void encoder_set_bounds(int mn, int mx) { _min = mn; _max = mx; encoder_set_value(_value); }

bool encoder_was_pressed(void)
{
    if (_btn_event) { _btn_event = false; return true; }
    return false;
}

bool encoder_is_pressed(void)
{
    return !bflb_gpio_read(gpio_dev, ENC_BTN_PIN);
}