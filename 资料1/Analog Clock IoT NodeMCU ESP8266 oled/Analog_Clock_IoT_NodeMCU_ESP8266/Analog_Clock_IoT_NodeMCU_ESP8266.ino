/**********************************************************************
 //  项目：网络时钟  （图形表盘）
 //  硬件：适用于NodeMCU ESP8266 + SSD1306
 //  功能：连接WiFi后获取时间并在OLED屏上显示
 //  修改及注解：油管机器猫 bilibili UID:16872024
 //  日期：2019/09/20
 //  硬件连接说明：
 //  OLED  --- ESP8266
 //  VCC   --- 3V(3.3V)
 //  GND   --- G (GND)
 //  SDA   --- D2(GPIO4)
 //  SCL   --- D1(GPIO5)
**********************************************************************/
#include <ESP8266WiFi.h>
#include <time.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET LED_BUILTIN  //4
Adafruit_SSD1306 display(OLED_RESET);


const char* ssid = "<your wifi ssid>";  //ssid
const char* password = "your wifi password";  //ssid password
int ledPin = 13;  // LED连接到数字引脚13
/*
 * NOTE: Digital pin 13 is harder to use as a digital input than the other digital pins because it has an LED and resistor attached to it that's soldered to the board on most boards. 
 * If you enable its internal 20k pull-up resistor, it will hang at around 1.7V instead of the expected 5V because the onboard LED and series resistor pull the voltage level down, meaning it always returns LOW.
If you must use pin 13 as a digital input, set its pinMode() to INPUT and use an external pull down resistor.
 */

/*
//来自Perl的时间::时区 $ pc_timezones = array（ 
'GMT'=> 0，//格林威治标准杆
 'UTC'=> 0，//通用（协调）
 'WET'=> 0，//西欧 
'WAT'=> -1 * 3600，//西非
 'AT'=> -2 * 3600，//亚速尔群岛
 'NFT'=> -3 * 3600-1800，//纽芬兰
 'AST'=> -4 * 3600，//大西洋标准
 'EST'=> -5 * 3600，//东部标准
'CST'=> -6 * 3600，//中央标准
 'MST'=> -7 * 3600，// Mountain Standard
 'PST'=> -8 * 3600，//太平洋标准
 'YST'=> -9 * 3600，// Yukon Standard 
'HST'=> -10 * 3600，//夏威夷标准
 'CAT'=> -10 * 3600，//阿拉斯加中部
 'AHST'=> -10 * 3600，//阿拉斯加 - 夏威夷标准
 'NT'=> -11 * 3600，// Nome
'IDLW'=> -12 * 3600，// International Date Line West
 'CET'=> + 1 * 3600，//中欧
 'MET'=> + 1 * 3600，//中欧
 'MEWT'=> + 1 * 3600，//中欧冬季
 'SWT'=> + 1 * 3600，//瑞典冬季 
'FWT'=> + 1 * 3600，//法国冬天
 'EET'=> + 2 * 3600，//东欧，苏联1区
 'BT'=> + 3 * 3600，//巴格达，苏联2区
 'IT'=> + 3 * 3600 + 1800，//伊朗
 'ZP4'=> + 4 * 3600，// USSR Zone 3
 'ZP5'=> + 5 * 3600，// USSR Zone 4
'IST'=> + 5 * 3600 + 1800，//印度标准
 'ZP6'=> + 6 * 3600，// USSR Zone 5
 'SST'=> + 7 * 3600，//苏门答腊南部，苏联6区
 'WAST'=> + 7 * 3600，//西澳大利亚标准
 'JT'=> + 7 * 3600 + 1800，// Java
 'CCT'=> + 8 * 3600，//中国海岸，苏联7区
 'JST'=> + 9 * 3600，//日本标准，苏联8区
 'CAST'=> + 9 * 3600 + 1800，//中澳大利亚标准
 'EAST'=> + 10 * 3600，//东澳大利亚标准
 'GST'=> + 10 * 3600，//关岛标准，苏联9区
 'NZT'=> + 12 * 3600，//新西兰
 'NZST'=> + 12 * 3600，//新西兰标准
 'IDLE'=> + 12 * 3600 //国际日期线东 ）; docstore.mik.ua/orelly/webprog/pcook/ch03_12.htm
 */

int timezone = 8 * 3600;  //以北京时间设置为例。北京在东八区
int dst = 0;// 设置 日期摆动时间

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif



void setup() {

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Clear the buffer.
  display.clearDisplay();
  display.display();
  
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin,LOW);

  Serial.begin(115200);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  display.setCursor(0,0);
  display.println("Wifi connecting to ");
  display.println( ssid );

  WiFi.begin(ssid,password);
 
  display.println("\nConnecting");

  display.display();

  while( WiFi.status() != WL_CONNECTED ){
      delay(500);
      display.print("."); 
      display.display();       
  }

  // Clear the buffer.
  display.clearDisplay();
  display.display();
  display.setCursor(0,0);
  
  display.println("Wifi Connected!");
  display.print("IP:");
  display.println(WiFi.localIP() );

  display.display();

  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
  display.println("\nWaiting for NTP...");

  while(!time(nullptr)){
     Serial.print("*");
     
     delay(1000);
  }
  display.println("\nTime response....OK"); 
  display.display();  
  delay(2000);

  display.clearDisplay();
  display.display();
  display.drawCircle(60,30,30,WHITE);//笑脸
  display.fillCircle(50,20,5,WHITE);
  display.fillCircle(70,20,5,WHITE);
  display.setTextSize(1);
  display.setCursor(38,40);
  display.print("Doraemon");
  display.display();  
  delay(2000);

  display.clearDisplay();
  display.display();
}

void loop() {
  
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int r = 35;
  // Now draw the clock face 
  
  display.drawCircle(display.width()/2, display.height()/2, 2, WHITE);
  //
  //hour ticks
  for( int z=0; z < 360;z= z + 30 ){
  //Begin at 0° and stop at 360°
    float angle = z ;
    
    angle=(angle/57.29577951) ; //Convert degrees to radians
    int x2=(64+(sin(angle)*r));
    int y2=(32-(cos(angle)*r));
    int x3=(64+(sin(angle)*(r-5)));
    int y3=(32-(cos(angle)*(r-5)));
    display.drawLine(x2,y2,x3,y3,WHITE);
  }
  // display second hand
  float angle = p_tm->tm_sec*6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  int x3=(64+(sin(angle)*(r)));
  int y3=(32-(cos(angle)*(r)));
  display.drawLine(64,32,x3,y3,WHITE);
  //
  // display minute hand
  angle = p_tm->tm_min * 6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  x3=(64+(sin(angle)*(r-3)));
  y3=(32-(cos(angle)*(r-3)));
  display.drawLine(64,32,x3,y3,WHITE);
  //
  // display hour hand
  angle = p_tm->tm_hour * 30 + int((p_tm->tm_min / 12) * 6 );
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  x3=(64+(sin(angle)*(r-11)));
  y3=(32-(cos(angle)*(r-11)));
  display.drawLine(64,32,x3,y3,WHITE);

  display.setTextSize(1);
  display.setCursor((display.width()/2)+10,(display.height()/2) - 3);
  display.print(p_tm->tm_mday);
  
   // update display with all data
  display.display();
  delay(100);
  display.clearDisplay();

}
