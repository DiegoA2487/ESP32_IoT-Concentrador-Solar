//#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"
#include <Wire.h>
#include <SPI.h>


#define MAX6675_SO 19
#define MAX6675_CS 5
#define MAX6675_SCK 18
#define Radiacion_Solar 300

int pinDHT = 23;
DHTesp dht;
//our sensor is DHT11 type
//#define DHTTYPE DHT11
//DHT dht(DHTPIN, DHTTYPE);

const char * ssid = "DIEGO-PC";
const char * password = "wifidiego";

String server = "http://maker.ifttt.com";
String eventName = "temp";
String IFTTT_Key = "bdClU_zgRe2_ERMeSOqEVf";
String IFTTTUrl="https://maker.ifttt.com/trigger/temp/json/with/key/bdClU_zgRe2_ERMeSOqEVf";

float value1;
float value2;
float value3;
float eficiencia=70.0;

void setup() {
  Serial.begin(115200);
  dht.setup(pinDHT,DHTesp::DHT11);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Viola, Connected !!!");
}



void sendDataToSheet(void)
{
  //String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + String(String(value1)+","+String(value2)+","+String(value3)) + "&value2="+String((float)value2)+ "&value3="+String((float)value3); 
  String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + String("300,"+String(value3)+",50,50,"+String(value1)+","+String(value2)+","+String(eficiencia)+",75,90,90");  
  Serial.println(url);
  //Start to send data to IFTTT
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  http.begin(url); //HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

}

void loop() {
  
  TempAndHumidity data = dht.getTempAndHumidity();
  value1 = data.temperature;
  value2 = data.humidity;
  float temperatura = leer_termopar();
  value3= temperatura;

  Serial.print("Values are ");
  Serial.print(value1);
  Serial.print(' ');
  Serial.print(value2);
  Serial.println(' ');
  Serial.print("La temperatura es: ");
  Serial.println(temperatura);


  sendDataToSheet();
  delay(20000);
}

double leer_termopar()
{

    uint16_t v;
    pinMode(MAX6675_CS, OUTPUT);
    pinMode(MAX6675_SO, INPUT);
    pinMode(MAX6675_SCK, OUTPUT);

    digitalWrite(MAX6675_CS, LOW);
    delay(1);

    v = shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);
    v <<= 8;
    v |= shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);

    digitalWrite(MAX6675_CS, HIGH);
    if (v & 0x4)
    {
        // Bit 2 indicates if the thermocouple is disconnected
        return NAN;
    }

    // The lower three bits (0,1,2) are discarded status bits
    v >>= 3;

    // The remaining bits are the number of 0.25 degree (C) counts
    return v * 0.25;
}
