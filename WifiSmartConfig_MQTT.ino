#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "FS.h"

#define DHTTYPE DHT11   // 设置库为DHT 11
#define DHTPIN 4 //温度使用 D2
#define IR_LED 14 //红外发射脚D5
const int pin  = 5;   //开关使用D1

String wifiSSID;//WIFISSID
String wifiPASS;//WIFI密码

const char* mqtt_server = "183.230.40.39";
const int mqttport  = 6002;   //10086端口号
const char* sbid = "26024974"; //1086设备ID
const char* cpid = "123509"; //产品ID
const char* jqxx = "sf123456"; //鉴权信息

long lastMsg = 0;
char msg[50];
int value = 0;
char tmp[28];
char d[3];
float h;//湿度
float t;//温度
//奥克斯空调开
uint16_t rawDataKTOpen[211] = {8924, 4514,  528, 1712,  554, 1666,  560, 536,  508, 588,  508, 568,  528, 588,  508, 1690,  532, 1712,  508, 1712,  508, 1714,  508, 1712,  510, 586,  510, 588,  508, 588,  510, 588,  508, 588,  508, 588,  508, 588,  508, 588,  508, 588,  508, 588,  508, 1712,  508, 1712,  508, 1712,  508, 588,  510, 586,  510, 588,  508, 588,  508, 588,  508, 588,  508, 590,  508, 588,  508, 588,  510, 586,  508, 570,  528, 564,  534, 588,  508, 1710,  510, 588,  508, 1712,  508, 590,  508, 568,  528, 588,  508, 588,  508, 590,  508, 588,  510, 586,  510, 588,  508, 588,  508, 588,  510, 588,  538, 558,  508, 588,  508, 588,  508, 588,  508, 588,  508, 588,  508, 588,  508, 588,  508, 588,  510, 586,  510, 588,  510, 564,  532, 588,  508, 564,  532, 588,  508, 588,  510, 588,  508, 568,  528, 588,  536, 562,  508, 588,  510, 588,  508, 588,  508, 568,  528, 566,  530, 588,  508, 1712,  508, 588,  508, 552,  546, 588,  508, 568,  530, 588,  508, 588,  508, 588,  510, 590,  508, 588,  538, 558,  508, 1712,  508, 588,  508, 1712,  508, 588,  508, 588,  508, 564,  532, 588,  508, 588,  510, 1712,  508, 1712,  508, 1660,  560, 1712,  508, 588,  508, 1712,  554, 1666,  508, 588,  534};  // UNKNOWN D90F3B05
//奥克斯空调关
uint16_t rawDataKTClose[211] = {8932, 4526,  516, 1704,  514, 1706,  516, 576,  554, 586,  480, 578,  518, 578,  552, 1666,  520, 1702,  552, 1668,  552, 1626,  562, 1696,  526, 572,  524, 648,  448, 576,  522, 574,  522, 576,  520, 572,  526, 574,  524, 578,  518, 560,  538, 576,  522, 1698,  556, 1664,  524, 1686,  536, 560,  536, 574,  524, 574,  520, 576,  522, 576,  518, 578,  520, 574,  524, 648,  448, 574,  524, 570,  524, 578,  518, 576,  522, 576,  520, 1684,  536, 574,  522, 1696,  524, 572,  526, 588,  510, 572,  526, 586,  510, 574,  522, 574,  524, 540,  556, 574,  522, 572,  524, 574,  522, 570,  526, 574,  522, 570,  526, 576,  524, 572,  522, 560,  536, 646,  450, 576,  522, 648,  448, 534,  564, 574,  522, 588,  508, 576,  520, 572,  524, 578,  518, 578,  518, 614,  482, 576,  520, 574,  522, 572,  526, 574,  522, 578,  520, 574,  520, 576,  520, 532,  566, 576,  522, 556,  540, 576,  522, 576,  520, 574,  524, 574,  522, 578,  520, 576,  552, 618,  448, 562,  534, 578,  518, 576,  552, 540,  524, 1700,  552, 516,  550, 1702,  518, 574,  522, 564,  534, 578,  518, 576,  522, 580,  518, 1702,  550, 1672,  516, 1700,  520, 1676,  544, 578,  518, 648,  450, 1742,  512, 526,  596};  // UNKNOWN 1E72A7F4



DHT dht(DHTPIN, DHTTYPE);
IRsend irsend(IR_LED);
WiFiClient espClient;
PubSubClient client(espClient);

//Wifi连接
void setup_wifi() {
  delay(10);
  //WiFi.begin();
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ssid:");
  Serial.println(wifiSSID.c_str());
  Serial.print("Connecting to password:");
  Serial.println(wifiPASS.c_str());
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(WiFi.status());
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//订阅消息回调
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //判断订阅主题设置GPIO
  String data;
  data = "";
  for (int i = 0; i < length; i++) {
    data = data + (char)payload[i];
  }
  Serial.print("data:");
  Serial.print(data);
  //判断第三方web发送格式：{"switch":"Open"}
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  if (!root.success()) {
    Serial.println("Json parseObject() failed");
    return;
  }
  String  cmd  = root["switch"];
  if (cmd == "Open") //如果收到打开灯信号
  {
    digitalWrite(pin, 0);
  } else {
    if (cmd == "Close")
    {
      digitalWrite(pin, 1);
    }
    else
    {
      if (cmd == "KTOpen")//判断空调
      {
        Serial.println("KTOpen");
        irsend.sendRaw(rawDataKTOpen, 211, 38);  // Send a raw data capture at 38kHz. 
      }
      else
      {
        if (cmd == "KTClose")
        {
          Serial.println("KTClose");
          irsend.sendRaw(rawDataKTClose, 211, 38);  // Send a raw data capture at 38kHz.
        }
        else
        {
          //判断其他
        }
      }
    }
  }
}

//物联网重连
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(sbid, cpid, jqxx)) {
      Serial.println("connected");
      client.subscribe("SuSbKz");//订阅设备控制
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//获取温度湿度并发布给10086
void sendwd() {
  // readHumidity() 这里是读取当前的湿度
  h = dht.readHumidity();
  // readTemperature() 读取当前的温度，单位C
  t = dht.readTemperature();
  int ih;
  ih = (int)h;
  int it;
  it = (int)t;
  snprintf(tmp, sizeof(tmp), "{\"wd\":%d,\"sd\":%d}", it, ih);
  uint16_t streamLen = strlen(tmp);
  d[0] = '\x03';
  d[1] = (streamLen >> 8);
  d[2] = (streamLen & 0xFF);
  snprintf(msg, sizeof(msg), "%c%c%c%s", d[0], d[1], d[2], tmp);
  client.publish("$dp", (uint8_t*)msg, streamLen + 3, false);
}

//这部分是判断有没有wifi密码之类的，如果没有就开启smartconfg等待配置密码，
//配置过程和小度之家有点类似，不过比小度之家方便那么一丢丢，直接在app上配
//输入wifi 密码就可以了，不用切换网络。
//sk=true强行进入smartconfig
//sk=false有密码就先联网
void smartConfig(bool sk)
{
  if (SPIFFS.begin()) {
    if (!SPIFFS.exists("wifi.txt") || sk) {
      WiFi.mode(WIFI_STA);
      delay(500);
      Serial.println("Wait for Smartconfig");
      WiFi.beginSmartConfig();
      while (1) {
        delay(500);
        Serial.print(".");
        if (WiFi.smartConfigDone()) {
          Serial.println("SPIFFS mounted.");
          File f = SPIFFS.open("wifi.txt", "w");
          if (!f) {
            Serial.println("file open failed");
          } else {
            String wificonfig = WiFi.SSID().c_str();
            wificonfig += ",";
            wificonfig += WiFi.psk().c_str();
            f.println(wificonfig);
            Serial.println("SmartConfig Success");
            f.close();
            break;
          }
        }
      }
    }
    else {
      Serial.println("open wifi.txt");
      File f = SPIFFS.open("wifi.txt", "r");
      wifiSSID = f.readString();
      wifiPASS = wifiSSID.substring(wifiSSID.indexOf(",") + 1, wifiSSID.length() - 2);
      wifiSSID = wifiSSID.substring(0, wifiSSID.indexOf(","));
      f.close();
    }
  }
  else
  {
    Serial.println("SPIFFS error");
    SPIFFS.end();
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(pin, OUTPUT); //设置控制脚为GPIO5输出模式
  smartConfig(false);
  setup_wifi();
  // 必须使用的begin()函数
  dht.begin();
  irsend.begin();
  client.setServer(mqtt_server, mqttport);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 5000) {//定时5秒上报一次温度数据
    lastMsg = now;
    sendwd();
  }
}

