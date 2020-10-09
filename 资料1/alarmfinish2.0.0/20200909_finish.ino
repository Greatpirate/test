#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <Adafruit_ssd1306syp.h>
#include <Wire.h>
#include <RtcDS3231.h>
//#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
//引入dfplayer头文件
#include <DFRobotDFPlayerMini.h>
//定义软串口
#define DS3231_I2C_ADDRESS 0x68
SoftwareSerial mySoftwareSerial(2, 13); // RX, TX
//定义播放器实例
DFRobotDFPlayerMini myDFPlayer;


//DS3231  SCL-----D1,SDA-----D2

//OLED
#define SDA_PIN 14//oled_SDA
#define SCL_PIN 12//oled_SCL
Adafruit_ssd1306syp display(SDA_PIN, SCL_PIN);
#define KEY 0   //按键，GPIO0 NodeMCU Flash Key
//int ledPin = 13;
//boolean lock = true;


const char* host = "10.42.0.1"; //需要访问的域名
const int httpsPort = 80;  // 需要访问的端口
const String url = "http://10.42.0.1/creat1.php";   // 需要访问的地址
String postRequest = (String)("POST ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: Keep Alive\r\n" +
                     "Content-Length: 27\r\n" +
                     "Origin: http://" + host + "\r\n" +
                     "Content-Type: application/json;charset=UTF-8\r\n" +
                     "User-Agent: ESP8266\r\n" +
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n" +
                     "Accept-Encoding: gzip, deflate" + "\r\n" +
                     "Accept-Language: zh-CN,zh;q=0.9" + "\r\n" +
                     "\r\n" +
                     "{\"num\": \"5\", \"signid\": \"3\"}" +
                     "\r\n";
const unsigned long NtpInterval = 180 * 1000UL; //NTP自动校时间隔(毫秒)
const unsigned long HttpInterval = 3 * 1000UL; //NTP自动校时间隔(毫秒)
unsigned int keys = 0;    //按键检测结果
unsigned long lastNtpupdate = millis();    //NTP校时标志
unsigned long lasthttpupdate = millis();    //NTP校时标志
RtcDS3231<TwoWire> Rtc(Wire);

//WIFI
//NTP
#define time_zone 8   //时区（要格林尼治时间加减的小时，北京为东八区，要将格林威治时间+8小时）
unsigned int localPort = 2390;      // local port to listen for UDP packets
/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
//const char* ntpServerName = "ntp1.aliyun.com";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val / 10 * 16) + (val % 10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val / 16 * 10) + (val % 16) );
}


// 设备初始化的时候 AP信息

const IPAddress softLocal(192, 168, 128, 1);
const IPAddress softGateway(192, 168, 128, 1);
const IPAddress softSubnet(255, 255, 255, 0);
const char* apSSID = "pillsbox";
const char* apPWD = "12345678";
boolean settingMode;
WiFiServer server(80);



String readntpurl() {
  String ntpurl = "" ;
  if (EEPROM.read(96) != 0) {
    for (int i = 96; i < 192; ++i) {
      ntpurl += char(EEPROM.read(i));
    }
  }
  Serial.print("ntpurl: ");
  Serial.println(ntpurl);
  return ntpurl;
}



void setup() {
  Wire.begin(5,4);
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);
  //初始化player
  myDFPlayer.begin(mySoftwareSerial);
  //设置音量为25(0-30)
  myDFPlayer.volume(25);
  Rtc.Begin();
  //pinMode(ledPin, OUTPUT);
  EEPROM.begin(512);  // 初始化EERPOM缓存区大小
  Serial.println();
  delay(1000);
  display.initialize();//oled初始化
  display.setTextColor(WHITE);//设置oled文字颜色
  display.setTextSize(2);//设置oled文字大小
  display.setCursor(0, 24); //设置oled指针位置
  display.print("WiFi-Clock");//oled显示文字
  display.setTextSize(1);
  display.setCursor(62, 56);
  display.print("Power By Cy");
  display.update();
  // We start by connecting to a WiFi network
  delay(1500);
  display.clear();
  delay(10);
  if (restoreConfig()) {
    Serial.println("Existing Wifi...");
    if (checkConnection()) {
      Serial.println("Existing Wifi is ok!!!!");
      settingMode = false;
      startWebServer();
      return;
    } else {
      Serial.println("Existing Wifi No!!!!");
      settingMode = true;
      setupMode();
      //restoreConfig();
      //checkConnection();

    }
  } else {
    Serial.println("Existing Wifi No!!!!");
    settingMode = true;
    setupMode();

  }
  delay(10);
}

void loop() {

  if ((lastNtpupdate != millis() / NtpInterval)) {    //依据计时或按键（短按）进入NTP更新
    //keys = 0;
    //get a random server from the pool
    WiFi.hostByName((char*)readntpurl().c_str(), timeServerIP);
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);
    int cb = udp.parsePacket();
    if (!cb) {
      Serial.println("no packet yet");
      return;
    } else {
      //NTP
      //Serial.print("packet received, length=");
      //Serial.println(cb);
      // We've received a packet, read the data from it
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      //Serial.print("Seconds since Jan 1 1900 = " );
      //Serial.println(secsSince1900);
      // now convert NTP time into everyday time:
      //Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      //Serial.println(epoch);

      unsigned long Time = time_zone * 3600 + epoch;
      unsigned long Y2KTime = (Time - 946684800) / 86400;//从2000年开始的天数
      unsigned long YTime;//从今年开始的天数
      unsigned int Year;
      unsigned int Month = 0;
      unsigned long Day;

      //日期
      if (Y2KTime % 146097 <= 36525)
      {
        Year = 2000 + Y2KTime / 146097 * 400 + Y2KTime % 146097 / 1461 * 4 + (Y2KTime % 146097 % 1461 - 1) / 365;
        YTime = (Y2KTime % 146097 % 1461 - 1) % 365 + 1;
      }
      else
      {
        Year = 2000 + Y2KTime / 146097 * 400 + (Y2KTime % 146097 - 1) / 36524 * 100 + ((Y2KTime % 146097 - 1) % 36524 + 1) / 1461 * 4 + (((Y2KTime % 146097 - 1) % 36524 + 1) % 1461 - 1) / 365;
        YTime = (((Y2KTime % 146097 - 1) % 36524 + 1) % 1461 - 1) % 365 + 1;
      }
      Day = YTime;
      unsigned char f = 1; //循环标志
      while (f)
      {
        switch (Month)
        {
          case 0:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
          case 1:
            if (Day < 29)
              f = 0;
            else
            {
              if (LY(Year))
              {
                Day -= 29;
              }
              else
              {
                Day -= 28;
              }
            }
            break;
          case 2:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
          case 3:
            if (Day < 30)
              f = 0;
            else
              Day -= 30;
            break;
          case 4:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
          case 5:
            if (Day < 30)
              f = 0;
            else
              Day -= 30;
            break;
          case 6:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
          case 7:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
          case 8:
            if (Day < 30)
              f = 0;
            else
              Day -= 30;
            break;
          case 9:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
          case 10:
            if (Day < 30)
              f = 0;
            else
              Day -= 30;
            break;
          case 11:
            if (Day < 31)
              f = 0;
            else
              Day -= 31;
            break;
        }
        Month += 1;
      }
      Day += 1;
      //display.clear();
      //display.setCursor(0, 48);
      //display.setTextSize(2);
      //Serial.print("net time:");
      //Serial.print(Year);
      //Serial.print("/");
      //Serial.print(Month);
      //Serial.print("/");
      //Serial.println(Day);

      //星期
      /*switch (Y2KTime % 7) //2000年1月1日是星期六
        {
        case 0: display.drawBitmap(112, 48, weekData6, 16, 16, WHITE); break;
        case 1: display.drawBitmap(112, 48, weekData7, 16, 16, WHITE); break;
        case 2: display.drawBitmap(112, 48, weekData1, 16, 16, WHITE); break;
        case 3: display.drawBitmap(112, 48, weekData2, 16, 16, WHITE); break;
        case 4: display.drawBitmap(112, 48, weekData3, 16, 16, WHITE); break;
        case 5: display.drawBitmap(112, 48, weekData4, 16, 16, WHITE); break;
        case 6: display.drawBitmap(112, 48, weekData5, 16, 16, WHITE); break;
        }*/


      //时间
      //display.setCursor(0, 0);
      //display.setTextSize(4);
      // print the hour, minute and second:
      //Serial.print("The time is ");       // UTC is the time at Greenwich Meridian (GMT)
      //Serial.print((Time  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      //Serial.print(':');
      //if ( ((Time % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      //  Serial.print('0');
      //}
      //Serial.print((Time  % 3600) / 60); // print the minute (3600 equals secs per minute)
      //Serial.print(':');
      //if ( (Time % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      //  Serial.print('0');
      //}
      //Serial.println(Time % 60); // print the second
      // set the initial time here:
      // DS3231 seconds, minutes, hours, day, date, month, year
      setDS3231time(Time % 60, (Time  % 3600) / 60, (Time  % 86400L) / 3600, Day, Day, Month, Year - 2000);
      Serial.println("Time Update!!");
    }
    lastNtpupdate = millis() / NtpInterval;

  }

  displayTime(); // display the real-time clock data on the Serial Monitor
  delay(1000);
  alarm();
  scankey();    //  按键扫描



}
void alarm() {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  String clock1 = "";
  String nums = "";
  String clocks = "";
  int num = 0;
  if (EEPROM.read(200) != 0) {
    for (int i = 200; i < 208; ++i) {
      nums += char(EEPROM.read(i));
    }
    num = StringtoInt(nums);
    //Serial.println(nums);
  }
  for (int j = 1; j <= num; ++j) {
    if (EEPROM.read(208) != 0) {
      for (int i = 200 + j * 8; i < 200 + (j + 1) * 8; ++i) {
        clocks += char(EEPROM.read(i));

      }
      //Serial.println(clocks);
      int clockint = StringtoInt(clocks);
      if (clockint == (hour * 100 + minute)) {
        if (second == 1 || second == 2 || second == 3 )
          //digitalWrite(ledPin, HIGH);   // 引脚高电平，点亮LED
          myDFPlayer.enableLoopAll();
      }
      clocks = "";
    }






  }
}

void scankey()
{
  if (digitalRead(KEY) == 0) {
    myDFPlayer.disableLoopAll(); //停止循环所有mp3文件
    unsigned int Keytime = 0;
    while (!digitalRead(KEY)) {
      Keytime ++;
      delay(10);
    }
    //if (Keytime >= 300) keys = 2;
    if (Keytime >= 1 & Keytime < 300) {
      //keys = 1;
      //digitalWrite(ledPin, LOW);
      Serial.print("connecting to ");
      Serial.println(host);

      WiFiClient client;

      /**
         测试是否正常连接
      */
      if (!client.connect(host, httpsPort)) {
        Serial.println("connection failed");
        return;
      }
      delay(10);


      Serial.println(postRequest);
      client.print(postRequest);  // 发送HTTP请求

      /**
         展示返回的所有信息
      */
      String status_code = client.readStringUntil('\r');        //读取GET数据，服务器返回的状态码，若成功则返回状态码200
      Serial.println(status_code);
      int mark = StringtoInt(status_code);
      //Serial.println(mark);
      //String json_from_server = client.readStringUntil('\n'); //读取返回的JSON数据
      //Serial.println(json_from_server);
      if (mark == 11200) {
        if (client.find("\r\n\r\n") == 1)                         //跳过返回的数据头，直接读取后面的JSON数据，
        {
          String json_from_server = client.readStringUntil('\n'); //读取返回的JSON数据
          Serial.println(json_from_server);
          String nums = JsontoString(json_from_server, "clocknum");
          int num = StringtoInt(nums);
          //client.flush();
          //client.print(json_from_client);
          if (num == 0) {
            for (int i = 0; i < 200; ++i) {
              EEPROM.write(i, 0);
            }
            String ssid = JsontoString(json_from_server, "ssid");
            Serial.print("SSID: ");
            Serial.println(ssid);
            String pass = JsontoString(json_from_server, "pass");
            Serial.print("Password: ");
            Serial.println(pass);
            String ntpurl = JsontoString(json_from_server, "ntpurl");
            Serial.print("ntpurl: ");
            Serial.println(ntpurl);
            String timezone = JsontoString(json_from_server, "timezone");
            Serial.print("timezone: ");
            Serial.println(timezone);

            Serial.println("Writing SSID to EEPROM...");    //0~32位存ssid,32~96存pass，96~192存ntpurl,192~200存timezone.
            for (int i = 0; i < ssid.length() - 2; ++i) {
              EEPROM.write(i, ssid[i + 1]);
            }
            Serial.println("Writing Password to EEPROM...");
            for (int i = 0; i < pass.length() - 2; ++i) {
              EEPROM.write(32 + i, pass[i + 1]);
            }
            Serial.println("Writing ntpurl to EEPROM...");
            for (int i = 0; i < ntpurl.length() - 2; ++i) {
              EEPROM.write(96 + i, ntpurl[i + 1]);
            }
            Serial.println("Writing timezone to EEPROM...");
            for (int i = 0; i < timezone.length() - 2; ++i) {
              EEPROM.write(192 + i, timezone[i + 1]);
            }
            EEPROM.commit();
          } else {
            for (int i = 200; i < 512; ++i) {
              EEPROM.write(i, 0);
            }
            for (int i = 0; i < nums.length() - 2; ++i) {
              EEPROM.write(200 + i, nums[i + 1]);
            }

            for (int j = 0; j < num; j++) {
              String a = "";
              String b = "clock";
              a = b + j;
              String clockWrite = JsontoString(json_from_server, a );
              for (int i = 0; i < clockWrite.length() - 2; ++i) {
                EEPROM.write(208 + 8 * j + i, clockWrite[i + 1]);
              }
              Serial.println("Writing...");

            }
            EEPROM.commit();

          }
          Serial.println("Write EEPROM done!");
          delay(3000);

        }

      }



    }
  }

}





boolean restoreConfig() {
  display.setCursor(0, 0);
  Serial.println("Reading EEPROM...");
  display.println("Reading EEPROM...");
  display.update();
  display.clear();
  String ssid = "";
  String pass = "" ;
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));

    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    display.print("SSID: ");
    display.println(ssid);
    for (int i = 32; i < 96; ++i) {
      pass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(pass);
    display.print("Password: ");
    display.println(pass);
    display.update();
    WiFi.softAPdisconnect(true); // 关闭软AP模式
    delay(1000);
    WiFi.begin(ssid, pass);
    WiFi.hostname("pillsbox-STA");  // 修改设备名称
    return true;
  }
  else {
    Serial.println("Config not found.");
    display.println("Config not found.");
    display.update();
    display.clear();
    return false;
  }
}
// 连接wifi
boolean checkConnection() {
  int count = 0;
  display.setCursor(0, 36);
  Serial.print("Waiting for Wi-Fi connection");
  display.print("Waiting for Wi-Fi connection");
  display.update();

  while ( count < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      display.println("");
      Serial.println("Connected!");
      display.println("WiFi connected");
      return (true);
    }
    delay(500);
    Serial.print(".");
    display.print(".");
    display.update();

    count++;
  }
  display.clear();
  Serial.println("Timed out");
  display.setTextSize(1);//设置oled文字大小
  display.setCursor(0, 0); //设置oled指针位置
  display.println("Timed out");
  display.print("Starting Access Point at: ");
  display.println(apSSID);
  display.print("password:");
  display.print(apPWD);
  display.update();
  display.clear();
  return false;
}
// 开启WebServer
void startWebServer() {
  if (settingMode) {

    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    server.begin();
    while (true) {
      boolean restartmark = false;
      WiFiClient client = server.available();
      if (client)
      {
        Serial.println("\n[Client connected]");
        while (client.connected())
        {
          // read line by line what the client (web browser) is requesting
          if (client.available())
          {
            String line = client.readStringUntil('\r');
            Serial.print(line);
            if (client.find("\r\n\r\n") == 1)                         //跳过返回的数据头，直接读取后面的JSON数据，
            {
              String json_from_client = client.readStringUntil('\n'); //读取返回的JSON数据
              Serial.println(json_from_client);
              String sign = JsontoString(json_from_client, "sign");
              int signs = StringtoInt(sign);
              if (signs == 0) {
                String nums = JsontoString(json_from_client, "num");
                int num = StringtoInt(nums);
                //client.flush();
                //client.print(json_from_client);
                if (num == 0) {
                  for (int i = 0; i < 200; ++i) {
                    EEPROM.write(i, 0);
                  }
                  String ssid = JsontoString(json_from_client, "ssid");
                  Serial.print("SSID: ");
                  Serial.println(ssid);
                  String pass = JsontoString(json_from_client, "pass");
                  Serial.print("Password: ");
                  Serial.println(pass);
                  String ntpurl = JsontoString(json_from_client, "ntpurl");
                  Serial.print("ntpurl: ");
                  Serial.println(ntpurl);
                  String timezone = JsontoString(json_from_client, "timezone");
                  Serial.print("timezone: ");
                  Serial.println(timezone);

                  Serial.println("Writing SSID to EEPROM...");    //0~32位存ssid,32~96存pass，96~192存ntpurl,192~200存timezone.
                  for (int i = 0; i < ssid.length() - 2; ++i) {
                    EEPROM.write(i, ssid[i + 1]);
                  }
                  Serial.println("Writing Password to EEPROM...");
                  for (int i = 0; i < pass.length() - 2; ++i) {
                    EEPROM.write(32 + i, pass[i + 1]);
                  }
                  Serial.println("Writing ntpurl to EEPROM...");
                  for (int i = 0; i < ntpurl.length() - 2; ++i) {
                    EEPROM.write(96 + i, ntpurl[i + 1]);
                  }
                  Serial.println("Writing timezone to EEPROM...");
                  for (int i = 0; i < timezone.length() - 2; ++i) {
                    EEPROM.write(192 + i, timezone[i + 1]);
                  }
                  EEPROM.commit();
                } else {
                  for (int i = 200; i < 512; ++i) {
                    EEPROM.write(i, 0);
                  }
                  for (int i = 0; i < nums.length() - 2; ++i) {
                    EEPROM.write(200 + i, nums[i + 1]);
                  }

                  for (int j = 0; j < num; j++) {
                    String a = "";
                    String b = "clcok";
                    a = b + j;
                    //Serial.println(a);
                    String clockWrite = JsontoString(json_from_client, a );
                    for (int i = 0; i < clockWrite.length() - 2; ++i) {
                      EEPROM.write(208 + 8 * j + i, clockWrite[i + 1]);
                    }
                    Serial.println("Writing...");

                  }
                  EEPROM.commit();

                }
                client.println(prepareHtmlPage());
                Serial.println("Write EEPROM done!");
                delay(100); // give the web browser time to receive the data

                // close the connection:
                client.flush();
                // client.stop();
                //Serial.println("[Client disonnected]");
                break;
              } else {
                //Serial.print("Starting Web Server a[[[[[[[[[[[[[[[[[[[[");
                client.println(prepareHtmlPage2());
                break;

              }

            }
          }
        }
      }

    }
    //ESP.restart();

  } else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    Serial.println("Starting UDP");
    udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(udp.localPort());
  }
}
// 初始化开启自身AP
void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_AP);
  display.setCursor(0, 0);
  WiFi.softAPConfig(softLocal, softGateway, softSubnet);
  WiFi.softAP(apSSID, apPWD);
  Serial.print("Starting Access Point at \"");
  display.println("Starting Access Point at \"");
  display.println(apSSID);
  display.println("password:");
  display.print(apPWD);
  Serial.println("\"");
  startWebServer();

}

/*
  函数说明： 从Json格式的String中，截取对应键值
  输入：  String 数据帧 String 键名
  输出：  int 类型的 键值
  示例;
  String m=  "{\"ledmode\":2,\"cr\":ff,\"cg\":a,\"cb\":1}";
  int a=JsontoString(m,"ledmode");//分割调用
  结果： a=2;
*/
String JsontoString(String zifuchuan, String fengefu)
{
  fengefu = "\"" + fengefu + "\"";
  int weizhi_KEY; //找查的位置
  int weizhi_DH;
  String temps;//临时字符串
  weizhi_KEY = zifuchuan.indexOf(fengefu);//找到位置
  temps = zifuchuan.substring( weizhi_KEY + fengefu.length(), zifuchuan.length()); //打印取第一个字符
  weizhi_DH = weizhi_KEY + fengefu.length() + temps.indexOf(','); //找到位置
  if ( temps.indexOf(',') == -1) {
    // weizhi_DH = weizhi_KEY+fengefu.length()+ temps.indexOf('}');//找到位置
    weizhi_DH = zifuchuan.length() - 1;
  }
  temps = "";
  temps = zifuchuan.substring( weizhi_KEY + fengefu.length() + 1,  weizhi_DH); //打印取第一个字符
  Serial.println(temps);
  //Serial.println("。。。。。。");

  return temps;
}
/*
  说明 String 转 10进制对应的10进制数
  输入：  String
  输出：  int
  示例：
  1023  1023
  1    1
*/
int StringtoInt(String temps)
{
  int l = 0;
  int p = 1;
  for (int i = temps.length() - 1; i >= 0; i--) {
    if (temps[i] == '0' || temps[i] == '1' || temps[i] == '2' || temps[i] == '3' || temps[i] == '4' || temps[i] == '5' || temps[i] == '6' || temps[i] == '7' || temps[i] == '8' || temps[i] == '9')
    {
      l += (int)(temps[i] - '0') * p;
      p *= 10;

    }

  }
  //Serial.println(l);
  return  l;

}
// prepare a web page to be send to a client (web browser)
String prepareHtmlPage()
{
  String htmlPage =
    String("HTTP/1.1 200 OK\r\n") +
    "Content-Type: text/plain\r\n" +
    "Connection: close\r\n" +  // the connection will be closed after completion of the response
    "Refresh: 5\r\n" +  // refresh the page automatically every 5 sec
    "\r\n" +
    "ok" ;
  return htmlPage;
}

String prepareHtmlPage2()
{
  String htmlPage =
    String("HTTP/1.1 200 OK\r\n") +
    "Content-Type: text/plain\r\n" +
    "Connection: close\r\n" +  // the connection will be closed after completion of the response
    "Refresh: 5\r\n" +  // refresh the page automatically every 5 sec
    "\r\n" +
    "201911201514";
  return htmlPage;
}







// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress & address)
{
  //Serial.println("sending NTP packet...");
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
unsigned char LY(unsigned int y)//判断是否为闰年
{
  if (y % 400 == 0)
    return 1;
  if (y % 100 == 0)
    return 0;
  if (y % 4 == 0)
    return 1;
}
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
                   dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte * second,
                    byte * minute,
                    byte * hour,
                    byte * dayOfWeek,
                    byte * dayOfMonth,
                    byte * month,
                    byte * year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
                 &year);
  // send it to the serial monitor
  Serial.print(year);
  Serial.print("/");
  Serial.print(month);
  Serial.print("/");
  Serial.print(dayOfMonth);
  Serial.print(" ");
  Serial.print(hour);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute < 10)
  {
    Serial.print("0");
  }
  Serial.print(minute);
  Serial.print(":");
  if (second < 10)
  {
    Serial.print("0");
  }
  Serial.println(second);

  //Serial.print(" Day of week: ");
  display.clear();
  display.setCursor(0, 48);
  display.setTextSize(2);
  display.print(year);
  display.print("/");
  if (month < 10)
  {
    display.print('0');
  }
  display.print(month);
  display.print("/");
  if (dayOfMonth < 10)
  {
    display.print('0');
  }
  display.print(dayOfMonth);

  //星期
  /*switch (Y2KTime % 7) //2000年1月1日是星期六
    {
    case 0: display.drawBitmap(112, 48, weekData6, 16, 16, WHITE); break;
    case 1: display.drawBitmap(112, 48, weekData7, 16, 16, WHITE); break;
    case 2: display.drawBitmap(112, 48, weekData1, 16, 16, WHITE); break;
    case 3: display.drawBitmap(112, 48, weekData2, 16, 16, WHITE); break;
    case 4: display.drawBitmap(112, 48, weekData3, 16, 16, WHITE); break;
    case 5: display.drawBitmap(112, 48, weekData4, 16, 16, WHITE); break;
    case 6: display.drawBitmap(112, 48, weekData5, 16, 16, WHITE); break;
    }*/


  //时间
  display.setCursor(0, 0);
  display.setTextSize(4);
  // print the hour, minute and second:
  if (hour < 10)
  {
    display.print('0');
  }
  display.print(hour);
  display.print(':');
  if (minute < 10)
  {
    display.print('0');
  }
  display.print(minute);


  display.update();
}
/*switch (dayOfWeek) {
  case 1:
    Serial.println("Sunday");
    break;
  case 2:
    Serial.println("Monday");
    break;
  case 3:
    Serial.println("Tuesday");
    break;
  case 4:
    Serial.println("Wednesday");
    break;
  case 5:
    Serial.println("Thursday");
    break;
  case 6:
    Serial.println("Friday");
    break;
  case 7:
    Serial.println("Saturday");
    break;
  }*/
