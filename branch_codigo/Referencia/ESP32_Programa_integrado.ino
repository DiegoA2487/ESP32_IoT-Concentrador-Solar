#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"
#include <Wire.h>
#include <SPI.h>
#include <time.h>
#include <ESP32Time.h>
#include "FS.h"
#include <LittleFS.h>

//++++++++++++++++++++++++++++++++++++ DEFINICIONES +++++++++++++++++++++++++
//      ++++++++++++++++++  1. SENSORES Y PERIFERICOS  ++++++++
// Definiciones para MAX6675 [Termopar]
#define MAX6675_SO  19                                // Pin de salida de datos
#define MAX6675_CS  5                                 // Chip Select para la comunicación SPI
#define MAX6675_SCK 18                                // Reloj del bus SPI

// Definiciones para HT11
DHTesp dht;                                          // Instancia de sensor DHT
const int pinDHT = 23;                               // Pin para el sensor DHT

// Definiciones para GPIOs (LDR:Noche y dia = 15)
const gpio_num_t ldrPin = GPIO_NUM_15;               // Pin para el sensor LDR, detecta día/noche
const int ledPin = 2;                                // Pin del LED

//      ++++++++++++++++  2. CONECTIVIDAD  ++++++++
// Definiciones para conexión WIFI
const char* ssid = "DIEGO-PC";                       // Nombre de la red WiFi
const char* password = "wifidiego";                  // Contraseña de la red WiFi

// Definiciones para enviar dato a la nube a traves de API
String server = "http://maker.ifttt.com";
String eventName = "temp";
String IFTTT_Key = "bdClU_zgRe2_ERMeSOqEVf";
String IFTTTUrl = "https://maker.ifttt.com/trigger/temp/json/with/key/"
                  "bdClU_zgRe2_ERMeSOqEVf";          // URL para enviar datos a IFTTT

// Definiciones para sincronizar RTC con NTP
const char* ntpServer = "pool.ntp.org";              // Servidor NTP
const long gmtOffset_sec = -21600;                   // GMT -6 para México
const int daylightOffset_sec = 3600;                 // Ajuste de horario de verano si aplica
ESP32Time rtc;                                       // Instancia para manejar tiempo real

//      ++++++++++++++++ 3. VARIABLES  ++++++++
// Definiciones para el muestreo
const int tiempoDeMuestreo = (1)*60*1000;                  // Tiempo de muestreo: (1) minuto

// Estructura para guardar las variables por muestra
class RegistroData {
  public:
    float Tc;                                        // Temperatura del colector
    float Ta;                                        // Temperatura ambiental
    float Ha;                                        // Humedad relativa del ambiente
    float R;                                         // Radiación
    float Fl;                                        // Flujo másico del líquido
    float E;                                         // Eficiencia del concentrador solar
    String timestamp;                                // Marca de tiempo de la muestra

    RegistroData() : Tc(0), Ta(0), Ha(0), R(0), Fl(0), E(0), timestamp("") {}

    void updateData(float tempColector, float tempAmb, float humAmb, float radiacion, 
                    float flujoLiquido, float eficiencia, String time) {
      Tc = tempColector;
      Ta = tempAmb;
      Ha = humAmb;
      R = radiacion;
      Fl = flujoLiquido;
      E = eficiencia;
      timestamp = time;
    }

    String toCSVString() {
      // Formatea los datos en un string CSV
      return timestamp + "," + String(R) + "," + String(Tc) + "," + String(Ta) + 
             "," + String(Ha) + "," + String(Fl) + "," + String(E);
    }
};

RegistroData registroData;                           // Instancia de RegistroData para almacenar y gestionar datos de muestreo

//++++++++++++++++++++++++++++++++++++ /end DEFINICIONES +++++++++++++++++++++++++

/* °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° PROGRAMA °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°*/
void setup() {
  Serial.begin(115200);                             // Inicia la comunicación serial
  dht.setup(pinDHT, DHTesp::DHT11);                 // Configura el sensor DHT11

  // Inicializa y verifica el sistema de archivos LittleFS
  if (!LittleFS.begin(true)) {  
    Serial.println("Error al montar el sistema de archivos LittleFS");
    return;
  }

  // Configuración y conexión a la red WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    blinkLed(1, 500); // Parpadea mientras intenta conectar
  }
  Serial.println("Conectado!!!");
  digitalWrite(ledPin, HIGH); // Enciende el LED al conectarse

  // Sincronización NTP del RTC (reloj de tiempo real)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("RTC sincronizado: " + rtc.getDateTime());
  blinkLed(4, 500);                                  // Indica la sincronización con destellos

  // Configuración de pines
  pinMode(ldrPin, INPUT);
  pinMode(ledPin, OUTPUT);
  esp_sleep_enable_ext0_wakeup(ldrPin, 1);           // 1 = Alto para despertar
}

void loop() {
  // Verifica si es de noche para entrar en modo hibernación
  if (digitalRead(ldrPin) == LOW) {
    Serial.println("Es de noche, entrando en modo de hibernación.");
    sleepingLed();  // Efecto de dormirse antes de hibernar
    esp_deep_sleep_start();
  }

  // Recopila datos de los sensores y actualiza la estructura RegistroData
  TempAndHumidity data = dht.getTempAndHumidity();
  registroData.updateData(leer_termopar(), data.temperature, data.humidity, 300 /* valor de radiación */, 50 /* valor de flujo másico del líquido */, 90 /* cálculo de eficiencia */, rtc.getDateTime());

  // Verifica la conexión WiFi y envía o almacena los datos localmente
  if (WiFi.status() == WL_CONNECTED) {
    enviarDatosAcumulados();
  } else {
    guardarDatosLocalmente(registroData.toCSVString());
  }

  blinkLed(2, 1000); // Dos parpadeos lentos por lectura de sensores
  delay(tiempoDeMuestreo);
}

/* uuuuuuuuuuuuuuuuuuuuuuuuuuu FUNCIONES LED uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu */
void blinkLed(int times, int delayTime) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(delayTime);
    digitalWrite(ledPin, LOW);
    delay(delayTime);
  }
}

void sleepingLed() {
  // Incrementa gradualmente el brillo
  for (int i = 0; i <= 255; i++) {
    analogWrite(ledPin, i);
    delay(15);
  }

  // Disminuye gradualmente el brillo
  for (int i = 255; i >= 0; i--) {
    analogWrite(ledPin, i);
    delay(15);
  }
}

// uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu   FUNCIONES DATA   uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu
void guardarDatosLocalmente(String dataString) {
  File archivo = LittleFS.open("/datos.csv", "a");
  if (!archivo) {
    Serial.println("Error al abrir el archivo para escribir");
    return;
  }
  archivo.println(dataString);
  archivo.close();
  Serial.println("Datos guardados localmente");
}

void enviarDatosAcumulados() {
  File archivo = LittleFS.open("/datos.csv", "r");
  if (!archivo) {
    Serial.println("No hay datos para enviar");
    return;
  }

  // Aquí deberías implementar la lógica para enviar los datos a tu servicio en la nube
  while (archivo.available()) {
    String data = archivo.readStringUntil('\n');
    Serial.println("Enviando dato: " + data);
    // Aquí iría el código para enviar 'data' a la nube
    sendDataToSheet();
  }

  archivo.close();
  // Borra el archivo después de enviar los datos
  LittleFS.remove("/datos.csv");
  Serial.println("Datos enviados y archivo local borrado");
}

void sendDataToSheet(void)
{
  //String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + String(String(value1)+","+String(value2)+","+String(value3)) + "&value2="+String((float)value2)+ "&value3="+String((float)value3);
  //String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + String("300,"+String(value3)+",50,50,"+String(value1)+","+String(value2)+","+String(eficiencia)+",75,90,90");
  String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + registroData.toCSVString();
  Serial.println(url);
  //Start to send data to IFTTT
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  http.begin(url); //HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

}

// uuuuuuuuuuuuuuuuuuuuuuuuuuu    FUNCIONES SENSORES    uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu
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

// ... [Otras funciones necesarias] ...
