
// //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 05_ESP32_MAX7219_DS3231_Digital_Clock_WS_SM
// //---------------------------------------- Including Libraries.
// #include <WiFi.h>
// #include <WebServer.h>
// #include <MD_Parola.h>
// #include <MD_MAX72xx.h>
// #include <SPI.h>
// #include "Custom_Font_Data.h"
// #include <Wire.h>
// #include "RTClib.h"
// #include <Preferences.h>
// #include "PageIndex.h"
// //----------------------------------------

// // Defines the MAX7219 LED Matrix hardware type.
// #define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// // Defines the number of devices and CS PIN.
// #define MAX_DEVICES 4
// #define CS_PIN      5

// // Defines the scrolling text speed, the scrolling seconds effect speed from top to bottom (in MODE_3) and the maximum number of characters for scrolling text.
// #define SPEED_TIME_SCROLLING_TEXT   50
// #define SPEED_TIME_SCROLLING_DOWN   80
// #define MAX_SCROLLING_TEXT_CHAR     30

// // Defines the MODE for displaying the clock.
// // - MODE_1 : Only displays hours and minutes. The colon between the hours and minutes will flash to indicate changing seconds.
// // - MODE_2 : Displays hours, minutes and seconds.
// // - MODE_3 : Displays hours, minutes and seconds. The seconds display uses a “scroll down” effect every second change.
// #define MODE_1  1
// #define MODE_2  2
// #define MODE_3  3

// //----------------------------------------Variable declaration for your network credentials.
// const char* ssid = "REPLACE_WITH_YOUR_SSID";  //--> Your wifi name
// const char* password = "REPLACE_WITH_YOUR_PASSWORD"; //--> Your wifi password
// //----------------------------------------

// //----------------------------------------Defining the key.
// // "Key" functions like a password. In order to change the text on the MAX7219 LED Matrix 32x8, the user must know the "key".
// // You can change it to another word.
// #define key_Txt "max7219esp32dcws"
// //----------------------------------------

// // Variable used to select the clock display mode.
// byte DIGITAL_CLOCK_DISPLAY_MODE;

// // Variable to hold the text that will be displayed in the scrolling text.
// char str_ScrollingText_Msg[35] = "";

// // Char array variable "Time_H_M" to hold the hour and minute values. Char array variable "Time_S" to hold the seconds value.
// char Time_H_M[10];
// char Time_S[3];

// // Variables for time and date. This variable is used when setting the time and date.
// byte t_hour;
// byte t_minute;
// byte t_second;
// byte d_day;
// byte d_month;
// int d_year;

// // Char array variable to hold day and date data.
// char str_Date[50];

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// char month_Name[12][10] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// // Variables for displaying or printing the time and date on a serial monitor or serial communication.
// char str_PrintFull_DateTime[60];

// // Variables for Millis/Timer.
// unsigned long previousMillis = 0;
// const long interval = 1000;

// // Variable to set the LED Matrix brightness intensity value.
// // Range from 0 to 15.
// byte brightness_Intensity = 0;

// bool colon_Blink = false;

// // Initialize MD_Parola as Display and its settings.
// MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// // Initialize RTC_DS3231 as rtc.
// RTC_DS3231 rtc;

// // Initialize the "Preferences" library as "preferences".
// Preferences preferences;

// // Server on port 80.
// WebServer server(80);

// //________________________________________________________________________________ handleRoot()
// // This routine is executed when you open ESP32 IP Address in browser.
// void handleRoot() {
//   server.send(200, "text/html", MAIN_page); //Send web page
// }
// //________________________________________________________________________________

// //________________________________________________________________________________ handleSettings().
// // Subroutine to handle settings. The displayed text are set here.
// void handleSettings() {
//   String incoming_Settings;
//   int incoming_Settings_Length;

//   incoming_Settings = server.arg("key");

//   Serial.println();
//   Serial.println("------------Settings");
//   Serial.print("Key : ");
//   Serial.println(incoming_Settings);

//   //---------------------------------------- Conditions for checking keys.
//   if (incoming_Settings == key_Txt) {
//     incoming_Settings = server.arg("sta");

//     //........................................ Conditions for setting the time and date.
//     if (incoming_Settings == "setDT") {
//       Serial.println();
//       Serial.println("Set Time and Date.");

//       incoming_Settings = server.arg("dt_year");
//       d_year = incoming_Settings.toInt();

//       incoming_Settings = server.arg("dt_month");
//       d_month = incoming_Settings.toInt();

//       incoming_Settings = server.arg("dt_day");
//       d_day = incoming_Settings.toInt();

//       incoming_Settings = server.arg("tm_hour");
//       t_hour = incoming_Settings.toInt();

//       incoming_Settings = server.arg("tm_minute");
//       t_minute = incoming_Settings.toInt();

//       incoming_Settings = server.arg("tm_second");
//       t_second = incoming_Settings.toInt();

//       delay(10);

//       Serial.println();
//       Serial.print("Date : ");
//       Serial.print(d_day);Serial.print("-");Serial.print(d_month);Serial.print("-");Serial.print(d_year);
//       Serial.print(" | Time : ");
//       Serial.print(t_hour);Serial.print(":");Serial.print(t_minute);Serial.print(":");Serial.println(t_second);
//       Serial.println("------------");

//       // Set the time and date on the DS3231 rtc module.
//       rtc.adjust(DateTime(d_year, d_month, d_day, t_hour, t_minute, t_second));  //--> rtc.adjust(DateTime(year, month, day, hour, minute, second));
//       delay(10);

//       set_Clock_Display();

//       server.send(200, "text/plane", "+OK_setDT");  //--> Sending replies to the client.
//       delay(500);
//     }
//     //........................................

//     //........................................ Conditions for sending settings to a web page or client.
//     if (incoming_Settings == "getSettings") {
//       Serial.println();
//       Serial.println("Get Settings");

//       char send_Settings[50];
//       sprintf(send_Settings, "gs|%d|%d|%s", brightness_Intensity, DIGITAL_CLOCK_DISPLAY_MODE, str_ScrollingText_Msg);

//       Serial.println(send_Settings);
//       Serial.println("------------");

//       server.send(200, "text/plane", send_Settings);  //--> Sending replies to the client.
//       delay(500);
//     }
//     //........................................

//     //........................................ Conditions for setting brightness intensity.
//     if (incoming_Settings == "setBrt") {
//       Serial.println();
//       Serial.println("Set the Brightness Intensity.");

//       incoming_Settings = server.arg("brt");
//       brightness_Intensity = incoming_Settings.toInt();

//       delay(10);

//       Serial.println();
//       Serial.print("Brightness Intensity from Client : ");
//       Serial.println(brightness_Intensity);

//       //######################################## Save and read the brightness intensity value to or from flash memory.
//       // Open Preferences with "mySettings" namespace.
//       preferences.begin("mySettings", false);
//       delay(10);

//       // Save the brightness_Intensity value.
//       preferences.putInt("brt_Intensity", brightness_Intensity);
//       delay(10);

//       // Get the brightness_Intensity value, if the key does not exist, return a default value of 0.
//       brightness_Intensity = preferences.getInt("brt_Intensity", 0);
//       delay(10);

//       Serial.println();
//       Serial.print("Saved Brightness Intensity : ");
//       Serial.println(brightness_Intensity);
//       Serial.println("------------");

//       // Close the Preferences.
//       preferences.end();
//       delay(10);
//       //########################################

//       set_Clock_Display();

//       server.send(200, "text/plane", "+OK_setBrt");  //--> Sending replies to the client.
//       delay(500);
//     }
//     //........................................

//     //........................................ Conditions for setting the clock display mode.
//     if (incoming_Settings == "setCDM") {
//       Serial.println();
//       Serial.println("Set the clock display mode.");

//       incoming_Settings = server.arg("cdm");
//       DIGITAL_CLOCK_DISPLAY_MODE = incoming_Settings.toInt();

//       delay(100);

//       Serial.println();
//       Serial.print("Clock Display Modes from Client : MODE_");
//       Serial.println(DIGITAL_CLOCK_DISPLAY_MODE);

//       //######################################## Save and read "clock display mode" to or from flash memory.
//       // Open Preferences with "mySettings" namespace.
//       preferences.begin("mySettings", false);
//       delay(10);

//       // Save the DIGITAL_CLOCK_DISPLAY_MODE value.
//       preferences.putInt("cd_Mode", DIGITAL_CLOCK_DISPLAY_MODE);
//       delay(10);

//       // Get the DIGITAL_CLOCK_DISPLAY_MODE value, if the key does not exist, return a default value of 1.
//       DIGITAL_CLOCK_DISPLAY_MODE = preferences.getInt("cd_Mode", 1);
//       delay(10);

//       Serial.println();
//       Serial.print("Saved Display Modes : MODE_");
//       Serial.println(DIGITAL_CLOCK_DISPLAY_MODE);
//       Serial.println("------------");

//       // Close the Preferences.
//       preferences.end();
//       delay(10);
//       //########################################

//       set_Clock_Display();

//       server.send(200, "text/plane", "+OK_setCDM");  //--> Sending replies to the client.
//       delay(500);
//     }
//     //........................a................

//     //........................................ Conditions for setting messages on scrolling text.
//     if (incoming_Settings == "setMsg") {
//       Serial.println();
//       Serial.println("Set Messages On Scrolling Text.");

//       incoming_Settings = server.arg("msg");
//       incoming_Settings_Length = incoming_Settings.length() + 1;
//       incoming_Settings.toCharArray(str_ScrollingText_Msg, incoming_Settings_Length);

//       delay(100);

//       Serial.println();
//       Serial.print("Message On Scrolling Text from Client : ");
//       Serial.println(str_ScrollingText_Msg);

//       //######################################## Save and read the "message text" which will be displayed in scrolling text to or from flash memory.
//       // Open Preferences with "mySettings" namespace.
//       preferences.begin("mySettings", false);
//       delay(10);

//       // Save the str_ScrollingText_Msg value.
//       preferences.putString("Msg_ST", str_ScrollingText_Msg);
//       delay(10);

//       // Get the str_ScrollingText_Msg value, if the key does not exist, return a default value of "".
//       String saved_Msg = "";
//       int saved_Msg_Length;
//       saved_Msg = preferences.getString("Msg_ST", "");
//       saved_Msg_Length = saved_Msg.length() + 1;
//       saved_Msg.toCharArray(str_ScrollingText_Msg, saved_Msg_Length);
//       delay(10);

//       Serial.println();
//       Serial.print("Saved Message On Scrolling Text : ");
//       Serial.println(str_ScrollingText_Msg);
//       Serial.println("------------");

//       // Close the Preferences.
//       preferences.end();
//       delay(10);
//       //########################################

//       server.send(200, "text/plane", "+OK_setMsg");  //--> Sending replies to the client.
//       delay(500);
//     }
//     //........................................

//   } else {
//     Serial.println();
//     Serial.println("Wrong Key Text !");
//     Serial.println("Please enter the correct Key Text.");
//     Serial.println("------------");

//     server.send(200, "text/plane", "+ERR"); //--> Sending replies to the client.
//     delay(500);
//   }
//   //----------------------------------------
// }
// //________________________________________________________________________________

// //________________________________________________________________________________ set_Clock_Display()
// // Subroutine for setting the clock display mode.
// void set_Clock_Display() {
//   // Conditions for displaying the clock in MODE_1.
//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
//     Display.begin();
//     Display.setIntensity(brightness_Intensity);
//     Display.setInvert(false);

//     // customFont5x7 to display seconds. This font only contains the characters:
//     // - Space.
//     // - 0 to 9.
//     // - : (colon).
//     // - ^ (I changed the "^" character to "two pixel space". For explanation, see "void get_Time_and_Date()".).
//     Display.setFont(0, customFont5x7);

//     Display.displayText(Time_H_M, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
//   }

//   // Conditions for displaying the clock in MODE_2 and MODE_3.
//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
//     Display.begin(2);
//     Display.setInvert(false);

//     // In MODE_2 and MODE_3 to display hours, minutes and seconds on the LED Matrix using "zones".
//     // For more details about the "zones" in the LED Matrix, please see here : https://arduinoplusplus.wordpress.com/2017/04/18/parola-a-to-z-multi-zone-displays/
//     Display.setZone(0, 0, 0);
//     Display.setZone(1, 1, 3);

//     Display.setIntensity(brightness_Intensity);

//     // customFont3x5 to display "seconds". This font only contains the characters :
//     // - Space.
//     // - 0 to 9.
//     Display.setFont(0, customFont3x5);

//     // customFont4x7 to display "hours and minutes". This font only contains the characters :
//     // - Space.
//     // - 0 to 9.
//     // - : (colon).
//     Display.setFont(1, customFont4x7);

//     // Conditions for the "seconds" display mode in MODE_2.
//     if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2) Display.displayZoneText(0, Time_S, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);

//     // Conditions for the "seconds" display mode in MODE_3.
//     if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) Display.displayZoneText(0, Time_S, PA_RIGHT, SPEED_TIME_SCROLLING_DOWN, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);

//     // Displays "hours and minutes" on MODE_2 and MODE_3.
//     Display.displayZoneText(1, Time_H_M, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
//   }
// }
// //________________________________________________________________________________

// //________________________________________________________________________________ scroll_Display(char szMesg[MAX_SCROLLING_TEXT_CHAR + 1])
// // Subroutine for displaying text on scrolling text.
// // Scrolling text uses built-in fonts from the library.
// void scroll_Display(char szMesg[MAX_SCROLLING_TEXT_CHAR + 1]) {
//   Display.begin();
//   Display.setIntensity(brightness_Intensity);
//   Display.setInvert(false);
//   delay(1000);
//   Display.displayScroll(szMesg, PA_RIGHT, PA_SCROLL_LEFT, SPEED_TIME_SCROLLING_TEXT);

//   while(true) {
//     if (Display.displayAnimate()) {
//       Display.displayReset();
//       set_Clock_Display();
//       delay(1000);
//       break;
//     }
//   }
// }
// //________________________________________________________________________________

// //________________________________________________________________________________ get_Time_and_Date()
// // Subroutine to get time and date data from DS3231 rtc module.
// void get_Time_and_Date() {
//   DateTime now = rtc.now();

//   t_second = now.second();

//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_1) {
//     colon_Blink = !colon_Blink;

//     // On the digital clock display "MODE_1", to display the clock using "custom Font 5x7".
//     // In the "MODE_1" display the seconds are not displayed. As a sign of changing seconds, the colon between the hours and minutes will flash based on the seconds.
//     // The colon in the font "customFont5x7" uses 2 pixels, so to make it blink I changed the "^" character to two empty pixels.
//     // So when the colon appears, the character used is "colon" and when the colon does not appear, the character used is "^" which I have changed to two empty pixels.
//     // Sorry if my explanation is confusing, I hope you understand what I mean.
//     sprintf(Time_H_M, "%02d %s %02d", now.hour(), (colon_Blink ? ":" : "^"), now.minute());  //--> Create hour and minute data.
//   }

//   if (DIGITAL_CLOCK_DISPLAY_MODE == MODE_2 || DIGITAL_CLOCK_DISPLAY_MODE == MODE_3) {
//     sprintf(Time_H_M, "%02d:%02d", now.hour(), now.minute());  //--> Create data for hours and minutes.
//   }

//   // Create data for seconds.
//   sprintf(Time_S, "%02d", now.second());

//   // Create data for dates.
//   sprintf(str_Date, "%s, %d %s %d", daysOfTheWeek[now.dayOfTheWeek()], now.day(), month_Name[now.month()-1], now.year());

//   // Creates data for dates to be printed on serial monitors or serial communications.
//   sprintf(str_PrintFull_DateTime, "Date : %s, %d %s %d | Time : %02d:%02d:%02d", daysOfTheWeek[now.dayOfTheWeek()], now.day(), month_Name[now.month()-1], now.year(),
//                                                                                  now.hour(), now.minute(), now.second());

//   Serial.println();
//   Serial.println(str_PrintFull_DateTime);
// }
// //________________________________________________________________________________

// //________________________________________________________________________________ VOID SETUP()
// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);
//   Serial.println();
//   delay(1000);

//   //---------------------------------------- Reads settings stored in flash memory.
//   // Open Preferences with "mySettings" namespace.
//   preferences.begin("mySettings", false);
//   delay(10);

//   // Get the brightness_Intensity value, if the key does not exist, return a default value of 0.
//   brightness_Intensity = preferences.getInt("brt_Intensity", 0);
//   delay(10);

//   // Get the DIGITAL_CLOCK_DISPLAY_MODE value, if the key does not exist, return a default value of 1.
//   DIGITAL_CLOCK_DISPLAY_MODE = preferences.getInt("cd_Mode", 1);
//   delay(10);

//   // Get the str_ScrollingText_Msg value, if the key does not exist, return a default value of "".
//   String saved_Msg = "";
//   int saved_Msg_Length;
//   saved_Msg = preferences.getString("Msg_ST", "");
//   saved_Msg_Length = saved_Msg.length() + 1;
//   saved_Msg.toCharArray(str_ScrollingText_Msg, saved_Msg_Length);
//   delay(10);

//   // Close the Preferences.
//   preferences.end();
//   delay(10);

//   Serial.println();
//   Serial.println("------------Saved settings.");
//   Serial.print("Brightness Intensity : ");
//   Serial.println(brightness_Intensity);
//   Serial.print("Clock Display Mode : MODE_");
//   Serial.println(DIGITAL_CLOCK_DISPLAY_MODE);
//   Serial.print("Message on Scrolling Text : ");
//   Serial.println(str_ScrollingText_Msg);
//   Serial.println("------------");
//   //----------------------------------------

//   delay(100);

//   //---------------------------------------- Set Wifi to STA mode.
//   Serial.println();
//   Serial.println("-------------");
//   Serial.println("WIFI mode : STA");
//   WiFi.mode(WIFI_STA);
//   Serial.println("-------------");
//   //----------------------------------------

//   delay(100);

//   //---------------------------------------- Connect to Wi-Fi (STA).
//   Serial.println();
//   Serial.println("------------");
//   Serial.print("Connecting to ");
//   Serial.println(ssid);
//   WiFi.begin(ssid, password);

//   //:::::::::::::::::: The process of connecting ESP32 with WiFi Hotspot / WiFi Router.
//   // The process timeout of connecting ESP32 with WiFi Hotspot / WiFi Router is 20 seconds.
//   // If within 20 seconds the ESP32 has not been successfully connected to WiFi, the ESP32 will restart.
//   // I made this condition because on my ESP32, there are times when it seems like it can't connect to WiFi, so it needs to be restarted to be able to connect to WiFi.

//   int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
//   connecting_process_timed_out = connecting_process_timed_out * 2;
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     delay(500);
//     if(connecting_process_timed_out > 0) connecting_process_timed_out--;
//     if(connecting_process_timed_out == 0) {
//       Serial.println();
//       Serial.println("Failed to connect to WiFi. The ESP32 will be restarted.");
//       Serial.println("------------");
//       delay(1000);
//       ESP.restart();
//     }
//   }

//   Serial.println();
//   Serial.println("WiFi connected");
//   Serial.print("Successfully connected to : ");
//   Serial.println(ssid);
//   Serial.println("------------");
//   //::::::::::::::::::
//   //----------------------------------------

//   delay(100);

//   //---------------------------------------- Setting the server.
//   server.on("/", handleRoot);
//   server.on("/settings", handleSettings);
//   //----------------------------------------

//   delay(100);

//   //---------------------------------------- Start server.
//   server.begin();
//   Serial.println();
//   Serial.println("HTTP server started");
//   //----------------------------------------

//   delay(100);

//   //---------------------------------------- Starting MAX7219 LED Matrix 32x8.
//   Serial.println();
//   Serial.println("Starting MAX7219 LED Matrix 32x8.");
//   Display.begin();
//   delay(100);
//   Display.begin();
//   delay(100);
//   //----------------------------------------

//   Display.setIntensity(brightness_Intensity);

//   //---------------------------------------- All LEDs in the LED Matrix will turn on and off twice when the ESP32 is turned on.
//   bool State = false;
//   for (byte i = 0; i < 4; i++) {
//     State = !State;
//     Display.displayClear();
//     Display.setInvert(State);
//     delay(1000);
//   }
//   Display.displayClear();
//   //----------------------------------------

//   //---------------------------------------- Starting the DS3231 RTC module.
//   Serial.println();
//   Serial.println("Starting the DS3231 RTC module.");
//   if (!rtc.begin()) {
//     Serial.println("Couldn't find RTC");
//     while (1);
//   }
//   //----------------------------------------

//   // Open the IP address in your browser to open the interface page.
//   // Make sure that your computer / mobile device and ESP32 are connected to the same WiFi network.
//   Serial.println();
//   Serial.print("IP address : ");
//   Serial.println(WiFi.localIP());
//   Serial.println("Open the IP address in your browser to open the interface page.");
//   Serial.println("Make sure that your computer / mobile device and");
//   Serial.println("ESP32 are connected to the same WiFi network.");

//   delay(500);

//   set_Clock_Display();
// }
// //________________________________________________________________________________

// //________________________________________________________________________________ VOID LOOP()
// void loop() {
//   // put your main code here, to run repeatedly:

//   // Handle client requests.
//   server.handleClient();

//   // Displays or animates and returns true when an animation is completed.
//   Display.displayAnimate();

//   //---------------------------------------- Timer/Millis to update the time and date. And also execute other conditions.
//   unsigned long currentMillis = millis();
//   if (currentMillis - previousMillis >= interval) {
//     previousMillis = currentMillis;

//     get_Time_and_Date();

//     // The date will be displayed in the scrolling text if seconds equals 10 or at the 10th of the second.
//     if (t_second == 10) scroll_Display(str_Date);

//     // The message will be displayed in the scrolling text if seconds equals 40 or at the 40th second.
//     if (t_second == 40 && strlen(str_ScrollingText_Msg) > 0) {
//       scroll_Display(str_ScrollingText_Msg);
//     }

//     // Displays the next display.
//     Display.displayReset(0);
//     Display.displayReset(1);
//   }
//   //----------------------------------------
// }
// //________________________________________________________________________________
// //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<