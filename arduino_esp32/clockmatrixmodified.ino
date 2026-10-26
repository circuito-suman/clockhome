/*
 * Offline Digital Clock - ESP32-C3 Super Mini
 *
 * PIN ASSIGNMENTS:
 * ─────────────────────────────────────────
 * MAX7219 (Software SPI):
 *   CLK  -> GPIO6
 *   DATA -> GPIO7
 *   CS   -> GPIO10
 *
 * RTC DS3231 (I2C):
 *   SDA  -> GPIO8
 *   SCL  -> GPIO9
 *
 * Rotary Encoder:
 *   A      -> GPIO0
 *   B      -> GPIO1
 *   Button -> GPIO2  (also: 2s hold = toggle encoder/button mode)
 *
 * Dedicated Buttons (active in button mode):
 *   INC    -> GPIO4
 *   DEC    -> GPIO5
 *
 * INPUT MODE TOGGLE:
 *   Hold encoder button 2s → switches encoder ↔ button mode
 *   Short press           → normal mode cycling (unchanged)
 */

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <Wire.h>
#include "RTClib.h"
#include "Custom_Font_Data.h"
#include "RotaryEncoder.h"

// ── Hardware pins ────────────────────────────────────────────────────────────
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4

#define CLK_PIN   6
#define DATA_PIN  7
#define CS_PIN    10

#define I2C_SDA   8
#define I2C_SCL   9

#define ROTARY_ENCODER_A_PIN      0
#define ROTARY_ENCODER_B_PIN      1
#define ROTARY_ENCODER_BUTTON_PIN 2   // Short press = cycle, hold 2s = toggle input mode

#define BTN_INC_PIN  4   // Increment button (button mode only)
#define BTN_DEC_PIN  5   // Decrement button (button mode only)

// ── Display config ───────────────────────────────────────────────────────────
#define SPEED_TIME_SCROLLING_TEXT  50
#define SPEED_TIME_SCROLLING_DOWN  80
#define MAX_SCROLLING_TEXT_CHAR    30
#define SPEED_TIME  75
#define PAUSE_TIME  0
#define MAX_MESG    20

#define MODE_1  1
#define MODE_2  2
#define MODE_3  3
byte DIGITAL_CLOCK_DISPLAY_MODE = MODE_3;

// ── Enums ────────────────────────────────────────────────────────────────────
typedef enum { MODE_NORMAL = 0, MODE_TIME_SETTING, MODE_DATE_SETTING } DisplayMode;
typedef enum { TIME_HOUR = 0, TIME_MINUTE, TIME_SECOND }               TimeSubMode;
typedef enum { DATE_DAY = 0, DATE_MONTH, DATE_YEAR }                   DateSubMode;

// Input mode
typedef enum { INPUT_ENCODER = 0, INPUT_BUTTONS } InputMode;

// ── Global objects ───────────────────────────────────────────────────────────
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
RotaryEncoder encoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN);
RTC_DS3231 rtc;

// ── State ────────────────────────────────────────────────────────────────────
DisplayMode currentMode   = MODE_NORMAL;
TimeSubMode timeSubMode   = TIME_HOUR;
DateSubMode dateSubMode   = DATE_DAY;
InputMode   inputMode     = INPUT_ENCODER;   // default: encoder

int brightness = 5;

char szTime[9];
char Time_S[3];
char Time_H_M[10];
char str_Date[50];
char szMesg[MAX_MESG + 1] = "";

unsigned long previousMillis  = 0;
unsigned long blinkTimer      = 0;
unsigned long buttonPressStart = 0;

bool blinkState        = false;
bool settingChanged    = false;
bool longPressDetected = false;
bool colon_Blink       = false;

int tempHour, tempMinute, tempSecond;
int tempDay,  tempMonth,  tempYear;

// Current adjustable value when in button mode
int  btnModeValue    = 0;
int  btnModeMin      = 0;
int  btnModeMax      = 23;

// Button mode repeat-on-hold config
#define BTN_DEBOUNCE_MS    50
#define BTN_REPEAT_DELAY  500   // ms before repeat starts
#define BTN_REPEAT_RATE    80   // ms between repeats while held

struct RepeatBtn {
  uint8_t pin;
  bool    lastState;
  bool    pressed;
  unsigned long pressedAt;
  unsigned long lastRepeat;
};
RepeatBtn incBtn = {BTN_INC_PIN, HIGH, false, 0, 0};
RepeatBtn decBtn = {BTN_DEC_PIN, HIGH, false, 0, 0};

// Encoder button (mode toggle on long hold)
bool     encBtnLastState    = HIGH;
bool     encBtnLongFired    = false;
unsigned long encBtnPressAt = 0;
#define  ENC_LONG_PRESS_MS  2000   // 2s hold → toggle input mode

char daysOfTheWeek[7][12] = {"Sunday","Monday","Tuesday","Wednesday",
                              "Thursday","Friday","Saturday"};
char month_Name[12][10]   = {"January","February","March","April","May","June",
                              "July","August","September","October","November","December"};

// ── Forward declarations ─────────────────────────────────────────────────────
void set_Clock_Display();
void scroll_Display(char*);
void get_Time_and_Date();
void handleEncoder();
void handleButtons();
void handleEncoderButton();
void updateDisplay();
void displaySettingMode();
void loadCurrentTime();
void saveSettings();
void updateBrightness();
void applyValueChange(int delta);
void setBtnModeBoundaries(int minVal, int maxVal, int currentVal);
void showInputModeMessage();

// ── set_Clock_Display() ──────────────────────────────────────────────────────
void set_Clock_Display() {
  if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
    P.begin();
    P.setIntensity(brightness);
    P.setInvert(false);
    P.setFont(0, customFont5x7);
    P.displayText(Time_H_M, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }
  if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
    P.begin(2);
    P.setInvert(false);
    P.setZone(0, 0, 0);
    P.setZone(1, 1, 3);
    P.setIntensity(brightness);
    P.setFont(0, customFont3x5);
    P.setFont(1, customFont4x7);
    if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2)
      P.displayZoneText(0, Time_S, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_3)
      P.displayZoneText(0, Time_S, PA_RIGHT, SPEED_TIME_SCROLLING_DOWN, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);
    P.displayZoneText(1, Time_H_M, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }
}

// ── scroll_Display() ─────────────────────────────────────────────────────────
void scroll_Display(char* msg) {
  P.begin();
  P.setIntensity(brightness);
  P.setInvert(false);
  delay(500);
  P.displayScroll(msg, PA_RIGHT, PA_SCROLL_LEFT, SPEED_TIME_SCROLLING_TEXT);
  while (true) {
    if (P.displayAnimate()) {
      P.displayReset();
      set_Clock_Display();
      delay(500);
      break;
    }
    delay(1);
  }
}

// ── get_Time_and_Date() ──────────────────────────────────────────────────────
void get_Time_and_Date() {
  DateTime now = rtc.now();
  if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
    colon_Blink = !colon_Blink;
    sprintf(Time_H_M, "%02d %s %02d", now.hour(), (colon_Blink ? ":" : "^"), now.minute());
  } else {
    sprintf(Time_H_M, "%02d:%02d", now.hour(), now.minute());
  }
  sprintf(Time_S, "%02d", now.second());
  sprintf(str_Date, "%s, %d %s %d",
          daysOfTheWeek[now.dayOfTheWeek()],
          now.day(),
          month_Name[now.month() - 1],
          now.year());
}

// ── showInputModeMessage() ───────────────────────────────────────────────────
void showInputModeMessage() {
  if (inputMode == INPUT_ENCODER) {
    char msg[] = "Encoder Mode";
    scroll_Display(msg);
  } else {
    char msg[] = "Button Mode";
    scroll_Display(msg);
  }
}

// ── setBtnModeBoundaries() ───────────────────────────────────────────────────
// Mirrors encoder.setBoundaries() + encoder.setValue() for button mode
void setBtnModeBoundaries(int minVal, int maxVal, int currentVal) {
  btnModeMin   = minVal;
  btnModeMax   = maxVal;
  btnModeValue = currentVal;
}

// ── applyValueChange() ───────────────────────────────────────────────────────
// Single place that routes a +1/-1 delta to the correct temp variable
// Called from both encoder change handler and button handler
void applyValueChange(int delta) {
  settingChanged = true;

  switch (currentMode) {
    case MODE_TIME_SETTING:
      switch (timeSubMode) {
        case TIME_HOUR:
          tempHour = constrain(tempHour + delta, 0, 23);
          if (inputMode == INPUT_BUTTONS) btnModeValue = tempHour;
          Serial.printf("Hour: %d\n", tempHour);
          break;
        case TIME_MINUTE:
          tempMinute = constrain(tempMinute + delta, 0, 59);
          if (inputMode == INPUT_BUTTONS) btnModeValue = tempMinute;
          Serial.printf("Minute: %d\n", tempMinute);
          break;
        case TIME_SECOND:
          tempSecond = constrain(tempSecond + delta, 0, 59);
          if (inputMode == INPUT_BUTTONS) btnModeValue = tempSecond;
          Serial.printf("Second: %d\n", tempSecond);
          break;
      }
      break;

    case MODE_DATE_SETTING:
      switch (dateSubMode) {
        case DATE_DAY:
          tempDay = constrain(tempDay + delta, 1, 31);
          if (inputMode == INPUT_BUTTONS) btnModeValue = tempDay;
          Serial.printf("Day: %d\n", tempDay);
          break;
        case DATE_MONTH:
          tempMonth = constrain(tempMonth + delta, 1, 12);
          if (inputMode == INPUT_BUTTONS) btnModeValue = tempMonth;
          Serial.printf("Month: %d\n", tempMonth);
          break;
        case DATE_YEAR:
          tempYear = constrain(tempYear + delta, 2020, 2050);
          if (inputMode == INPUT_BUTTONS) btnModeValue = tempYear;
          Serial.printf("Year: %d\n", tempYear);
          break;
      }
      break;

    default: break;
  }
}

// ── handleEncoderButton() ────────────────────────────────────────────────────
// Reads GPIO2 directly so we can detect both short press and 2s long press.
// Short press → mode cycling (same as before).
// Hold 2s     → toggle encoder/button input mode.
void handleEncoderButton() {
  bool state = digitalRead(ROTARY_ENCODER_BUTTON_PIN);  // LOW = pressed (INPUT_PULLUP)

  // Falling edge — button just pressed
  if (encBtnLastState == HIGH && state == LOW) {
    encBtnPressAt   = millis();
    encBtnLongFired = false;
  }

  // While held — check for long press
  if (state == LOW && !encBtnLongFired) {
    if (millis() - encBtnPressAt >= ENC_LONG_PRESS_MS) {
      encBtnLongFired = true;
      // Toggle input mode
      inputMode = (inputMode == INPUT_ENCODER) ? INPUT_BUTTONS : INPUT_ENCODER;
      Serial.printf("Input mode → %s\n", inputMode == INPUT_ENCODER ? "ENCODER" : "BUTTONS");
      showInputModeMessage();
      // Re-sync btn mode value if we just switched to button mode mid-setting
      if (inputMode == INPUT_BUTTONS && currentMode != MODE_NORMAL) {
        switch (currentMode) {
          case MODE_TIME_SETTING:
            switch (timeSubMode) {
              case TIME_HOUR:   setBtnModeBoundaries(0,    23,   tempHour);   break;
              case TIME_MINUTE: setBtnModeBoundaries(0,    59,   tempMinute); break;
              case TIME_SECOND: setBtnModeBoundaries(0,    59,   tempSecond); break;
            }
            break;
          case MODE_DATE_SETTING:
            switch (dateSubMode) {
              case DATE_DAY:   setBtnModeBoundaries(1,    31,   tempDay);   break;
              case DATE_MONTH: setBtnModeBoundaries(1,    12,   tempMonth); break;
              case DATE_YEAR:  setBtnModeBoundaries(2020, 2050, tempYear);  break;
            }
            break;
          default: break;
        }
      }
    }
  }

  // Rising edge — button released
  if (encBtnLastState == LOW && state == HIGH) {
    // Only treat as short press if long press did NOT fire
    if (!encBtnLongFired) {
      handleShortPress();
    }
  }

  encBtnLastState = state;
}

// ── handleShortPress() ───────────────────────────────────────────────────────
// Exactly the mode-cycling logic from the original, extracted for clarity
void handleShortPress() {
  switch (currentMode) {
    case MODE_NORMAL: {
      char msg[] = "Time Change";
      scroll_Display(msg);
      currentMode = MODE_TIME_SETTING;
      timeSubMode = TIME_HOUR;
      loadCurrentTime();
      if (inputMode == INPUT_ENCODER) {
        encoder.setBoundaries(0, 23);
        encoder.setValue(tempHour);
      } else {
        setBtnModeBoundaries(0, 23, tempHour);
      }
      Serial.println("→ Time Setting: Hour");
      break;
    }

    case MODE_TIME_SETTING: {
      if (timeSubMode == TIME_HOUR) {
        timeSubMode = TIME_MINUTE;
        if (inputMode == INPUT_ENCODER) { encoder.setBoundaries(0, 59); encoder.setValue(tempMinute); }
        else                            { setBtnModeBoundaries(0, 59, tempMinute); }
        Serial.println("→ Time Setting: Minute");

      } else if (timeSubMode == TIME_MINUTE) {
        timeSubMode = TIME_SECOND;
        if (inputMode == INPUT_ENCODER) { encoder.setBoundaries(0, 59); encoder.setValue(tempSecond); }
        else                            { setBtnModeBoundaries(0, 59, tempSecond); }
        Serial.println("→ Time Setting: Second");

      } else {
        { char msg[] = "Date Change"; scroll_Display(msg); }
        { char msg[] = "Day";         scroll_Display(msg); }
        currentMode = MODE_DATE_SETTING;
        dateSubMode = DATE_DAY;
        if (inputMode == INPUT_ENCODER) { encoder.setBoundaries(1, 31); encoder.setValue(tempDay); }
        else                            { setBtnModeBoundaries(1, 31, tempDay); }
        Serial.println("→ Date Setting: Day");
      }
      break;
    }

    case MODE_DATE_SETTING: {
      if (dateSubMode == DATE_DAY) {
        { char msg[] = "Month"; scroll_Display(msg); }
        dateSubMode = DATE_MONTH;
        if (inputMode == INPUT_ENCODER) { encoder.setBoundaries(1, 12);   encoder.setValue(tempMonth); }
        else                            { setBtnModeBoundaries(1, 12, tempMonth); }
        Serial.println("→ Date Setting: Month");

      } else if (dateSubMode == DATE_MONTH) {
        { char msg[] = "Year"; scroll_Display(msg); }
        dateSubMode = DATE_YEAR;
        if (inputMode == INPUT_ENCODER) { encoder.setBoundaries(2020, 2050); encoder.setValue(tempYear); }
        else                            { setBtnModeBoundaries(2020, 2050, tempYear); }
        Serial.println("→ Date Setting: Year");

      } else {
        saveSettings();
        currentMode = MODE_NORMAL;
        Serial.println("→ Normal (saved)");
      }
      break;
    }
  }
}

// ── handleEncoder() ──────────────────────────────────────────────────────────
void handleEncoder() {
  // Always handle the encoder button (it handles both modes)
  handleEncoderButton();

  // Rotation only matters in encoder mode
  if (inputMode != INPUT_ENCODER) return;
  if (currentMode == MODE_NORMAL) return;

  encoder.update();
  if (encoder.hasChanged()) {
    int v = encoder.getValue();
    // Map encoder position directly to temp variable
    switch (currentMode) {
      case MODE_TIME_SETTING:
        switch (timeSubMode) {
          case TIME_HOUR:   tempHour   = v; break;
          case TIME_MINUTE: tempMinute = v; break;
          case TIME_SECOND: tempSecond = v; break;
        }
        break;
      case MODE_DATE_SETTING:
        switch (dateSubMode) {
          case DATE_DAY:   tempDay   = v; break;
          case DATE_MONTH: tempMonth = v; break;
          case DATE_YEAR:  tempYear  = v; break;
        }
        break;
      default: break;
    }
    settingChanged = true;
  }
}

// ── handleButtons() ──────────────────────────────────────────────────────────
// Handles INC/DEC buttons with debounce + repeat-on-hold.
// Only active in INPUT_BUTTONS mode AND when in a setting mode.
void handleButtons() {
  if (inputMode != INPUT_BUTTONS) return;
  if (currentMode == MODE_NORMAL) return;

  unsigned long now = millis();

  auto processBtn = [&](RepeatBtn& btn, int delta) {
    bool reading = digitalRead(btn.pin);  // LOW = pressed

    if (btn.lastState == HIGH && reading == LOW) {
      // Fresh press
      btn.pressed   = true;
      btn.pressedAt = now;
      btn.lastRepeat = now;
      applyValueChange(delta);
    }

    if (reading == LOW && btn.pressed) {
      // Hold — fire repeat after initial delay, then at repeat rate
      if (now - btn.pressedAt >= BTN_REPEAT_DELAY) {
        if (now - btn.lastRepeat >= BTN_REPEAT_RATE) {
          btn.lastRepeat = now;
          applyValueChange(delta);
        }
      }
    }

    if (reading == HIGH) {
      btn.pressed = false;
    }

    btn.lastState = reading;
  };

  processBtn(incBtn, +1);
  processBtn(decBtn, -1);
}

// ── updateDisplay() ──────────────────────────────────────────────────────────
void updateDisplay() {
  static unsigned long lastTimeUpdate = 0;
  unsigned long updateInterval = (currentMode == MODE_NORMAL) ? 1000 : 200;

  if (millis() - lastTimeUpdate < updateInterval) return;
  lastTimeUpdate = millis();

  if (millis() - blinkTimer >= 500) {
    blinkTimer = millis();
    blinkState = !blinkState;
  }

  if (currentMode == MODE_NORMAL) {
    DateTime now = rtc.now();

    if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
      sprintf(szTime, blinkState ? "%02d.%02d" : "%02d %02d", now.hour(), now.minute());
      P.displayText(szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
    } else {
      sprintf(Time_S, "%02d", now.second());
      sprintf(szTime,  "%02d.%02d", now.hour(), now.minute());
      if (P.getZoneStatus(0)) {
        if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_3)
          P.displayZoneText(0, Time_S, PA_RIGHT, SPEED_TIME_SCROLLING_DOWN, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);
        else
          P.displayZoneText(0, Time_S, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
      }
      if (P.getZoneStatus(1))
        P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
    }
  } else {
    displaySettingMode();
  }
}

// ── displaySettingMode() ─────────────────────────────────────────────────────
void displaySettingMode() {
  if (currentMode == MODE_TIME_SETTING) {
    if (timeSubMode == TIME_SECOND && blinkState) sprintf(Time_S, "  ");
    else                                           sprintf(Time_S, "%02d", tempSecond);

    if      (timeSubMode == TIME_HOUR   && blinkState) sprintf(Time_H_M, "  :%02d", tempMinute);
    else if (timeSubMode == TIME_MINUTE && blinkState) sprintf(Time_H_M, "%02d:  ", tempHour);
    else                                               sprintf(Time_H_M, "%02d:%02d", tempHour, tempMinute);

    P.displayZoneText(0, Time_S,   PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayZoneText(1, Time_H_M, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    return;
  }

  if (currentMode == MODE_DATE_SETTING) {
    switch (dateSubMode) {
      case DATE_DAY:   sprintf(szTime, blinkState ? "  "   : "%02d", tempDay);   break;
      case DATE_MONTH: sprintf(szTime, blinkState ? "  "   : "%02d", tempMonth); break;
      case DATE_YEAR:  sprintf(szTime, blinkState ? "    " : "%d",   tempYear);  break;
    }
    P.begin();
    P.setIntensity(brightness);
    P.setFont(customFont5x7);
    P.displayText(szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  }
}

// ── loadCurrentTime() ────────────────────────────────────────────────────────
void loadCurrentTime() {
  if (!rtc.begin(&Wire)) {
    tempHour = 12; tempMinute = 0; tempSecond = 0;
    tempDay  = 1;  tempMonth  = 1; tempYear   = 2024;
    return;
  }
  DateTime now = rtc.now();
  tempHour   = now.hour();   tempMinute = now.minute(); tempSecond = now.second();
  tempDay    = now.day();    tempMonth  = now.month();  tempYear   = now.year();
}

// ── saveSettings() ───────────────────────────────────────────────────────────
void saveSettings() {
  rtc.adjust(DateTime(tempYear, tempMonth, tempDay, tempHour, tempMinute, tempSecond));
  Serial.printf("Saved: %02d:%02d:%02d  %02d/%02d/%04d\n",
                tempHour, tempMinute, tempSecond, tempDay, tempMonth, tempYear);
  char msg[] = "Settings Updated";
  scroll_Display(msg);
  set_Clock_Display();
  get_Time_and_Date();
  settingChanged = false;
}

// ── updateBrightness() ───────────────────────────────────────────────────────
void updateBrightness() {
  brightness = 6;
  P.setIntensity(brightness);
}

// ── setup() ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Clock — ESP32-C3 Super Mini");

  // INC / DEC buttons with pull-ups
  pinMode(BTN_INC_PIN, INPUT_PULLUP);
  pinMode(BTN_DEC_PIN, INPUT_PULLUP);

  // Encoder button also needs pull-up (encoder lib may not set it)
  pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  bool rtcOk = false;
  for (int i = 0; i < 3; i++) {
    if (rtc.begin(&Wire)) { rtcOk = true; break; }
    delay(500);
  }
  if (!rtcOk) Serial.println("RTC FAILED");

  P.begin();
  delay(100);
  P.setIntensity(brightness);

  bool state = false;
  for (byte i = 0; i < 4; i++) {
    state = !state;
    P.displayClear();
    P.setInvert(state);
    delay(500);
  }
  P.displayClear();

  set_Clock_Display();

  encoder.begin();

  if (rtcOk) {
    loadCurrentTime();
    DateTime now = rtc.now();
    sprintf(szTime, "%02d:%02d", now.hour(), now.minute());
  } else {
    strcpy(szTime, "00:00");
    tempHour = 12; tempMinute = 0; tempSecond = 0;
    tempDay  = 1;  tempMonth  = 1; tempYear   = 2024;
  }

  lastUpdate = blinkTimer = millis();
  updateBrightness();

  Serial.println("=== READY ===");
  Serial.println("Input mode : ENCODER (hold button 2s to switch)");
  Serial.println("INC=GPIO4  DEC=GPIO5");
}

// ── loop() ───────────────────────────────────────────────────────────────────
void loop() {
  delay(5);

  handleEncoder();   // encoder rotation (encoder mode) + encoder button (always)
  handleButtons();   // INC/DEC buttons (button mode only)

  if (currentMode == MODE_NORMAL) {
    P.displayAnimate();

    unsigned long now = millis();
    if (now - previousMillis >= 1000) {
      previousMillis = now;
      get_Time_and_Date();
      DateTime dt = rtc.now();
      if (dt.second() == 10) scroll_Display(str_Date);
      P.displayReset(0);
      P.displayReset(1);
    }
  } else {
    updateDisplay();
    P.displayAnimate();
  }
}
