/*
  Este código para ESP32 gestiona la conexión Wi-Fi, sincroniza el tiempo con NTP y envía datos a la nube usando IFTTT. 
  Características principales:
  - Conexión a Wi-Fi y sincronización de tiempo con un servidor NTP.
  - Almacenamiento de datos de muestreo en LittleFS cuando no hay conexión Wi-Fi.
  - Envío de datos almacenados y en tiempo real a un servidor remoto mediante IFTTT.
  - Uso de la clase RegistroData para manejar datos como temperatura, humedad y otros parámetros relevantes.
  - Indicación del estado de las operaciones a través de un LED.
  Ideal para proyectos que requieren recolección de datos de sensores, almacenamiento local y envío remoto para procesamiento o visualización.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "LittleFS.h"
#include <time.h>
#include <ESP32Time.h>

// Configuración de Wi-Fi
const char *ssid = "BISONProDiego"; //"DIEGO-PC"; //Para la PC
const char *password = "wifidiego";
const int ledPin = 2;       // LED para indicar el estado de la conexión Wi-Fi
unsigned long contador = 0; // Contador que incrementa en cada ciclo

// Configuración del servidor NTP
const char* ntpServer = "mx.pool.ntp.org";
const long gmtOffset_sec = (-6)*3600;  // GMT -6 para México
const int daylightOffset_sec = 3600;  // Horario de verano
ESP32Time rtc;

//Configuración de API IFTTT para el envío de datos
String server = "http://maker.ifttt.com";
String eventName = "temp";
String IFTTT_Key = "bdClU_zgRe2_ERMeSOqEVf";
String IFTTTUrl="https://maker.ifttt.com/trigger/temp/json/with/key/bdClU_zgRe2_ERMeSOqEVf";

// Definiciones para el muestreo
const int tiempoDeMuestreo = (0.25)*60*1000;                  // Tiempo de muestreo: (n) minutos

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


void setup()
{
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);

    // Inicializa el sistema de archivos LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("Error al montar el sistema de archivos LittleFS");
        return;
    }
    if (LittleFS.exists("/datos.csv"))
    {
        LittleFS.remove("/datos.csv");
        Serial.println("Archivo /datos.csv removido");
    }

    // Intenta conectar al Wi-Fi al iniciar
    conectarWiFi();
    sincronizarRTC(); // Sincroniza la hora con el RTC 
}

void loop()
{
    String data = String(contador)+ "> " + registroData.toCSVString(); // Convierte el objeto RegistroData en un string CSV
    registroData.timestamp = String(rtc.getDateTime());
    // Verifica el estado de la conexión Wi-Fi
    if (WiFi.status() != WL_CONNECTED)
    {
        guardarDatosLocalmente(data); // Guarda el contador en LittleFS si no está conectado
    }
    else
    {
        enviarDatos(data); // Envía datos a la consola cuando está conectado
    }
    contador++;  // Incrementa el contador
    delay(tiempoDeMuestreo); // Espera 5 segundos antes del próximo ciclo
}

// Conecta el dispositivo a la red Wi-Fi
void conectarWiFi()
{
    blinkLed(ledPin, 5, 500); // Parpadea el LED mientras intenta conectar
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        digitalWrite(ledPin, !digitalRead(ledPin)); // Continúa parpadeando
    }
    digitalWrite(ledPin, HIGH); // Enciende el LED al conectar
    Serial.println("Conectado a Wi-Fi");
}

void sincronizarRTC() {
    Serial.println("Sincronización de RTC con NTP en curso");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(6000); // Espera para que la hora se sincronice
    Serial.println("RTC sincronizado: " + rtc.getDateTime());
}

// Guarda datos en el sistema de archivos LittleFS
void guardarDatosLocalmente(String dataString)
{
    File archivoEscritura = LittleFS.open("/datos.csv", "a");
    if (!archivoEscritura)
    {
        Serial.println("Error al abrir el archivo para escribir");
        return;
    }
    else
    {
        archivoEscritura.println(dataString); // Escribe el contador en el archivo
        archivoEscritura.close();             // Cierra el archivo
        Serial.println(">LittleFs:" + dataString);
        //delay(100);
    }
}

// Envía los datos almacenados y el contador actual a la consola
void enviarDatos(String dataString)
{
    if (!LittleFS.exists("/datos.csv")){
        Serial.println("Archivo /datos.csv no existe, envio directo");
        sendDataToSheet(dataString);
        return;
    }
    else{
        File archivo = LittleFS.open("/datos.csv", "r");

        Serial.println("Enviando datos almacenados:"); // Resto de tu lógica para leer y enviar datos
        while (archivo.available() ){
            String dataArchivo = archivo.readStringUntil('\n');
            if (dataArchivo.length() > 0){ // Verifica si la cadena leída tiene contenido
                //Serial.println("[" + dataArchivo + "]");
                dataArchivo.trim();                 // Elimina los espacios en blanco //O: sendDataToSheet(dataArchivo.substring(0, dataArchivo.length() - 1));
                sendDataToSheet(dataArchivo);
                delay(5000);        //Tiempo suficiente para que la Gsheet actualice su script
            }
            if (!archivo.available()) { // Si ya no hay más datos
                Serial.println("Fin del archivo alcanzado.");
                break;
            }
        }
        Serial.println("Eliminando archivo....");                  // Envía el valor actual del contador
        archivo.close();                                        // Cierra el archivo
        LittleFS.remove("/datos.csv");                          // Opcional: elimina el archivo después de enviar los datos
    } 
}

//Método GET
/* void sendDataToSheet(String dataString)
{
    // Construir la URL con los datos a enviar
    String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + dataString;  

    // Iniciar una conexión HTTP
    HTTPClient http;

    http.begin(url); //HTTP

    int httpCode = http.GET();
  
    // Verificar si la solicitud fue exitosa
    if(httpCode > 0) {
        Serial.printf("> code: %d :", httpCode);
        // Si la respuesta del servidor es OK, imprimir el payload
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
        }
    } else {
        // Si la solicitud falla, imprimir el error
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    // Terminar la conexión HTTP
    http.end();
} */

//Método POST
void sendDataToSheet(String dataString) {
    HTTPClient http;
    String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key;

    // Datos a enviar
    String payload = "value1=" + dataString;

    // Iniciar una conexión HTTP
    http.begin(url); //HTTP

    // Establecer el tipo de contenido de la solicitud
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Enviar una solicitud POST con los datos
    Serial.print("[HTTP] POST...");
    int httpCode = http.POST(payload);

    // Verificar si la solicitud fue exitosa
    if(httpCode > 0) {
        Serial.printf("> code: %d :", httpCode);
        if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
            String response = http.getString();
            Serial.println(response);
        }
    } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    // Terminar la conexión HTTP
    http.end();
}



// Función para controlar el parpadeo del LED
void blinkLed(int pin, int times, int delayTime)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}
