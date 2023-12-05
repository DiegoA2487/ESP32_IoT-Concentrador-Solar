/*
  Este programa controla el ciclo de sueño/vigilia de un ESP32 usando un sensor LDR y RTC sincronizado con NTP.
  
  - Se conecta a Wi-Fi y sincroniza el RTC con NTP en el setup().
  - Usa rangos horarios para determinar amanecer y anochecer, combinando la hora (RTC) y la luz (LDR).
  - Entra en modo de sueño profundo durante la noche y se despierta con luz (por la mañana).
  - 'esNoche()' verifica si es hora de dormir basado en la hora y el estado del LDR.
  - Principalmente opera en 'setup()', con lógica mínima en 'loop()'.

  Ideal para aplicaciones que operan solo de día o para ahorro de energía nocturno.
*/


#include <WiFi.h>
#include <time.h>
#include <ESP32Time.h>

// Configuración de Wi-Fi
const char* ssid = "DIEGO-PC";
const char* password = "wifidiego";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -21600;  // GMT -6 para México
const int daylightOffset_sec = 3600;  // Horario de verano

// Configuración de pines
const gpio_num_t ldrPin = GPIO_NUM_15;  // Pin LDR para detectar día/noche
const int ledPin = 2;                   // Pin del LED para indicadores

// Crear objeto ESP32Time
ESP32Time rtc;

// Definir rangos de horas para amanecer y anochecer
const int horaInicioAmanecer = 6;  // 6 AM
const int horaFinAmanecer = 8;     // 8 AM
const int horaInicioAnochecer = 18; // 6 PM
const int horaFinAnochecer = 20;    // 8 PM

void setup() {
    Serial.begin(115200);
    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);

    // Conexión Wi-Fi y sincronización NTP
    conectarWiFi();
    sincronizarRTC();

    // Configurar despertar por interrupción del LDR
    esp_sleep_enable_ext0_wakeup(ldrPin, LOW);

    digitalWrite(ledPin, HIGH);
    Serial.println("ESP32 en modo de ejecución normal.");
}

void loop() {
    // Comprobar si es de noche o de día
    if (esNoche()) {
        Serial.println("Es de noche, entrando en modo de sueño profundo.");
        sleepingLed();
        esp_deep_sleep_start();
    } else if(!esNoche()){
        Serial.println("Es de día");
        Serial.println("🤖 :Estoy despierto");
    }
    Serial.println("Haciendo cualquier cosa...");
    delay(2000);
}

void conectarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Conectado a Wi-Fi");
}

void sincronizarRTC() {
    Serial.println("Sincronización de RTC con NTP en curso");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //rtc.setTimeStruct(time(NULL)); // Sincronizar el RTC
    delay(4000); // Espera para que la hora se sincronice
    Serial.println("RTC sincronizado: " + rtc.getDateTime());
}

bool esNoche() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return false;
    }
    int horaActual = timeinfo.tm_hour;

    // Comprobar si es hora de noche y el LDR indica oscuridad
    if ((horaActual >= horaInicioAnochecer && horaActual <= horaFinAnochecer) &&
        digitalRead(ldrPin) == HIGH) {
        Serial.println("Es de noche; LDR indica oscuridad");
        Serial.print("Hora actual: ");
        Serial.println(horaActual);
        return true;
    }
    // Comprobar si es hora de mañana y el LDR indica luz
    if ((horaActual >= horaInicioAmanecer && horaActual <= horaFinAmanecer) &&
        digitalRead(ldrPin) == LOW) {
        Serial.println("Es de día; LDR indica luz");
        Serial.print("Hora actual: ");
        Serial.println(horaActual);
        return false;
    }

    // Por defecto, asumir que no es noche
    return false;
}

void sleepingLed() {
    // Efecto visual de "dormirse" del LED
    for (int i = 255; i >= 0; i--) {
        analogWrite(ledPin, i);
        delay(15);
    }
}
