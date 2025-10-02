// /*
//  * Offline Digital Clock - ESP32
//  * Encoder: A=25, B=26, Button=27
//  * MAX7219: CS=5, RTC: SDA=21, SCL=22
//  */

// #include <MD_Parola.h>
// #include <MD_MAX72xx.h>
// #include <SPI.h>
// #include <Wire.h>
// #include "RTClib.h"
// #include "Custom_Font_Data.h"
// #include "RotaryEncoder.h"

// // Hardware Configuration
// #define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// #define MAX_DEVICES 4
// #define CS_PIN    5

// #define ROTARY_ENCODER_A_PIN 25
// #define ROTARY_ENCODER_B_PIN 26
// #define ROTARY_ENCODER_BUTTON_PIN 27

// // Display Configuration - Using original style
// #define SPEED_TIME_SCROLLING_TEXT   50
// #define SPEED_TIME_SCROLLING_DOWN   80
// #define MAX_SCROLLING_TEXT_CHAR     30

// #define SPEED_TIME 75
// #define PAUSE_TIME 0
// #define MAX_MESG 20

// // Display modes
// #define MODE_1  1
// #define MODE_2  2
// #define MODE_3  3

// byte DIGITAL_CLOCK_DISPLAY_MODE = MODE_3; // Default to mode 3 as requested

// // Setting Modes
// typedef enum {
//   MODE_NORMAL = 0,
//   MODE_TIME_SETTING,
//   MODE_DATE_SETTING
// } DisplayMode;

// typedef enum {
//   TIME_HOUR = 0,
//   TIME_MINUTE,
//   TIME_SECOND
// } TimeSubMode;

// typedef enum {
//   DATE_DAY = 0,
//   DATE_MONTH,
//   DATE_YEAR
// } DateSubMode;

// // Global Objects - Using hardware SPI
// MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// RotaryEncoder encoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN);
// RTC_DS3231 rtc;

// // Global Variables
// DisplayMode currentMode = MODE_NORMAL;
// TimeSubMode timeSubMode = TIME_HOUR;
// DateSubMode dateSubMode = DATE_DAY;

// int brightness = 5;
// char szTime[9];
// char szSeconds[4];
// char szMesg[MAX_MESG+1] = "";

// // Time display strings for zones
// char Time_H[3];  // Hours
// char Time_M[3];  // Minutes
// char Time_S[3];  // Seconds

// // Timing variables
// unsigned long lastUpdate = 0;
// unsigned long blinkTimer = 0;
// unsigned long buttonPressStart = 0;
// bool blinkState = false;
// bool settingChanged = false;
// bool longPressDetected = false;

// // Temporary setting values
// int tempHour, tempMinute, tempSecond;
// int tempDay, tempMonth, tempYear;

// // Month and day names
// const char* monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
//                            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// const char* dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

// // Original style variables for display
// char Time_H_M[10];  // Hours and minutes combined
// char str_Date[50];  // Full date string for scrolling
// bool colon_Blink = false;

// // Full month names for scrolling display
// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// char month_Name[12][10] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// // Variables for Millis/Timer
// unsigned long previousMillis = 0;
// const long interval = 1000;

// //________________________________________________________________________________ set_Clock_Display()
// // Original display setup function
// void set_Clock_Display() {
//   // Conditions for displaying the clock in MODE_1.
//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
//     P.begin();
//     P.setIntensity(brightness);
//     P.setInvert(false);

//     P.setFont(0, customFont5x7);
//     P.displayText(Time_H_M, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
//   }

//   // Conditions for displaying the clock in MODE_2 and MODE_3.
//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
//     P.begin(2);
//     P.setInvert(false);

//     // Original zone configuration
//     P.setZone(0, 0, 0);    // Zone 0: Seconds (rightmost module)
//     P.setZone(1, 1, 3);    // Zone 1: Hours:Minutes (leftmost 3 modules)

//     P.setIntensity(brightness);

//     P.setFont(0, customFont3x5);  // Seconds font
//     P.setFont(1, customFont4x7);  // Time font

//     // Conditions for the "seconds" display mode in MODE_2.
//     if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2) P.displayZoneText(0, Time_S, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);

//     // Conditions for the "seconds" display mode in MODE_3.
//     if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) P.displayZoneText(0, Time_S, PA_RIGHT, SPEED_TIME_SCROLLING_DOWN, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);

//     // Displays "hours and minutes" on MODE_2 and MODE_3.
//     P.displayZoneText(1, Time_H_M, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
//   }
// }

// //________________________________________________________________________________ scroll_Display()
// // Original scrolling display function
// void scroll_Display(char szMesg[MAX_SCROLLING_TEXT_CHAR + 1]) {
//   P.begin();
//   P.setIntensity(brightness);
//   P.setInvert(false);
//   delay(1000);
//   P.displayScroll(szMesg, PA_RIGHT, PA_SCROLL_LEFT, SPEED_TIME_SCROLLING_TEXT);

//   while(true) {
//     if (P.displayAnimate()) {
//       P.displayReset();
//       set_Clock_Display();
//       delay(1000);
//       break;
//     }
//   }
// }

// //________________________________________________________________________________ get_Time_and_Date()
// // Original time and date function
// void get_Time_and_Date() {
//   DateTime now = rtc.now();

//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
//     colon_Blink = !colon_Blink;
//     sprintf(Time_H_M, "%02d %s %02d", now.hour(), (colon_Blink ? ":" : "^"), now.minute());
//   }

//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
//     sprintf(Time_H_M, "%02d:%02d", now.hour(), now.minute());  // Colon between hr:min (font should support this)
//   }

//   // Create data for seconds.
//   sprintf(Time_S, "%02d", now.second());

//   // Create data for dates with full scrolling format
//   sprintf(str_Date, "%s, %d %s %d", daysOfTheWeek[now.dayOfTheWeek()], now.day(), month_Name[now.month()-1], now.year());

//   // Debug output occasionally
//   static int debug_counter = 0;
//   if (debug_counter++ % 10 == 0) {
//     Serial.print("Time: ");
//     Serial.print(Time_H_M);
//     Serial.print(" Seconds: ");
//     Serial.println(Time_S);
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000); // Allow ESP32 to stabilize
//   Serial.println("Offline Digital Clock Starting...");

//   // Initialize I2C first for RTC
//   Serial.println("Starting I2C...");
// #ifdef ESP32
//   Wire.begin(21, 22); // ESP32 I2C pins
// #else
//   Wire.begin(); // Arduino default I2C pins
// #endif
//   delay(100);
//   Serial.println("I2C initialized");

//   // Initialize RTC first with error handling
//   Serial.println("Initializing RTC...");
//   bool rtcOk = false;
//   for (int i = 0; i < 3; i++) { // Try 3 times
//     if (rtc.begin()) {
//       Serial.println("RTC initialized successfully");
//       rtcOk = true;
//       break;
//     } else {
//       Serial.print("RTC init attempt ");
//       Serial.print(i + 1);
//       Serial.println(" failed");
//       delay(500);
//     }
//   }

//   if (!rtcOk) {
//     Serial.println("RTC initialization failed!");
//     strcpy(szMesg, "RTC ERROR");
//   }

//   // Initialize SPI for display
//   Serial.println("Starting SPI...");
//   SPI.begin();
//   delay(100);
//   Serial.println("SPI initialized");

//   // Initialize display using original style
//   Serial.println("Starting MAX7219 LED Matrix 32x8.");
//   P.begin();
//   delay(100);
//   P.begin();
//   delay(100);

//   P.setIntensity(brightness);

//   // All LEDs in the LED Matrix will turn on and off twice when the ESP32 is turned on.
//   bool State = false;
//   for (byte i = 0; i < 4; i++) {
//     State = !State;
//     P.displayClear();
//     P.setInvert(State);
//     delay(1000);
//   }
//   P.displayClear();

//   // Set up the clock display using original function
//   set_Clock_Display();

//   // Initialize encoder
//   Serial.println("Initializing rotary encoder...");
//   encoder.begin();
//   Serial.print("Encoder pins - A: ");
//   Serial.print(ROTARY_ENCODER_A_PIN);
//   Serial.print(", B: ");
//   Serial.print(ROTARY_ENCODER_B_PIN);
//   Serial.print(", Button: ");
//   Serial.println(ROTARY_ENCODER_BUTTON_PIN);

//   Serial.println("Brightness set to fixed level 6");

//   // Load current time safely
//   if (rtcOk) {
//     loadCurrentTime();
//     DateTime now = rtc.now();
//     sprintf(szTime, "%02d:%02d", now.hour(), now.minute());
//   } else {
//     strcpy(szTime, "00:00");
//   }

//   // Initialize timing variables
//   lastUpdate = millis();
//   blinkTimer = millis();

//   // Set fixed brightness
//   updateBrightness();

//   Serial.println("=== SETUP COMPLETE ===");
//   Serial.println("Display Mode: MODE_3 (Original style with date scrolling)");
//   Serial.println("Layout: Zone 1(1-3)=Hours.Minutes, Zone 0(0)=Seconds");
//   Serial.println("Display: [SS][HH.MM] - Seconds right, Time left with dot separator");
//   Serial.println("Date scrolls at 10 seconds every minute");
//   Serial.println("Brightness: Fixed at level 6");
//   Serial.println("Controls: Button=Mode change, Rotate=Adjust value, Long press=Save");
//   Serial.println("Date setting: Numeric format (D 01, M 02, YEAR)");
//   Serial.println("Messages: 'Time Change'/'Date Change' when entering modes");
//   Serial.println("Encoder: Direction REVERSED");
//   Serial.println("======================");
// }

// void loop() {
//   // Small delay for stability
//   delay(10);

//   handleEncoder();

//   // Only update normal display when not in setting modes
//   if (currentMode == MODE_NORMAL) {
//     // Displays or animates and returns true when an animation is completed.
//     P.displayAnimate();

//     // Original timer/millis to update the time and date with scrolling
//     unsigned long currentMillis = millis();
//     if (currentMillis - previousMillis >= interval) {
//       previousMillis = currentMillis;

//       get_Time_and_Date();

//       // Get current time for scrolling triggers
//       DateTime now = rtc.now();
//       byte t_second = now.second();

//       // The date will be displayed in the scrolling text if seconds equals 10
//       if (t_second == 10) scroll_Display(str_Date);

//       // Displays the next display after updates
//       P.displayReset(0);
//       P.displayReset(1);
//     }
//   } else {
//     // Handle setting modes display
//     updateDisplay();
//     P.displayAnimate();
//   }

//   // Yield to system
//   yield();
// }

// void updateBrightness() {
//   // Fixed brightness at level 6
//   brightness = 6;
//   P.setIntensity(brightness);
// }

// void handleEncoder() {
//   encoder.update();

//   // Handle encoder rotation first
//   if (encoder.hasChanged()) {
//     int currentValue = encoder.getValue();
//     settingChanged = true;

//     switch (currentMode) {
//       case MODE_TIME_SETTING:
//         switch (timeSubMode) {
//           case TIME_HOUR:
//             tempHour = currentValue;
//             Serial.print("Setting Hour: ");
//             Serial.println(tempHour);
//             break;
//           case TIME_MINUTE:
//             tempMinute = currentValue;
//             Serial.print("Setting Minute: ");
//             Serial.println(tempMinute);
//             break;
//           case TIME_SECOND:
//             tempSecond = currentValue;
//             Serial.print("Setting Second: ");
//             Serial.println(tempSecond);
//             break;
//         }
//         break;

//       case MODE_DATE_SETTING:
//         switch (dateSubMode) {
//           case DATE_DAY:
//             tempDay = currentValue;
//             Serial.print("Setting Day: ");
//             Serial.println(tempDay);
//             break;
//           case DATE_MONTH:
//             tempMonth = currentValue;
//             Serial.print("Setting Month: ");
//             Serial.println(tempMonth);
//             break;
//           case DATE_YEAR:
//             tempYear = currentValue;
//             Serial.print("Setting Year: ");
//             Serial.println(tempYear);
//             break;
//         }
//         break;
//     }
//   }

//   // Handle button press for mode switching
//   if (encoder.wasPressed()) {
//     buttonPressStart = millis();
//     longPressDetected = false;

//     switch (currentMode) {
//       case MODE_NORMAL: {
//         // Show "Time Change" message before entering time setting mode
//         char timeChangeMsg[] = "Time Change";
//         scroll_Display(timeChangeMsg);

//         // Enter time setting mode
//         currentMode = MODE_TIME_SETTING;
//         timeSubMode = TIME_HOUR;
//         loadCurrentTime();
//         encoder.setBoundaries(0, 23);
//         encoder.setValue(tempHour);
//         Serial.println("MODE: Time Setting - Hour");
//         break;
//       }

//       case MODE_TIME_SETTING: {
//         // Move through time sub-modes
//         if (timeSubMode == TIME_HOUR) {
//           timeSubMode = TIME_MINUTE;
//           encoder.setBoundaries(0, 59);
//           encoder.setValue(tempMinute);
//           Serial.println("MODE: Time Setting - Minute");
//         } else if (timeSubMode == TIME_MINUTE) {
//           timeSubMode = TIME_SECOND;
//           encoder.setBoundaries(0, 59);
//           encoder.setValue(tempSecond);
//           Serial.println("MODE: Time Setting - Second");
//         } else {
//           // Show "Date Change" message before entering date setting mode
//           char dateChangeMsg[] = "Date Change";
//           scroll_Display(dateChangeMsg);

//           // Show "Day" message before starting day setting
//           char dayChangeMsg[] = "Day";
//           scroll_Display(dayChangeMsg);

//           // Move to date setting
//           currentMode = MODE_DATE_SETTING;
//           dateSubMode = DATE_DAY;
//           encoder.setBoundaries(1, 31);
//           encoder.setValue(tempDay);
//           Serial.println("MODE: Date Setting - Day");
//         }
//         break;
//       }

//       case MODE_DATE_SETTING: {
//         // Move through date sub-modes with scroll messages for each transition
//         if (dateSubMode == DATE_DAY) {
//           // Show "Month" message before switching to month setting
//           char monthChangeMsg[] = "Month";
//           scroll_Display(monthChangeMsg);

//           dateSubMode = DATE_MONTH;
//           encoder.setBoundaries(1, 12);
//           encoder.setValue(tempMonth);
//           Serial.println("MODE: Date Setting - Month");
//         } else if (dateSubMode == DATE_MONTH) {
//           // Show "Year" message before switching to year setting
//           char yearChangeMsg[] = "Year";
//           scroll_Display(yearChangeMsg);

//           dateSubMode = DATE_YEAR;
//           encoder.setBoundaries(2020, 2050);
//           encoder.setValue(tempYear);
//           Serial.println("MODE: Date Setting - Year");
//         } else {
//           // Return to normal mode after year
//           saveSettings();
//           currentMode = MODE_NORMAL;
//           Serial.println("MODE: Normal - Settings saved");
//         }
//         break;
//       }
//     }
//   }

//   // Handle long press (5 seconds) for immediate save
//   if (encoder.isPressed()) {
//     if (millis() - buttonPressStart >= 5000 && !longPressDetected) {
//       longPressDetected = true;
//       if (currentMode != MODE_NORMAL) {
//         saveSettings();
//         currentMode = MODE_NORMAL;
//         Serial.println("Long press detected - Settings saved");
//       }
//     }
//   }

// }

// void updateDisplay() {
//   static unsigned long lastTimeUpdate = 0;

//   // Update time display every second in normal mode, faster in setting modes
//   unsigned long updateInterval = (currentMode == MODE_NORMAL) ? 1000 : 500;

//   if (millis() - lastTimeUpdate >= updateInterval) {
//     lastTimeUpdate = millis();

//     // Handle blinking for setting modes
//     if (millis() - blinkTimer >= 500) {
//       blinkTimer = millis();
//       blinkState = !blinkState;
//     }

//     if (currentMode == MODE_NORMAL) {
//       // Normal operation - display time based on mode
//       if (rtc.begin()) {
//         DateTime now = rtc.now();

//         if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
//           // MODE_1: Only hours and minutes with flashing dot
//           if (blinkState) {
//             sprintf(szTime, "%02d.%02d", now.hour(), now.minute());
//           } else {
//             sprintf(szTime, "%02d %02d", now.hour(), now.minute());
//           }
//           P.displayText(szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

//         } else if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
//           // MODE_2 and MODE_3: Multi-zone display with seconds
//           sprintf(Time_H, "%02d", now.hour());
//           sprintf(Time_M, "%02d", now.minute());
//           sprintf(Time_S, "%02d", now.second());

//           // Format hours and minutes together with single dot (like original)
//           sprintf(szTime, "%02d.%02d", now.hour(), now.minute());

//           // Debug output for MODE_3
//           static int lastSecond = -1;
//           if (now.second() != lastSecond) {
//             Serial.print("MODE_3 Display - Time: ");
//             Serial.print(szTime);
//             Serial.print(", Seconds: ");
//             Serial.println(Time_S);
//             lastSecond = now.second();
//           }

//           // Display in zones - Zone 0: seconds (rightmost), Zone 1: time (leftmost)
//           if (P.getZoneStatus(0)) {
//             if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
//               // MODE_3: Seconds with scroll-down effect in zone 0 (rightmost)
//               P.displayZoneText(0, Time_S, PA_RIGHT, SPEED_TIME_SCROLLING_DOWN, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);
//             } else {
//               // MODE_2: Seconds without effect in zone 0
//               P.displayZoneText(0, Time_S, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
//             }
//           }

//           if (P.getZoneStatus(1)) {
//             // Hours:Minutes in zone 1 (leftmost 3 modules)
//             P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
//           }
//         }
//       } else {
//         // RTC error - show error message
//         strcpy(szTime, "ERR");
//         P.displayText(szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
//       }

//     } else {
//       // Setting modes with blinking
//       displaySettingMode();

//       // Note: Both time and date setting modes handle their own zone display in displaySettingMode()
//       // No additional display calls needed here
//     }
//   }
// }

// void displaySettingMode() {
//   if (currentMode == MODE_TIME_SETTING) {
//     // Use original zone layout: Zone 0 (module 0) = seconds, Zone 1 (modules 1-3) = time

//     // Always show current seconds in Zone 0 (rightmost module)
//     if (timeSubMode == TIME_SECOND && blinkState) {
//       sprintf(Time_S, "  ");  // Blank seconds when blinking
//     } else {
//       sprintf(Time_S, "%02d", tempSecond);
//     }

//     // Handle hour/minute display with consistent 5-character width to prevent shifting
//     if (timeSubMode == TIME_HOUR && blinkState) {
//       // Use underscores or spaces for missing hour digits - exactly 5 chars: "__:MM"
//       sprintf(Time_H_M, "  :%02d", tempMinute);
//     } else if (timeSubMode == TIME_MINUTE && blinkState) {
//       // Use underscores or spaces for missing minute digits - exactly 5 chars: "HH:__"
//       sprintf(Time_H_M, "%02d:  ", tempHour);
//     } else {
//       // Normal display - exactly 5 chars: "HH:MM"
//       sprintf(Time_H_M, "%02d:%02d", tempHour, tempMinute);
//     }    // Display both zones using SAME alignment as normal mode to prevent shifting
//     P.displayZoneText(0, Time_S, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);  // Changed from PA_RIGHT to PA_CENTER
//     P.displayZoneText(1, Time_H_M, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
//     return;

//   } else if (currentMode == MODE_DATE_SETTING) {
//     // Display date setting - simple blinking numeric values
//     switch (dateSubMode) {
//       case DATE_DAY:
//         if (blinkState) {
//           sprintf(szTime, "  ");  // Blank when blinking
//         } else {
//           sprintf(szTime, "%02d", tempDay);  // Just the number
//         }
//         break;
//       case DATE_MONTH:
//         if (blinkState) {
//           sprintf(szTime, "  ");  // Blank when blinking
//         } else {
//           sprintf(szTime, "%02d", tempMonth);  // Just the number
//         }
//         break;
//       case DATE_YEAR:
//         if (blinkState) {
//           sprintf(szTime, "    ");  // Blank when blinking (4 digits)
//         } else {
//           sprintf(szTime, "%d", tempYear);  // Just the number
//         }
//         break;
//     }

//     // Use full display (single zone) for date setting
//     P.begin();  // Reset to single zone
//     P.setIntensity(brightness);
//     P.setFont(customFont5x7);
//     P.displayText(szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
//     return;
//   }
// }

// void loadCurrentTime() {
//   if (!rtc.begin()) {
//     Serial.println("RTC not available for time load");
//     // Set default values
//     tempHour = 12;
//     tempMinute = 0;
//     tempSecond = 0;
//     tempDay = 1;
//     tempMonth = 1;
//     tempYear = 2024;
//     return;
//   }

//   DateTime now = rtc.now();
//   tempHour = now.hour();
//   tempMinute = now.minute();
//   tempSecond = now.second();
//   tempDay = now.day();
//   tempMonth = now.month();
//   tempYear = now.year();
// }

// void saveSettings() {
//   rtc.adjust(DateTime(tempYear, tempMonth, tempDay, tempHour, tempMinute, tempSecond));

//   Serial.println("Time and date saved to RTC");
//   Serial.print("Time: "); Serial.print(tempHour); Serial.print(":");
//   Serial.print(tempMinute); Serial.print(":"); Serial.println(tempSecond);
//   Serial.print("Date: "); Serial.print(tempDay); Serial.print("/");
//   Serial.print(tempMonth); Serial.print("/"); Serial.println(tempYear);

//   // Show "Settings Updated" message for 1 second
//   char updateMsg[] = "Settings Updated";
//   Serial.println("Showing update message...");
//   scroll_Display(updateMsg);

//   // Reinitialize display to normal clock mode
//   set_Clock_Display();

//   // Make sure we update the time display immediately
//   get_Time_and_Date();

//   Serial.println("Display reinitialized to normal mode");
//   Serial.print("Current time display: ");
//   Serial.println(Time_H_M);

//   settingChanged = false;
// }