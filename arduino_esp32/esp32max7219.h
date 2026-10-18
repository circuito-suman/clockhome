#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Custom_Font_Data.h"
#include <Wire.h>
#include <Preferences.h>
#include "PageIndex.h"
#include <NTPClient.h>
#include <WiFiUdp.h>  // Required by NTPClient
//----------------------------------------

// Defines the MAX7219 LED Matrix hardware type.
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// Defines the number of devices and CS PIN.
#define MAX_DEVICES 4
#define CS_PIN      5

// Defines the scrolling text speed, the scrolling seconds effect speed from top to bottom (in MODE_3) and the maximum number of characters for scrolling text.
#define SPEED_TIME_SCROLLING_TEXT   50
#define SPEED_TIME_SCROLLING_DOWN   80
#define MAX_SCROLLING_TEXT_CHAR     30

// Defines the MODE for displaying the clock.
// - MODE_1 : Only displays hours and minutes. The colon between the hours and minutes will flash to indicate changing seconds.
// - MODE_2 : Displays hours, minutes and seconds.
// - MODE_3 : Displays hours, minutes and seconds. The seconds display uses a “scroll down” effect every second change.
#define MODE_1  1
#define MODE_2  2
#define MODE_3  3

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", 19800, 60000); // 19800 seconds offset for India
//----------------------------------------Variable declaration for your network credentials.
const char* ssid = "real";  //--> Your wifi name
const char* password = "suman saha"; //--> Your wifi password
//----------------------------------------

//----------------------------------------Defining the key.
// "Key" functions like a password. In order to change the text on the MAX7219 LED Matrix 32x8, the user must know the "key".
// You can change it to another word.
#define key_Txt "12345678"
//----------------------------------------

// Variable used to select the clock display mode.
byte DIGITAL_CLOCK_DISPLAY_MODE;

// Variable to hold the text that will be displayed in the scrolling text.
char str_ScrollingText_Msg[35] = "";

// Char array variable "Time_H_M" to hold the hour and minute values. Char array variable "Time_S" to hold the seconds value.
char Time_H_M[10];
char Time_S[3];

// Variables for time and date. This variable is used when setting the time and date.
byte t_hour;
byte t_minute;
byte t_second;
byte d_day;
byte d_month;
int d_year;

// Char array variable to hold day and date data.
char str_Date[50];

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char month_Name[12][10] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// Variables for displaying or printing the time and date on a serial monitor or serial communication.
char str_PrintFull_DateTime[60];

// Variables for Millis/Timer.
unsigned long previousMillis = 0;
const long interval = 1000;

// Variable to set the LED Matrix brightness intensity value.
// Range from 0 to 15.
byte brightness_Intensity = 0;

bool colon_Blink = false;

// Initialize MD_Parola as Display and its settings.
MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);


// Initialize the "Preferences" library as "preferences".
Preferences preferences;

// Server on port 80.
WebServer server(80);

//________________________________________________________________________________ handleRoot()
// This routine is executed when you open ESP32 IP Address in browser.
void handleRoot() {
  server.send(200, "text/html", MAIN_page); //Send web page
}
//________________________________________________________________________________

//________________________________________________________________________________ handleSettings().
// Subroutine to handle settings. The displayed text are set here.
void handleSettings() {
  String incoming_Settings;
  int incoming_Settings_Length;

  incoming_Settings = server.arg("key");

  Serial.println();
  Serial.println("------------Settings");
  Serial.print("Key : ");
  Serial.println(incoming_Settings);

  //---------------------------------------- Conditions for checking keys.
  if (incoming_Settings == key_Txt) {
    incoming_Settings = server.arg("sta");

    //........................................ Conditions for setting the time and date.
    if (incoming_Settings == "setDT") {
      Serial.println();
      Serial.println("Set Time and Date.");

      incoming_Settings = server.arg("dt_year");
      d_year = incoming_Settings.toInt();

      incoming_Settings = server.arg("dt_month");
      d_month = incoming_Settings.toInt();

      incoming_Settings = server.arg("dt_day");
      d_day = incoming_Settings.toInt();

      incoming_Settings = server.arg("tm_hour");
      t_hour = incoming_Settings.toInt();

      incoming_Settings = server.arg("tm_minute");
      t_minute = incoming_Settings.toInt();

      incoming_Settings = server.arg("tm_second");
      t_second = incoming_Settings.toInt();

      delay(10);

      Serial.println();
      Serial.print("Date : ");
      Serial.print(d_day);Serial.print("-");Serial.print(d_month);Serial.print("-");Serial.print(d_year);
      Serial.print(" | Time : ");
      Serial.print(t_hour);Serial.print(":");Serial.print(t_minute);Serial.print(":");Serial.println(t_second);
      Serial.println("------------");


      set_Clock_Display();

      server.send(200, "text/plane", "+OK_setDT");  //--> Sending replies to the client.
      delay(500);
    }
    //........................................

    //........................................ Conditions for sending settings to a web page or client.
    if (incoming_Settings == "getSettings") {
      Serial.println();
      Serial.println("Get Settings");

      char send_Settings[50];
      sprintf(send_Settings, "gs|%d|%d|%s", brightness_Intensity, DIGITAL_CLOCK_DISPLAY_MODE, str_ScrollingText_Msg);

      Serial.println(send_Settings);
      Serial.println("------------");

      server.send(200, "text/plane", send_Settings);  //--> Sending replies to the client.
      delay(500);
    }
    //........................................

    //........................................ Conditions for setting brightness intensity.
    if (incoming_Settings == "setBrt") {
      Serial.println();
      Serial.println("Set the Brightness Intensity.");

      incoming_Settings = server.arg("brt");
      brightness_Intensity = incoming_Settings.toInt();

      delay(10);

      Serial.println();
      Serial.print("Brightness Intensity from Client : ");
      Serial.println(brightness_Intensity);

      //######################################## Save and read the brightness intensity value to or from flash memory.
      // Open Preferences with "mySettings" namespace.
      preferences.begin("mySettings", false);
      delay(10);

      // Save the brightness_Intensity value.
      preferences.putInt("brt_Intensity", brightness_Intensity);
      delay(10);

      // Get the brightness_Intensity value, if the key does not exist, return a default value of 0.
      brightness_Intensity = preferences.getInt("brt_Intensity", 0);
      delay(10);

      Serial.println();
      Serial.print("Saved Brightness Intensity : ");
      Serial.println(brightness_Intensity);
      Serial.println("------------");

      // Close the Preferences.
      preferences.end();
      delay(10);
      //########################################

      set_Clock_Display();

      server.send(200, "text/plane", "+OK_setBrt");  //--> Sending replies to the client.
      delay(500);
    }
    //........................................

    //........................................ Conditions for setting the clock display mode.
    if (incoming_Settings == "setCDM") {
      Serial.println();
      Serial.println("Set the clock display mode.");

      incoming_Settings = server.arg("cdm");
      DIGITAL_CLOCK_DISPLAY_MODE = incoming_Settings.toInt();

      delay(100);

      Serial.println();
      Serial.print("Clock Display Modes from Client : MODE_");
      Serial.println(DIGITAL_CLOCK_DISPLAY_MODE);

      //######################################## Save and read "clock display mode" to or from flash memory.
      // Open Preferences with "mySettings" namespace.
      preferences.begin("mySettings", false);
      delay(10);

      // Save the DIGITAL_CLOCK_DISPLAY_MODE value.
      preferences.putInt("cd_Mode", DIGITAL_CLOCK_DISPLAY_MODE);
      delay(10);

      // Get the DIGITAL_CLOCK_DISPLAY_MODE value, if the key does not exist, return a default value of 1.
      DIGITAL_CLOCK_DISPLAY_MODE = preferences.getInt("cd_Mode", 1);
      delay(10);

      Serial.println();
      Serial.print("Saved Display Modes : MODE_");
      Serial.println(DIGITAL_CLOCK_DISPLAY_MODE);
      Serial.println("------------");

      // Close the Preferences.
      preferences.end();
      delay(10);
      //########################################

      set_Clock_Display();

      server.send(200, "text/plane", "+OK_setCDM");  //--> Sending replies to the client.
      delay(500);
    }
    //........................................

    //........................................ Conditions for setting messages on scrolling text.
    if (incoming_Settings == "setMsg") {
      Serial.println();
      Serial.println("Set Messages On Scrolling Text.");

      incoming_Settings = server.arg("msg");
      incoming_Settings_Length = incoming_Settings.length() + 1;
      incoming_Settings.toCharArray(str_ScrollingText_Msg, incoming_Settings_Length);

      delay(100);

      Serial.println();
      Serial.print("Message On Scrolling Text from Client : ");
      Serial.println(str_ScrollingText_Msg);

      //######################################## Save and read the "message text" which will be displayed in scrolling text to or from flash memory.
      // Open Preferences with "mySettings" namespace.
      preferences.begin("mySettings", false);
      delay(10);

      // Save the str_ScrollingText_Msg value.
      preferences.putString("Msg_ST", str_ScrollingText_Msg);
      delay(10);

      // Get the str_ScrollingText_Msg value, if the key does not exist, return a default value of "".
      String saved_Msg = "";
      int saved_Msg_Length;
      saved_Msg = preferences.getString("Msg_ST", "");
      saved_Msg_Length = saved_Msg.length() + 1;
      saved_Msg.toCharArray(str_ScrollingText_Msg, saved_Msg_Length);
      delay(10);

      Serial.println();
      Serial.print("Saved Message On Scrolling Text : ");
      Serial.println(str_ScrollingText_Msg);
      Serial.println("------------");

      // Close the Preferences.
      preferences.end();
      delay(10);
      //########################################

      server.send(200, "text/plane", "+OK_setMsg");  //--> Sending replies to the client.
      delay(500);
    }
    //........................................

  } else {
    Serial.println();
    Serial.println("Wrong Key Text !");
    Serial.println("Please enter the correct Key Text.");
    Serial.println("------------");

    server.send(200, "text/plane", "+ERR"); //--> Sending replies to the client.
    delay(500);
  }
  //----------------------------------------
}
//________________________________________________________________________________

//________________________________________________________________________________ set_Clock_Display()
// Subroutine for setting the clock display mode.
void set_Clock_Display() {
  // Conditions for displaying the clock in MODE_1.
  if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
    Display.begin();
    Display.setIntensity(brightness_Intensity);
    Display.setInvert(false);

    // customFont5x7 to display seconds. This font only contains the characters:
    // - Space.
    // - 0 to 9.
    // - : (colon).
    // - ^ (I changed the "^" character to "two pixel space". For explanation, see "void get_Time_and_Date()".).
    Display.setFont(0, customFont5x7);

    Display.displayText(Time_H_M, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }

  // Conditions for displaying the clock in MODE_2 and MODE_3.
  if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
    Display.begin(2);
    Display.setInvert(false);

    // In MODE_2 and MODE_3 to display hours, minutes and seconds on the LED Matrix using "zones".
    // For more details about the "zones" in the LED Matrix, please see here : https://arduinoplusplus.wordpress.com/2017/04/18/parola-a-to-z-multi-zone-displays/
    Display.setZone(0, 0, 0);
    Display.setZone(1, 1, 3);

    Display.setIntensity(brightness_Intensity);

    // customFont3x5 to display "seconds". This font only contains the characters :
    // - Space.
    // - 0 to 9.
    Display.setFont(0, customFont3x5);

    // customFont4x7 to display "hours and minutes". This font only contains the characters :
    // - Space.
    // - 0 to 9.
    // - : (colon).
    Display.setFont(1, customFont4x7);

    // Conditions for the "seconds" display mode in MODE_2.
    if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2) Display.displayZoneText(0, Time_S, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);

    // Conditions for the "seconds" display mode in MODE_3.
    if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) Display.displayZoneText(0, Time_S, PA_RIGHT, SPEED_TIME_SCROLLING_DOWN, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);

    // Displays "hours and minutes" on MODE_2 and MODE_3.
    Display.displayZoneText(1, Time_H_M, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }
}
//________________________________________________________________________________

//________________________________________________________________________________ scroll_Display(char szMesg[MAX_SCROLLING_TEXT_CHAR + 1])
// Subroutine for displaying text on scrolling text.
// Scrolling text uses built-in fonts from the library.
void scroll_Display(char szMesg[MAX_SCROLLING_TEXT_CHAR + 1]) {
  Display.begin();
  Display.setIntensity(brightness_Intensity);
  Display.setInvert(false);
  delay(1000);
  Display.displayScroll(szMesg, PA_RIGHT, PA_SCROLL_LEFT, SPEED_TIME_SCROLLING_TEXT);

  while(true) {
    if (Display.displayAnimate()) {
      Display.displayReset();
      set_Clock_Display();
      delay(1000);
      break;
    }
  }
}
//________________________________________________________________________________

//________________________________________________________________________________ get_Time_and_Date()
// Subroutine to get time and date data from DS3231 rtc module.
void get_Time_and_Date() {
  timeClient.update();

  // ✅ Correct IST time directly from NTPClient
  t_hour   = timeClient.getHours();
  t_minute = timeClient.getMinutes();
  t_second = timeClient.getSeconds();

  // ✅ Correct IST date
  unsigned long epochTime = timeClient.getEpochTime();
  time_t rawTime = (time_t)epochTime;
  struct tm ptm;
  localtime_r(&rawTime, &ptm);

  byte dayOfWeek = ptm.tm_wday;  // Sunday = 0, Monday = 1, etc.

  if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
    colon_Blink = !colon_Blink;
    sprintf(Time_H_M, "%02d %s %02d", t_hour, (colon_Blink ? ":" : "^"), t_minute);
  } else {
    sprintf(Time_H_M, "%02d:%02d", t_hour, t_minute);
  }
  sprintf(Time_S, "%02d", t_second);

  sprintf(str_Date, "%s, %d %s %d",
          daysOfTheWeek[dayOfWeek],
          ptm.tm_mday,
          month_Name[ptm.tm_mon],
          ptm.tm_year + 1900);

  sprintf(str_PrintFull_DateTime,
          "Date : %s, %d %s %d | Time : %02d:%02d:%02d",
          daysOfTheWeek[dayOfWeek],
          ptm.tm_mday,
          month_Name[ptm.tm_mon],
          ptm.tm_year + 1900,
          t_hour, t_minute, t_second);

  Serial.println();
  Serial.println(str_PrintFull_DateTime);
}


//________________________________________________________________________________

//________________________________________________________________________________ VOID SETUP()
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  //---------------------------------------- Reads settings stored in flash memory.
  // Open Preferences with "mySettings" namespace.
  preferences.begin("mySettings", false);
  delay(10);

  // Get the brightness_Intensity value, if the key does not exist, return a default value of 0.
  brightness_Intensity = preferences.getInt("brt_Intensity", 0);
  delay(10);

  // Get the DIGITAL_CLOCK_DISPLAY_MODE value, if the key does not exist, return a default value of 1.
  DIGITAL_CLOCK_DISPLAY_MODE = preferences.getInt("cd_Mode", 1);
  delay(10);

  // Get the str_ScrollingText_Msg value, if the key does not exist, return a default value of "".
  String saved_Msg = "";
  int saved_Msg_Length;
  saved_Msg = preferences.getString("Msg_ST", "");
  saved_Msg_Length = saved_Msg.length() + 1;
  saved_Msg.toCharArray(str_ScrollingText_Msg, saved_Msg_Length);
  delay(10);

  // Close the Preferences.
  preferences.end();
  delay(10);

  Serial.println();
  Serial.println("------------Saved settings.");
  Serial.print("Brightness Intensity : ");
  Serial.println(brightness_Intensity);
  Serial.print("Clock Display Mode : MODE_");
  Serial.println(DIGITAL_CLOCK_DISPLAY_MODE);
  Serial.print("Message on Scrolling Text : ");
  Serial.println(str_ScrollingText_Msg);
  Serial.println("------------");
  //----------------------------------------

  delay(100);

  //---------------------------------------- Set Wifi to STA mode.
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  WiFi.mode(WIFI_STA);
  Serial.println("-------------");
  //----------------------------------------

  delay(100);

  //---------------------------------------- Connect to Wi-Fi (STA).
  Serial.println();
  Serial.println("------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  //:::::::::::::::::: The process of connecting ESP32 with WiFi Hotspot / WiFi Router.
  // The process timeout of connecting ESP32 with WiFi Hotspot / WiFi Router is 20 seconds.
  // If within 20 seconds the ESP32 has not been successfully connected to WiFi, the ESP32 will restart.
  // I made this condition because on my ESP32, there are times when it seems like it can't connect to WiFi, so it needs to be restarted to be able to connect to WiFi.

  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      Serial.println();
      Serial.println("Failed to connect to WiFi. The ESP32 will be restarted.");
      Serial.println("------------");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.println("------------");
  //::::::::::::::::::
  //----------------------------------------

  delay(100);


  timeClient.begin();


  //---------------------------------------- Setting the server.
  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  //----------------------------------------

  delay(100);

  //---------------------------------------- Start server.
  server.begin();
  Serial.println();
  Serial.println("HTTP server started");
  //----------------------------------------

  delay(100);

  //---------------------------------------- Starting MAX7219 LED Matrix 32x8.
  Serial.println();
  Serial.println("Starting MAX7219 LED Matrix 32x8.");
  Display.begin();
  delay(100);
  Display.begin();
  delay(100);
  //----------------------------------------

  Display.setIntensity(brightness_Intensity);

  //---------------------------------------- All LEDs in the LED Matrix will turn on and off twice when the ESP32 is turned on.
  bool State = false;
  for (byte i = 0; i < 4; i++) {
    State = !State;
    Display.displayClear();
    Display.setInvert(State);
    delay(1000);
  }
  Display.displayClear();
  //----------------------------------------

  //---------------------------------------- Starting the DS3231 RTC module.
  Serial.println();

  //----------------------------------------

  // Open the IP address in your browser to open the interface page.
  // Make sure that your computer / mobile device and ESP32 are connected to the same WiFi network.
  Serial.println();
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());
  Serial.println("Open the IP address in your browser to open the interface page.");
  Serial.println("Make sure that your computer / mobile device and");
  Serial.println("ESP32 are connected to the same WiFi network.");

  delay(500);

  set_Clock_Display();
}
//________________________________________________________________________________

//________________________________________________________________________________ VOID LOOP()
void loop() {
  // put your main code here, to run repeatedly:

  // Handle client requests.
  server.handleClient();

  // Displays or animates and returns true when an animation is completed.
  Display.displayAnimate();

  //---------------------------------------- Timer/Millis to update the time and date. And also execute other conditions.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    get_Time_and_Date();

    // The date will be displayed in the scrolling text if seconds equals 10 or at the 10th of the second.
    if (t_second == 10) scroll_Display(str_Date);

    // The message will be displayed in the scrolling text if seconds equals 40 or at the 40th second.
    if (t_second == 40 && strlen(str_ScrollingText_Msg) > 0) {
      scroll_Display(str_ScrollingText_Msg);
    }

    // Displays the next display.
    Display.displayReset(0);
    Display.displayReset(1);
  }
  //----------------------------------------
}
//________________________________________________________________________________
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// Data file for user example user defined fonts
#ifndef FONTDATA_H
#define FONTDATA_H

MD_MAX72XX::fontType_t customFont5x7[] PROGMEM = 
{
  0,  // 0
  0,  // 1
  0,  // 2
  0,  // 3
  0,  // 4
  0,  // 5
  0,  // 6
  0,  // 7
  0,  // 8
  0,  // 9
  0,  // 10
  0,  // 11
  0,  // 12
  0,  // 13
  0,  // 14
  0,  // 15
  0,  // 16
  0,  // 17
  0,  // 18
  0,  // 19
  0,  // 20
  0,  // 21
  0,  // 22
  0,  // 23
  0,  // 24
  0,  // 25
  0,  // 26
  0,  // 27
  0,  // 28
  0,  // 29
  0,  // 30
  0,  // 31
  1, 0,   // 32 Space
  0,  // 33
  0,  // 34
  0,  // 35
  0,  // 36
  0,  // 37
  0,  // 38
  0,  // 39
  0,  // 40
  0,  // 41
  0,  // 42
  0,  // 43
  0,  // 44
  0,  // 45
  0,  // 46
  0,  // 47
  5, 62, 65, 65, 65, 62,    // 48 0
  5, 0, 66, 127, 64, 0,     // 49  1
  5, 66, 97, 81, 73, 70,    // 50  2
  5, 33, 65, 69, 75, 49,    // 51  3
  5, 24, 20, 18, 127, 16,   // 52  4
  5, 79, 73, 73, 73, 49,    // 53  5
  5, 60, 74, 73, 73, 48,    // 54  6
  5, 1, 1, 113, 9, 7,       // 55  7
  5, 54, 73, 73, 73, 54,    // 56  8
  5, 6, 73, 73, 41, 30,     // 57  9
  2, 54, 54,                // 58  :
  0,  // 59
  0,  // 60
  0,  // 61
  0,  // 62
  0,  // 63
  0,  // 64
  0,  // 65
  0,  // 66
  0,  // 67
  0,  // 68
  0,  // 69
  0,  // 70
  0,  // 71
  0,  // 72
  0,  // 73
  0,  // 74
  0,  // 75
  0,  // 76
  0,  // 77
  0,  // 78
  0,  // 79
  0,  // 80
  0,  // 81
  0,  // 82
  0,  // 83
  0,  // 84
  0,  // 85
  0,  // 86
  0,  // 87
  0,  // 88
  0,  // 89
  0,  // 90
  0,  // 91
  0,  // 92
  0,  // 93
  2, 0, 0,   // 94  ^
  0,  // 95
  0,  // 96
  0,  // 97
  0,  // 98
  0,  // 99
  0,  // 100
  0,  // 101
  0,  // 102
  0,  // 103
  0,  // 104
  0,  // 105
  0,  // 106
  0,  // 107
  0,  // 108
  0,  // 109
  0,  // 110
  0,  // 111
  0,  // 112
  0,  // 113
  0,  // 114
  0,  // 115
  0,  // 116
  0,  // 117
  0,  // 118
  0,  // 119
  0,  // 120
  0,  // 121
  0,  // 122
  0,  // 123
  0,  // 124
  0,  // 125
  0,  // 126
  0,  // 127
  0,  // 128
  0,  // 129
  0,  // 130
  0,  // 131
  0,  // 132
  0,  // 133
  0,  // 134
  0,  // 135
  0,  // 136
  0,  // 137
  0,  // 138
  0,  // 139
  0,  // 140
  0,  // 141
  0,  // 142
  0,  // 143
  0,  // 144
  0,  // 145
  0,  // 146
  0,  // 147
  0,  // 148
  0,  // 149
  0,  // 150
  0,  // 151
  0,  // 152
  0,  // 153
  0,  // 154
  0,  // 155
  0,  // 156
  0,  // 157
  0,  // 158
  0,  // 159
  0,  // 160
  0,  // 161
  0,  // 162
  0,  // 163
  0,  // 164
  0,  // 165
  0,  // 166
  0,  // 167
  0,  // 168
  0,  // 169
  0,  // 170
  0,  // 171
  0,  // 172
  0,  // 173
  0,  // 174
  0,  // 175
  0,  // 176
  0,  // 177
  0,  // 178
  0,  // 179
  0,  // 180
  0,  // 181
  0,  // 182
  0,  // 183
  0,  // 184
  0,  // 185
  0,  // 186
  0,  // 187
  0,  // 188
  0,  // 189
  0,  // 190
  0,  // 191
  0,  // 192
  0,  // 193
  0,  // 194
  0,  // 195
  0,  // 196
  0,  // 197
  0,  // 198
  0,  // 199
  0,  // 200
  0,  // 201
  0,  // 202
  0,  // 203
  0,  // 204
  0,  // 205
  0,  // 206
  0,  // 207
  0,  // 208
  0,  // 209
  0,  // 210
  0,  // 211
  0,  // 212
  0,  // 213
  0,  // 214
  0,  // 215
  0,  // 216
  0,  // 217
  0,  // 218
  0,  // 219
  0,  // 220
  0,  // 221
  0,  // 222
  0,  // 223
  0,  // 224
  0,  // 225
  0,  // 226
  0,  // 227
  0,  // 228
  0,  // 229
  0,  // 230
  0,  // 231
  0,  // 232
  0,  // 233
  0,  // 234
  0,  // 235
  0,  // 236
  0,  // 237
  0,  // 238
  0,  // 239
  0,  // 240
  0,  // 241
  0,  // 242
  0,  // 243
  0,  // 244
  0,  // 245
  0,  // 246
  0,  // 247
  0,  // 248
  0,  // 249
  0,  // 250
  0,  // 251
  0,  // 252
  0,  // 253
  0,  // 254
  0,  // 255
};

MD_MAX72XX::fontType_t customFont4x7[] PROGMEM = 
{
  0,  // 0
  0,  // 1
  0,  // 2
  0,  // 3
  0,  // 4
  0,  // 5
  0,  // 6
  0,  // 7
  0,  // 8
  0,  // 9
  0,  // 10
  0,  // 11
  0,  // 12
  0,  // 13
  0,  // 14
  0,  // 15
  0,  // 16
  0,  // 17
  0,  // 18
  0,  // 19
  0,  // 20
  0,  // 21
  0,  // 22
  0,  // 23
  0,  // 24
  0,  // 25
  0,  // 26
  0,  // 27
  0,  // 28
  0,  // 29
  0,  // 30
  0,  // 31
  1, 0,   // 32 Space
  0,  // 33
  0,  // 34
  0,  // 35
  0,  // 36
  0,  // 37
  0,  // 38
  0,  // 39
  0,  // 40
  0,  // 41
  0,  // 42
  0,  // 43
  0,  // 44
  0,  // 45
  0,  // 46
  0,  // 47
  4, 62, 65, 65, 62,  // 48  0
  4, 0, 66, 127, 64,  // 49  1
  4, 98, 81, 73, 70,  // 50  2
  4, 65, 73, 73, 54,  // 51  3
  4, 7, 8, 8, 127,    // 52  4
  4, 79, 73, 73, 49,  // 53  5
  4, 60, 74, 73, 49,  // 54  6
  4, 1, 97, 25, 7,    // 55  7
  4, 54, 73, 73, 54,  // 56  8
  4, 70, 73, 41, 30,  // 57  9
  3, 0, 34, 0,        // 58  :
  0,  // 59
  0,  // 60
  0,  // 61
  0,  // 62
  0,  // 63
  0,  // 64
  0,  // 65
  0,  // 66
  0,  // 67
  0,  // 68
  0,  // 69
  0,  // 70
  0,  // 71
  0,  // 72
  0,  // 73
  0,  // 74
  0,  // 75
  0,  // 76
  0,  // 77
  0,  // 78
  0,  // 79
  0,  // 80
  0,  // 81
  0,  // 82
  0,  // 83
  0,  // 84
  0,  // 85
  0,  // 86
  0,  // 87
  0,  // 88
  0,  // 89
  0,  // 90
  0,  // 91
  0,  // 92
  0,  // 93
  0,  // 94
  0,  // 95
  0,  // 96
  0,  // 97
  0,  // 98
  0,  // 99
  0,  // 100
  0,  // 101
  0,  // 102
  0,  // 103
  0,  // 104
  0,  // 105
  0,  // 106
  0,  // 107
  0,  // 108
  0,  // 109
  0,  // 110
  0,  // 111
  0,  // 112
  0,  // 113
  0,  // 114
  0,  // 115
  0,  // 116
  0,  // 117
  0,  // 118
  0,  // 119
  0,  // 120
  0,  // 121
  0,  // 122
  0,  // 123
  0,  // 124
  0,  // 125
  0,  // 126
  0,  // 127
  0,  // 128
  0,  // 129
  0,  // 130
  0,  // 131
  0,  // 132
  0,  // 133
  0,  // 134
  0,  // 135
  0,  // 136
  0,  // 137
  0,  // 138
  0,  // 139
  0,  // 140
  0,  // 141
  0,  // 142
  0,  // 143
  0,  // 144
  0,  // 145
  0,  // 146
  0,  // 147
  0,  // 148
  0,  // 149
  0,  // 150
  0,  // 151
  0,  // 152
  0,  // 153
  0,  // 154
  0,  // 155
  0,  // 156
  0,  // 157
  0,  // 158
  0,  // 159
  0,  // 160
  0,  // 161
  0,  // 162
  0,  // 163
  0,  // 164
  0,  // 165
  0,  // 166
  0,  // 167
  0,  // 168
  0,  // 169
  0,  // 170
  0,  // 171
  0,  // 172
  0,  // 173
  0,  // 174
  0,  // 175
  0,  // 176
  0,  // 177
  0,  // 178
  0,  // 179
  0,  // 180
  0,  // 181
  0,  // 182
  0,  // 183
  0,  // 184
  0,  // 185
  0,  // 186
  0,  // 187
  0,  // 188
  0,  // 189
  0,  // 190
  0,  // 191
  0,  // 192
  0,  // 193
  0,  // 194
  0,  // 195
  0,  // 196
  0,  // 197
  0,  // 198
  0,  // 199
  0,  // 200
  0,  // 201
  0,  // 202
  0,  // 203
  0,  // 204
  0,  // 205
  0,  // 206
  0,  // 207
  0,  // 208
  0,  // 209
  0,  // 210
  0,  // 211
  0,  // 212
  0,  // 213
  0,  // 214
  0,  // 215
  0,  // 216
  0,  // 217
  0,  // 218
  0,  // 219
  0,  // 220
  0,  // 221
  0,  // 222
  0,  // 223
  0,  // 224
  0,  // 225
  0,  // 226
  0,  // 227
  0,  // 228
  0,  // 229
  0,  // 230
  0,  // 231
  0,  // 232
  0,  // 233
  0,  // 234
  0,  // 235
  0,  // 236
  0,  // 237
  0,  // 238
  0,  // 239
  0,  // 240
  0,  // 241
  0,  // 242
  0,  // 243
  0,  // 244
  0,  // 245
  0,  // 246
  0,  // 247
  0,  // 248
  0,  // 249
  0,  // 250
  0,  // 251
  0,  // 252
  0,  // 253
  0,  // 254
  0,  // 255
};

MD_MAX72XX::fontType_t customFont3x5[] PROGMEM = 
{
  0,  // 0
  0,  // 1
  0,  // 2
  0,  // 3
  0,  // 4
  0,  // 5
  0,  // 6
  0,  // 7
  0,  // 8
  0,  // 9
  0,  // 10
  0,  // 11
  0,  // 12
  0,  // 13
  0,  // 14
  0,  // 15
  0,  // 16
  0,  // 17
  0,  // 18
  0,  // 19
  0,  // 20
  0,  // 21
  0,  // 22
  0,  // 23
  0,  // 24
  0,  // 25
  0,  // 26
  0,  // 27
  0,  // 28
  0,  // 29
  0,  // 30
  0,  // 31
  1, 0,   // 32 space
  0,  // 33
  0,  // 34
  0,  // 35
  0,  // 36
  0,  // 37
  0,  // 38
  0,  // 39
  0,  // 40
  0,  // 41
  0,  // 42
  0,  // 43
  0,  // 44
  0,  // 45
  0,  // 46
  0,  // 47
  3, 31, 17, 31,  // 48  0
  3, 18, 31, 16,  // 49  1
  3, 29, 21, 23,  // 50  2
  3, 21, 21, 31,  // 51  3
  3, 7, 4, 31,    // 52  4
  3, 23, 21, 29,  // 53  5
  3, 31, 21, 29,  // 54  6
  3, 3, 1, 31,    // 55  7
  3, 31, 21, 31,  // 56  8
  3, 23, 21, 31,  // 57  9
  0,  // 58
  0,  // 59
  0,  // 60
  0,  // 61
  0,  // 62
  0,  // 63
  0,  // 64
  0,  // 65
  0,  // 66
  0,  // 67
  0,  // 68
  0,  // 69
  0,  // 70
  0,  // 71
  0,  // 72
  0,  // 73
  0,  // 74
  0,  // 75
  0,  // 76
  0,  // 77
  0,  // 78
  0,  // 79
  0,  // 80
  0,  // 81
  0,  // 82
  0,  // 83
  0,  // 84
  0,  // 85
  0,  // 86
  0,  // 87
  0,  // 88
  0,  // 89
  0,  // 90
  0,  // 91
  0,  // 92
  0,  // 93
  0,  // 94
  0,  // 95
  0,  // 96
  0,  // 97
  0,  // 98
  0,  // 99
  0,  // 100
  0,  // 101
  0,  // 102
  0,  // 103
  0,  // 104
  0,  // 105
  0,  // 106
  0,  // 107
  0,  // 108
  0,  // 109
  0,  // 110
  0,  // 111
  0,  // 112
  0,  // 113
  0,  // 114
  0,  // 115
  0,  // 116
  0,  // 117
  0,  // 118
  0,  // 119
  0,  // 120
  0,  // 121
  0,  // 122
  0,  // 123
  0,  // 124
  0,  // 125
  0,  // 126
  0,  // 127
  0,  // 128
  0,  // 129
  0,  // 130
  0,  // 131
  0,  // 132
  0,  // 133
  0,  // 134
  0,  // 135
  0,  // 136
  0,  // 137
  0,  // 138
  0,  // 139
  0,  // 140
  0,  // 141
  0,  // 142
  0,  // 143
  0,  // 144
  0,  // 145
  0,  // 146
  0,  // 147
  0,  // 148
  0,  // 149
  0,  // 150
  0,  // 151
  0,  // 152
  0,  // 153
  0,  // 154
  0,  // 155
  0,  // 156
  0,  // 157
  0,  // 158
  0,  // 159
  0,  // 160
  0,  // 161
  0,  // 162
  0,  // 163
  0,  // 164
  0,  // 165
  0,  // 166
  0,  // 167
  0,  // 168
  0,  // 169
  0,  // 170
  0,  // 171
  0,  // 172
  0,  // 173
  0,  // 174
  0,  // 175
  0,  // 176
  0,  // 177
  0,  // 178
  0,  // 179
  0,  // 180
  0,  // 181
  0,  // 182
  0,  // 183
  0,  // 184
  0,  // 185
  0,  // 186
  0,  // 187
  0,  // 188
  0,  // 189
  0,  // 190
  0,  // 191
  0,  // 192
  0,  // 193
  0,  // 194
  0,  // 195
  0,  // 196
  0,  // 197
  0,  // 198
  0,  // 199
  0,  // 200
  0,  // 201
  0,  // 202
  0,  // 203
  0,  // 204
  0,  // 205
  0,  // 206
  0,  // 207
  0,  // 208
  0,  // 209
  0,  // 210
  0,  // 211
  0,  // 212
  0,  // 213
  0,  // 214
  0,  // 215
  0,  // 216
  0,  // 217
  0,  // 218
  0,  // 219
  0,  // 220
  0,  // 221
  0,  // 222
  0,  // 223
  0,  // 224
  0,  // 225
  0,  // 226
  0,  // 227
  0,  // 228
  0,  // 229
  0,  // 230
  0,  // 231
  0,  // 232
  0,  // 233
  0,  // 234
  0,  // 235
  0,  // 236
  0,  // 237
  0,  // 238
  0,  // 239
  0,  // 240
  0,  // 241
  0,  // 242
  0,  // 243
  0,  // 244
  0,  // 245
  0,  // 246
  0,  // 247
  0,  // 248
  0,  // 249
  0,  // 250
  0,  // 251
  0,  // 252
  0,  // 253
  0,  // 254
  0,  // 255
};
#endif

////////////////////////////////////////////////



const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>ESP32 MAX7219 LED Matrix 32x8 Digital Clock Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      :root {
        --bg-gradient: linear-gradient(135deg, #e0f7fa 0%, #f7fafc 100%);
        --card-bg: #fff;
        --card-shadow: 0 8px 32px rgba(0, 0, 0, 0.1),
          0 1.5px 8px rgba(0, 135, 143, 0.07);
        --text-main: #222;
        --accent: #00878f;
        --accent2: #00bcd4;
        --input-bg: #f9f9f9;
        --input-border: #b2ebf2;
        --input-focus-bg: #e0f7fa;
        --input-focus-border: #00bcd4;
        --button-bg: linear-gradient(90deg, #00878f 60%, #00bcd4 100%);
        --button-hover-bg: linear-gradient(90deg, #037980 60%, #26c6da 100%);
        --button-disabled-bg: #b2ebf2;
        --button-disabled-text: #888;
        --slider-bg: #e0f7fa;
        --slider-thumb: #00878f;
        --slider-thumb-focus: #00bcd4;
        --hr: #e0f7fa;
        --info: #888;
        --label: #00878f;
        --shadow-hover: 0 12px 36px rgba(0, 135, 143, 0.13),
          0 2px 12px rgba(0, 135, 143, 0.09);
      }
      body.night-mode {
        --bg-gradient: linear-gradient(135deg, #181c22 0%, #232b34 100%);
        --card-bg: #232b34;
        --card-shadow: 0 8px 32px rgba(0, 0, 0, 0.32),
          0 1.5px 8px rgba(0, 188, 212, 0.09);
        --text-main: #e0f7fa;
        --accent: #26c6da;
        --accent2: #00bcd4;
        --input-bg: #1a232a;
        --input-border: #374151;
        --input-focus-bg: #232b34;
        --input-focus-border: #26c6da;
        --button-bg: linear-gradient(90deg, #00bcd4 60%, #00878f 100%);
        --button-hover-bg: linear-gradient(90deg, #26c6da 60%, #037980 100%);
        --button-disabled-bg: #374151;
        --button-disabled-text: #aaa;
        --slider-bg: #26313a;
        --slider-thumb: #26c6da;
        --slider-thumb-focus: #00bcd4;
        --hr: #374151;
        --info: #b2ebf2;
        --label: #26c6da;
        --shadow-hover: 0 12px 36px rgba(0, 188, 212, 0.13),
          0 2px 12px rgba(0, 188, 212, 0.09);
      }
      html,
      body {
        font-family: "Segoe UI", Helvetica, Arial, sans-serif;
        background: var(--bg-gradient);
        margin: 0;
        padding: 0;
        min-height: 100vh;
        color: var(--text-main);
        transition: background 0.4s, color 0.3s;
      }
      body {
        display: flex;
        flex-direction: column;
        align-items: center;
        min-height: 100vh;
        justify-content: flex-start;
      }
      h1 {
        font-size: 2rem;
        color: var(--accent);
        margin-top: 2.5rem;
        margin-bottom: 2rem;
        letter-spacing: 1px;
        text-align: center;
        font-weight: 700;
        text-shadow: 0 2px 8px #b2ebf2;
        transition: color 0.3s;
      }
      .div_Form {
        background: var(--card-bg);
        border-radius: 22px;
        box-shadow: var(--card-shadow);
        padding: 2.2rem 2.5rem 2.2rem 2.5rem;
        width: 100%;
        max-width: 430px;
        margin-bottom: 2.5rem;
        margin-top: 0.5rem;
        transition: box-shadow 0.2s, background 0.3s;
        position: relative;
      }
      .div_Form:hover {
        box-shadow: var(--shadow-hover);
      }
      .night-toggle {
        position: absolute;
        top: 1.2rem;
        right: 1.2rem;
        display: flex;
        align-items: center;
        gap: 0.5em;
        z-index: 2;
        background: transparent;
      }
      .night-toggle label {
        font-size: 1em;
        color: var(--accent);
        font-weight: 600;
        cursor: pointer;
        margin-bottom: 0;
        letter-spacing: 0.5px;
        transition: color 0.3s;
      }
      .night-toggle input[type="checkbox"] {
        width: 42px;
        height: 22px;
        appearance: none;
        background: var(--slider-bg);
        outline: none;
        border-radius: 22px;
        position: relative;
        transition: background 0.3s;
        cursor: pointer;
        box-shadow: 0 0 2px #aaa;
      }
      .night-toggle input[type="checkbox"]:checked {
        background: var(--accent);
      }
      .night-toggle input[type="checkbox"]::before {
        content: "";
        position: absolute;
        left: 4px;
        top: 4px;
        width: 14px;
        height: 14px;
        background: #fff;
        border-radius: 50%;
        transition: 0.3s;
        box-shadow: 0 1px 4px #0002;
      }
      .night-toggle input[type="checkbox"]:checked::before {
        left: 24px;
        background: #fff;
      }
      label {
        font-size: 1rem;
        color: var(--text-main);
        margin-bottom: 0.3rem;
        display: block;
        font-weight: 500;
        transition: color 0.3s;
      }
      input[type="password"],
      input[type="text"] {
        width: 100%;
        padding: 10px 12px;
        margin: 0.3rem 0 1.1rem 0;
        border: 1.5px solid var(--input-border);
        border-radius: 8px;
        font-size: 1rem;
        box-sizing: border-box;
        background: var(--input-bg);
        color: var(--text-main);
        transition: border 0.2s, box-shadow 0.2s, background 0.3s, color 0.3s;
      }
      input[type="password"]:focus,
      input[type="text"]:focus {
        border: 2px solid var(--input-focus-border);
        outline: none;
        box-shadow: 0 0 0 2px var(--accent2);
        background: var(--input-focus-bg);
        color: var(--text-main);
      }
      .myButton {
        display: inline-block;
        padding: 10px 28px;
        font-size: 1rem;
        cursor: pointer;
        text-align: center;
        color: #fff;
        background: var(--button-bg);
        border: none;
        border-radius: 10px;
        box-shadow: 0 2px #b2ebf2;
        margin: 0.2rem 0.3rem 0.2rem 0;
        transition: background 0.2s, box-shadow 0.2s, transform 0.1s;
        font-weight: 600;
        letter-spacing: 0.5px;
      }
      .myButton:hover,
      .myButton:active {
        background: var(--button-hover-bg);
        box-shadow: 0 1px #80deea;
        transform: translateY(-2px) scale(1.03);
      }
      .myButton:disabled {
        background: var(--button-disabled-bg);
        color: var(--button-disabled-text);
        box-shadow: none;
        cursor: not-allowed;
      }
      .slider-container {
        display: flex;
        align-items: center;
        gap: 1rem;
        margin-bottom: 1.2rem;
        margin-top: 0.5rem;
      }
      .slider-label {
        min-width: 110px;
        font-size: 1rem;
        color: var(--label);
        font-weight: 600;
        transition: color 0.3s;
      }
      input[type="range"] {
        width: 100%;
        accent-color: var(--slider-thumb);
        margin: 0 0.5rem;
        height: 4px;
        background: var(--slider-bg);
        border-radius: 4px;
        outline: none;
        transition: accent-color 0.2s, background 0.3s;
      }
      input[type="range"]:focus {
        accent-color: var(--slider-thumb-focus);
        background: #b2ebf2;
      }
      .slider-value {
        font-weight: bold;
        color: var(--label);
        min-width: 2.5em;
        text-align: right;
        font-size: 1.1em;
        letter-spacing: 1px;
        transition: color 0.3s;
      }
      select {
        width: 100%;
        padding: 9px 12px;
        border-radius: 8px;
        border: 1.5px solid var(--input-border);
        background: var(--input-bg);
        color: var(--text-main);
        font-size: 1rem;
        margin-bottom: 1.1rem;
        transition: border 0.2s, background 0.3s, color 0.3s;
      }
      select:focus {
        border: 2px solid var(--input-focus-border);
        background: var(--input-focus-bg);
        outline: none;
        color: var(--text-main);
      }
      hr {
        border: none;
        border-top: 1.5px solid var(--hr);
        margin: 1.4rem 0;
        transition: border-color 0.3s;
      }
      .checkbox-row {
        display: flex;
        align-items: center;
        gap: 0.6rem;
        margin-bottom: 1.1rem;
      }
      .info-label {
        font-size: 0.95em;
        color: var(--info);
        margin-left: 0.5rem;
        display: block;
        margin-top: 0.2rem;
        margin-bottom: 0.2rem;
        transition: color 0.3s;
      }
      #show_Date_Time {
        font-size: 1.15rem;
        color: var (--label);
        margin-bottom: 1.2rem;
        display: block;
        text-align: center;
        font-weight: 600;
        letter-spacing: 1px;
        transition: color 0.3s;
      }
      /* Responsive Design */
      @media (max-width: 600px) {
        .div_Form {
          padding: 1.1rem 0.7rem 1.3rem 0.7rem;
          max-width: 98vw;
          margin: 0.7rem 0.2rem 1.5rem 0.2rem;
        }
        h1 {
          font-size: 1.25rem;
          margin-top: 1.2rem;
          margin-bottom: 1.1rem;
        }
        .slider-label {
          min-width: 80px;
          font-size: 0.98rem;
        }
        .slider-container {
          gap: 0.5rem;
        }
        .info-label {
          font-size: 0.9em;
        }
        #show_Date_Time {
          font-size: 1rem;
        }
        .night-toggle {
          top: 0.7rem;
          right: 0.7rem;
        }
      }
    </style>
  </head>
  <body>
    <h1>ESP32 CLOCK</h1>
    <div class="night-toggle">
      <label for="nightSwitch" id="nightModeLabel">Dark Mode</label>
      <input type="checkbox" id="nightSwitch" onchange="toggleNightMode()" />
    </div>
    <div class="div_Form">
      <form onsubmit="return false;">
        <!-- Key -->
        <label for="Keys_TXT">Key :</label>
        <div class="checkbox-row">
          <input
            type="password"
            id="Keys_TXT"
            name="Keys_TXT"
            maxlength="20"
            placeholder="Enter key here..."
          />
          <input
            type="checkbox"
            id="showKeyCheckbox"
            onclick="myCheckBoxEvent()"
          />
          <label for="showKeyCheckbox" style="font-size: 13px; margin: 0"
            >Show Key</label
          >
        </div>
        <hr />
        <!-- Date/Time -->
        <label id="show_Date_Time">Please wait...</label>
        <button
          type="button"
          class="myButton"
          id="BTN_Set_DateTime_ESP32_DS3231"
          onclick="BTN_Set_DateTime_ESP32_DS3231_Click()"
        >
          Set Time and Date
        </button>
        <hr />
        <!-- Get Settings -->
        <button
          type="button"
          class="myButton"
          id="BTN_Getting_Texts"
          onclick="BTN_Get_Settings_Click()"
        >
          Get Settings
        </button>
        <span class="info-label">*Get settings from ESP32.</span>
        <hr />
        <!-- Brightness Slider -->
        <div class="slider-container">
          <span class="slider-label">Brightness:</span>
          <input
            type="range"
            min="0"
            max="15"
            id="brt_Val_Sel"
            value="8"
            oninput="onBrightnessSliderChange(this.value)"
          />
          <span class="slider-value" id="brt_Val_Display">8</span>
        </div>
        <hr />
        <!-- Clock Display Mode -->
        <label for="mode_Val_Sel">Set Clock Display Mode :</label>
        <select id="mode_Val_Sel">
          <option value="1">MODE 1</option>
          <option value="2">MODE 2</option>
          <option value="3">MODE 3</option>
        </select>
        <button
          type="button"
          class="myButton"
          id="BTN_Set_Clock_Display_Mode"
          onclick="BTN_Set_Clock_Display_Mode_Click()"
        >
          Set
        </button>
        <hr />
        <!-- Scrolling Text -->
        <label for="input_TXT">Set Messages On Scrolling Text :</label>
        <input
          type="text"
          id="input_TXT"
          name="input_TXT"
          maxlength="30"
          placeholder="Enter text here...(Max 30 characters)"
        />
        <button
          type="button"
          class="myButton"
          id="BTN_Set_Messages"
          onclick="BTN_Set_Messages_Click()"
        >
          Set
        </button>
        <button
          type="button"
          class="myButton"
          id="BTN_Clear_Input_Text"
          onclick="BTN_Clear_Input_Text_Click()"
        >
          Clear Input Text
        </button>
      </form>
    </div>
    <script>
      setInterval(myTimer, 1000);

      var t_Hour, t_Minute, t_Second;
      const d_DaysOfTheWeek_Array = [
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
      ];
      var d_DaysOfTheWeek_Now, d_Day, d_Month, d_Year, full_DateTime;

      function myTimer() {
        getDateTime();
        document.getElementById("show_Date_Time").innerHTML = full_DateTime;
      }

      function getDateTime() {
        const dt = new Date();
        t_Hour = dt.getHours();
        t_Minute = dt.getMinutes();
        t_Second = dt.getSeconds();
        d_DaysOfTheWeek_Now = dt.getDay();
        d_Day = dt.getDate();
        d_Month = dt.getMonth();
        d_Year = dt.getFullYear();
        full_DateTime = "Date : <b>";
        full_DateTime += d_DaysOfTheWeek_Array[d_DaysOfTheWeek_Now];
        full_DateTime += ", ";
        full_DateTime +=
          d_Day.toString().padStart(2, "0") +
          "-" +
          (d_Month + 1).toString().padStart(2, "0") +
          "-" +
          d_Year;
        full_DateTime +=
          "</b> | Time : <b>" +
          t_Hour.toString().padStart(2, "0") +
          ":" +
          t_Minute.toString().padStart(2, "0") +
          ":" +
          t_Second.toString().padStart(2, "0") +
          "</b>";
      }

      function BTN_Set_DateTime_ESP32_DS3231_Click() {
        var get_Key_TXT = document.getElementById("Keys_TXT").value;
        if (get_Key_TXT == "") {
          alert("Error ! \rKey tidak boleh kosong !");
          return;
        }
        getDateTime();
        var send_Request;
        send_Request = "key=" + get_Key_TXT;
        send_Request += "&sta=setDT";
        send_Request += "&dt_year=" + d_Year;
        send_Request += "&dt_month=" + (d_Month + 1);
        send_Request += "&dt_day=" + d_Day;
        send_Request += "&tm_hour=" + t_Hour;
        send_Request += "&tm_minute=" + t_Minute;
        send_Request += "&tm_second=" + t_Second;
        Send("set", send_Request);
      }

      function BTN_Get_Settings_Click() {
        var key_TXT = document.getElementById("Keys_TXT").value;
        if (key_TXT == "") {
          alert("Error ! \rThe key cannot be empty.");
          return;
        }
        var send_Request;
        send_Request = "key=" + key_TXT;
        send_Request += "&sta=getSettings";
        Send("get", send_Request);
      }

      // Brightness slider logic
      let brightnessTimeout = null;
      function onBrightnessSliderChange(val) {
        document.getElementById("brt_Val_Display").textContent = val;
        // Debounce: only send after user stops sliding for 300ms
        if (brightnessTimeout) clearTimeout(brightnessTimeout);
        brightnessTimeout = setTimeout(function () {
          BTN_Brightness_Intensity_AutoSet(val);
        }, 300);
      }
      function BTN_Brightness_Intensity_AutoSet(val) {
        var key_TXT = document.getElementById("Keys_TXT").value;
        if (key_TXT == "") {
          // Don't alert on slider, just skip if no key
          return;
        }
        var send_Request;
        send_Request = "key=" + key_TXT;
        send_Request += "&sta=setBrt";
        send_Request += "&brt=" + val;
        Send("set", send_Request);
      }

      function BTN_Set_Clock_Display_Mode_Click() {
        var key_TXT = document.getElementById("Keys_TXT").value;
        if (key_TXT == "") {
          alert("Error ! \rThe key cannot be empty.");
          return;
        }
        var cdm_Val = document.getElementById("mode_Val_Sel").value;
        var send_Request;
        send_Request = "key=" + key_TXT;
        send_Request += "&sta=setCDM";
        send_Request += "&cdm=" + cdm_Val;
        Send("set", send_Request);
      }

      function BTN_Set_Messages_Click() {
        var key_TXT = document.getElementById("Keys_TXT").value;
        if (key_TXT == "") {
          alert("Error ! \rThe key cannot be empty.");
          return;
        }
        var input_text_Val = document.getElementById("input_TXT").value;
        var send_Request;
        send_Request = "key=" + key_TXT;
        send_Request += "&sta=setMsg";
        send_Request += "&msg=" + input_text_Val;
        Send("set", send_Request);
      }

      function BTN_Clear_Input_Text_Click() {
        document.getElementById("input_TXT").value = "";
      }

      function myCheckBoxEvent() {
        var x = document.getElementById("Keys_TXT");
        x.type = x.type === "password" ? "text" : "password";
      }

      function Send(sta, req) {
        if (window.XMLHttpRequest) {
          xmlhttp = new XMLHttpRequest();
        } else {
          xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
        }
        xmlhttp.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            if (this.responseText == "+OK_setDT") {
              alert("Setting the time and date was successful.");
            } else if (this.responseText == "+OK_setBrt") {
              // Optionally show a toast or highlight the slider
            } else if (this.responseText == "+OK_setCDM") {
              alert("Setting the clock display mode is successful.");
            } else if (this.responseText == "+OK_setMsg") {
              alert("Setting the message on scrolling text was successful.");
            } else if (this.responseText == "+ERR") {
              alert("Error !\rWrong Key !\rPlease enter the correct key.");
            } else {
              let Get_Settings = this.responseText;
              const myArray_Get_Settings = Get_Settings.split("|");
              if (myArray_Get_Settings[0] == "gs") {
                document.getElementById("brt_Val_Sel").value =
                  myArray_Get_Settings[1];
                document.getElementById("brt_Val_Display").textContent =
                  myArray_Get_Settings[1];
                document.getElementById("mode_Val_Sel").value =
                  myArray_Get_Settings[2];
                document.getElementById("input_TXT").value =
                  myArray_Get_Settings[3];
                alert("Successfully got the settings.");
              }
            }
          }
        };
        xmlhttp.open("GET", "settings?" + req, true);
        xmlhttp.send();
      }
      function toggleNightMode() {
        document.body.classList.toggle("night-mode");
        const isDark = document.body.classList.contains("night-mode");
        document.getElementById("nightModeLabel").textContent = isDark
          ? "Light Mode"
          : "Dark Mode";
        // Optionally, save preference to localStorage:
        if (isDark) {
          localStorage.setItem("nightMode", "1");
        } else {
          localStorage.removeItem("nightMode");
        }
      }
      // On load, restore night mode if set and update label
      window.addEventListener("DOMContentLoaded", function () {
        const isDark = localStorage.getItem("nightMode");
        if (isDark) {
          document.body.classList.add("night-mode");
          document.getElementById("nightSwitch").checked = true;
          document.getElementById("nightModeLabel").textContent = "Light Mode";
        } else {
          document.getElementById("nightModeLabel").textContent = "Dark Mode";
        }
      });
    </script>
  </body>
</html>
)=====";



