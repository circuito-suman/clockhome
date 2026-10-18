#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stdbool.h>

void encoder_init(void);

bool encoder_has_changed(void);
int  encoder_get_value(void);
void encoder_set_value(int val);
void encoder_set_bounds(int min, int max);

bool encoder_was_pressed(void);
bool encoder_is_pressed(void);

#endif