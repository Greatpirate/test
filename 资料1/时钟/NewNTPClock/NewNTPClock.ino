#include <ESP8266WiFi.h>
#include <Time.h>
#include <Timezone.h>
#include "NTP.h"
#define WIFI_SSID "yourID"       // 使用时请修改为当前你的 wifi ssid
#define WIFI_PASS "yourPass"   // 使用时请修改为当前你的 wifi 密码

// 北京时间时区
#define STD_TIMEZONE_OFFSET +8    // Standard Time offset (-7 is mountain time)

TimeChangeRule mySTD = {"", First,  Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);
WiFiClient client;

#include <TM1637.h>
#define CLK D5//pins definitions for TM1637 and can be changed to other ports       
#define DIO D6
TM1637 tm1637(CLK, DIO);
bool secondFlash = true;
// This function is called once a second
void updateDisplay(void) {
  TimeChangeRule *tcr;        // Pointer to the time change rule
  // Read the current UTC time from the NTP provider
  time_t utc = now();
  // Convert to local time taking DST into consideration
  time_t localTime = myTZ.toLocal(utc, &tcr);

  int weekdays =   weekday(localTime);
  int days    =   day(localTime);
  int months  =   month(localTime);
  int years   =   year(localTime);
  String date = pressNum(years) + '-' + pressNum(months) + '-' + pressNum(days);

  int seconds =   second(localTime);
  int minutes =   minute(localTime);
  int hours   =   hour(localTime) ;   //12 hour format use : hourFormat12(localTime)  isPM()/isAM()
  String times = pressNum(hours) + pressNum(minutes) +  pressNum(seconds);
  Serial.println( date + "  " + times);

  tm1637.display(0, int(times[0]) - 48);
  tm1637.display(1, int(times[1]) - 48);
  tm1637.display(2, int(times[2]) - 48);
  tm1637.display(3, int(times[3]) - 48);


  if (secondFlash)
    tm1637.point(POINT_ON);
  else
    tm1637.point(POINT_OFF);
  secondFlash = !secondFlash;

}



String pressNum(int num) {

  if (num < 10 )
    return "0" + String(num);
  else
    return String(num);
}

void setup() {
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

  Serial.begin(115200);
  // pinMode(ledPin, OUTPUT);
  delay(10);
  // We start by connecting to a WiFi network
  initNTP(WIFI_SSID, WIFI_PASS);
}

// Previous seconds value
time_t previousSecond = 0;

void loop() {
  //  Update the display only if time has changed
  if (timeStatus() != timeNotSet) {
    if (second() != previousSecond) {
      previousSecond = second();
      // Update the display
      updateDisplay();
    }
  }
  delay(1000);
}
