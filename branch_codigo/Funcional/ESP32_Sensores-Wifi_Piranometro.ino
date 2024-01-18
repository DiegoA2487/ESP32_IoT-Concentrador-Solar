/*
  Programa que se conecta a a una red WiFi y mide la radiación solar.
  
  !OJO: ADC2 no funcionan con WIFI, de modo que se debe usar ADC1
  
  Autor: Diego
  Fecha: 2022.04.25
*/

#include <Arduino.h>
#include <WiFi.h>

//      ++++++++++++++++++  DEFINICIONES  ++++++++
// Definiciones para el piranómetro analógico
#define pinRADIACION  34     // Pin para el sensor de radiación (piranómetro)

// Definiciones para GPIOs
#define ledPin        2     // Pin del LED

// Definiciones para Wifi
const char* ssid = "BISONProDiego";
const char* password = "wifidiego";

// Definiciones para el muestreo
#define tiempoDeMuestreo 3000  // Tiempo de muestreo: 15 segundos

//°°°°°°°°°°°°°°°°°°°°°°°°°°°°°         PROGRAMA PRINCIPAL      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);

    Serial.println("Prueba WIFI- Conexión y verificación");

    Serial.println("Estableciendo conexión");
    conectarWiFi(); // Intentar conectar al iniciar
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Desconectado de servidor");
        conectarWiFi(); // Intentar reconectar si se pierde la conexión
    }

    Serial.println("Leyendo");
    leer_piranometro();
    delay(tiempoDeMuestreo);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ FUNCIONES DE SENSORES $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
float leer_piranometro() {
    int lecturaADC = analogRead(pinRADIACION);
    Serial.printf("\n LecturaADC : %i ", lecturaADC);
    float voltaje = (lecturaADC / 4095.0) * 3.33; // Asumiendo que el rango máximo del ADC es 3.3V
    Serial.printf("\n Voltaje : %0.2f ", voltaje);
    float irradiacion = voltaje * (1800.0 / 3.33); // Conversión basada en la calibración del piranómetro
    Serial.printf("\n Radiacion: %0.2f ", irradiacion);
    Serial.println("Fin lectura");
    return irradiacion;
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ OTRAS FUNCIONES $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
void conectarWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        digitalWrite(ledPin, !digitalRead(ledPin)); // Parpadeo continuo
    }
    Serial.println("Conectado a Wi-Fi");
    digitalWrite(ledPin, HIGH); // LED constante cuando está conectado
}

void blinkLed(int pin, int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}
