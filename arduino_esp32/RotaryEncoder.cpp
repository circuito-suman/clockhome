#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int pinA, int pinB, int pinButton) {
    _pinA = pinA;
    _pinB = pinB;
    _pinButton = pinButton;
    _minValue = 0;
    _maxValue = 100;
    _currentValue = 0;
    _lastValue = 0;
    _buttonPressed = false;
    _lastButtonState = HIGH;
    _buttonStateChanged = false;
    _lastButtonTime = 0;
    _lastEncoderTime = 0;
    _hasChanged = false;
}

void RotaryEncoder::begin() {
    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    pinMode(_pinButton, INPUT_PULLUP);

    _lastA = digitalRead(_pinA);
    _lastB = digitalRead(_pinB);
    _lastButtonState = digitalRead(_pinButton);
}

void RotaryEncoder::update() {
    unsigned long currentTime = millis();

    // Handle encoder rotation with debouncing (reduced for finer control)
    if (currentTime - _lastEncoderTime > 10) {
        int newA = digitalRead(_pinA);
        int newB = digitalRead(_pinB);

        if (newA != _lastA || newB != _lastB) {
            // Determine rotation direction (REVERSED)
            if (_lastA == LOW && _lastB == HIGH) {
                if (newA == HIGH && newB == HIGH) {
                    _currentValue--;
                } else if (newA == LOW && newB == LOW) {
                    _currentValue++;
                }
            } else if (_lastA == HIGH && _lastB == HIGH) {
                if (newA == LOW && newB == HIGH) {
                    _currentValue--;
                } else if (newA == HIGH && newB == LOW) {
                    _currentValue++;
                }
            } else if (_lastA == HIGH && _lastB == LOW) {
                if (newA == HIGH && newB == HIGH) {
                    _currentValue--;
                } else if (newA == LOW && newB == LOW) {
                    _currentValue++;
                }
            } else if (_lastA == LOW && _lastB == LOW) {
                if (newA == HIGH && newB == LOW) {
                    _currentValue--;
                } else if (newA == LOW && newB == HIGH) {
                    _currentValue++;
                }
            }

            // Apply boundaries
            if (_currentValue > _maxValue) _currentValue = _maxValue;
            if (_currentValue < _minValue) _currentValue = _minValue;

            // Check if value changed
            if (_currentValue != _lastValue) {
                _hasChanged = true;
                _lastValue = _currentValue;
            }

            _lastA = newA;
            _lastB = newB;
            _lastEncoderTime = currentTime;
        }
    }

    // Handle button with debouncing
    if (currentTime - _lastButtonTime > 50) {
        bool currentButtonState = digitalRead(_pinButton);

        if (currentButtonState != _lastButtonState) {
            _lastButtonState = currentButtonState;
            _lastButtonTime = currentTime;

            if (currentButtonState == LOW) {
                _buttonPressed = true;
                _buttonStateChanged = true;
            }
        }
    }
}

bool RotaryEncoder::wasPressed() {
    if (_buttonStateChanged && _buttonPressed) {
        _buttonStateChanged = false;
        _buttonPressed = false;
        return true;
    }
    return false;
}

bool RotaryEncoder::isPressed() {
    return digitalRead(_pinButton) == LOW;
}

bool RotaryEncoder::hasChanged() {
    if (_hasChanged) {
        _hasChanged = false;
        return true;
    }
    return false;
}

void RotaryEncoder::setBoundaries(int min, int max) {
    _minValue = min;
    _maxValue = max;
    if (_currentValue < min) _currentValue = min;
    if (_currentValue > max) _currentValue = max;
}

void RotaryEncoder::setValue(int value) {
    _currentValue = value;
    _lastValue = value;
    if (_currentValue < _minValue) _currentValue = _minValue;
    if (_currentValue > _maxValue) _currentValue = _maxValue;
}

int RotaryEncoder::getValue() {
    return _currentValue;
}