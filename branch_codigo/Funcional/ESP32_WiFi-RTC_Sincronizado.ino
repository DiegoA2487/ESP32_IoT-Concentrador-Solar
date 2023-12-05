/**
 * Resumen: Este código sincroniza el Reloj en Tiempo Real (RTC) de un ESP32 con un servidor NTP.
 *          Primero se configura la hora y la zona horaria del ESP32. Luego se espera un tiempo para
 *          que la sincronización se complete. Finalmente, se muestra por el puerto serie la fecha y
 *          hora actual del RTC.
 */

#include <WiFi.h>
#include <time.h>
#include <ESP32Time.h>

const char* ssid = "DIEGO-PC";
const char* password = "wifidiego";
const int ledPin = 2; // Asume que el LED2 está conectado al pin GPIO 2

// Configuración del servidor NTP
const char* ntpServer = "mx.pool.ntp.org";
const long gmtOffset_sec = (-6)*3600;  // GMT -6 para México
const int daylightOffset_sec = 3600;  // Horario de verano
ESP32Time rtc;

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);

    Serial.println("Prueba WIFI- Sincronización RTC con NTP");
    Serial.println("Estableciendo conexión");
    WiFi.begin(ssid, password);
    sincronizarRTC();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED){ 
        Serial.println("Desconectado");
    } 
    else {
    Serial.println("Conectando a Wi-Fi "+ String(ssid));
    }
    // Aquí va el resto del programa...
    Serial.println("Tmestamp: " + rtc.getDateTime());
    delay(6000);                                        // Un delay para evitar la verificación constante
}

void sincronizarRTC() {
    Serial.println("Sincronización de RTC con NTP en curso");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(6000); // Espera para que la hora se sincronice
    Serial.println("RTC sincronizado: " + rtc.getDateTime());
}

void blinkLed(int pin, int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}
