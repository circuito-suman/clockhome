#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

Ticker ticker;
Ticker Timer500ms;
Ticker BrightnessTicker;
Ticker tickerRefresh;

WiFiManager wifiMn;

char ssid[] = "-----";      // your network SSID (name)
char pass[] = "---------";      // your network password

unsigned int localPort = 123;  // local port to listen for UDP packets

IPAddress timeServerIP;         // time.nist.gov NTP server address
const char* ntpServerName = "in.pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;



unsigned long epoch;
bool res;


double timeZoneCorrection = 5.30;
const int rebootEvery = 60 * 60 * 24;


const int flashButtonPin = 0;

const int dataPin = 14;  // DS pin of 74HC595
const int clockPin =12 ; // SH_CP pin of 74HC595
const int latchPin = 13; // ST_CP pin of 74HC595
const int oePin = 2;   // OE pin of 74HC595 (connect to a PWM-capable pin)
const int ldrPin = A0;  // LDR connected to A0
const int LEDpin = 4;  // Pin connected to LED

bool Led = 0;

const byte sevenSegmentDigits[10] = {
    0b11111100, // 0
    0b01100000, // 1
    0b11011010, // 2
    0b11110010, // 3
    0b01100110, // 4
    0b10110110, // 5
    0b10111110, // 6
    0b11100000, // 7
    0b11111110, // 8
    0b11110110  // 9
};

// Array to store the digits to be displayed





int hours;
  int minutes;
  int seconds;
 



void tick() {


/* 
 * Executed every 1 second
 */
   hours   = ((epoch+19800) % 86400L) / 3600;
   minutes = ((epoch+19800) % 3600) / 60;
   seconds =  (epoch+19800) % 60;
 




  // Add 1 second
  epoch++;  
  Serial.println(hours);
    Serial.println(minutes);

}


void Setiap500ms()
{
  Led = !Led;
  digitalWrite(LEDpin, Led);
  if (Led == 1)
  {
    //Serial.println(int(hour() * 100 + minute()));
  }
}

void adjustBrightness()
{
  //int ldrValue = analogRead(ldrPin);
  // Serial.println(ldrValue);
  //int brightness = map(ldrValue, 100, 1023, 230, 0); // Map LDR value to PWM value (inverted)
  analogWrite(oePin, 0);
}

void updateDisplay()
{
  digitalWrite(latchPin, 0);
shiftOut(dataPin, clockPin, LSBFIRST, sevenSegmentDigits[(int)(minutes % 10)]);
shiftOut(dataPin, clockPin, LSBFIRST, sevenSegmentDigits[(int)(minutes / 10)]);
shiftOut(dataPin, clockPin, LSBFIRST, sevenSegmentDigits[(int)(hours % 10)]);
shiftOut(dataPin, clockPin, LSBFIRST, sevenSegmentDigits[(int)(hours / 10)]);


digitalWrite(latchPin, 1);

}


void reboot()
{
  disableDigits();
  ESP.restart();
}

void disableDigits()
{
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b00000000);

  digitalWrite(latchPin, 1);
}

void wifinotconnected()
{
  digitalWrite(LEDpin, 1);
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b11100000);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b11100000);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b11100000);
  shiftOut(dataPin, clockPin, LSBFIRST, 0b11100000);

  digitalWrite(latchPin, 1);
}


void setup()
{

Serial.begin(115200);

  // Set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(oePin, OUTPUT);         // OE pin for controlling brightness
  pinMode(LEDpin, OUTPUT);        // LED pin for blinking
  pinMode(flashButtonPin, INPUT); // Pin for resetting wifi credentials
  pinMode(ldrPin, INPUT);
  udp.begin(localPort);

  BrightnessTicker.attach_ms(10, adjustBrightness); // Adjust brightness every 250 milliseconds

  if (WiFi.status() != WL_CONNECTED)
    wifinotconnected();

  if (!wifiMn.autoConnect("Esp Web Server", "12345678"))
  {
    Serial.println("Failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  }

  Serial.println("Conected to Wifi");

  getDateTime();

  Timer500ms.attach_ms(500, Setiap500ms);

  tickerRefresh.attach(rebootEvery, reboot);
}


void loop()
{
  if (digitalRead(flashButtonPin) == LOW)
  {
    wifiMn.resetSettings();
    ESP.reset();
    Serial.println("Wifi settings reset");
    delay(1000);
  }
  updateDisplay();
}


/* 
 *  The following code is based on the NTP Client example for ESP8266, more info on:
 *  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi/examples/NTPClient
 */
void getDateTime() {

  disableDigits();
  bool timeSuccess = false;
  ticker.detach();

  while (!timeSuccess) {
  
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP); 
  
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);
    
    int cb = udp.parsePacket();
    if (!cb) {
      Serial.println("no packet yet");
    }
    else {      
      Serial.print("packet received, length=");
      Serial.println(cb);
      // We've received a packet, read the data from it
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
  
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Seconds since Jan 1 1900 = " );
      Serial.println(secsSince1900);
  
      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      epoch =( secsSince1900 - seventyYears);
      // print Unix time:
      Serial.println(epoch);  
  
      // print the hour, minute and second:
      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      Serial.print(':');
      if ( ((epoch % 3600) / 60) < 10 ) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ( (epoch % 60) < 10 ) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second 

      timeSuccess = true;      

      // Enable ticker every second
      ticker.attach(1, tick);
    }
    // wait x seconds before asking for the time again
    delay(1000);
  }  
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
