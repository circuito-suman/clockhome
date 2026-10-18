#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "Arduino.h"

class RotaryEncoder {
public:
    RotaryEncoder(int pinA, int pinB, int pinButton);

    void begin();
    void update();

    bool wasPressed();
    bool isPressed();
    bool hasChanged();

    void setBoundaries(int min, int max);
    void setValue(int value);
    int getValue();

private:
    int _pinA, _pinB, _pinButton;
    int _minValue, _maxValue;
    int _currentValue;
    int _lastValue;

    bool _buttonPressed;
    bool _lastButtonState;
    bool _buttonStateChanged;

    int _lastA, _lastB;

    unsigned long _lastButtonTime;
    unsigned long _lastEncoderTime;

    bool _hasChanged;
};

#endif