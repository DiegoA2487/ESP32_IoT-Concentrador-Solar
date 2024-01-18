/*
  Este programa para ESP32 se enfoca en la recolección y procesamiento de datos ambientales y de rendimiento de un concentrador solar. Las características principales incluyen:

  - Lectura de datos de temperatura y humedad a través del sensor DHT.
  - Medición de la temperatura del colector solar utilizando un termopar MAX6675.
  - Medición de la radiación solar con un piranómetro analógico y la medición del flujo másico del líquido.
  - Cálculo de la eficiencia del concentrador solar en base a los datos recopilados.

  Este código está diseñado para ser utilizado en aplicaciones relacionadas con la energía solar y la monitorización ambiental, proporcionando una base para la recopilación de datos precisa y en tiempo real.
*/

#include <Arduino.h>
#include "DHTesp.h"
#include <Wire.h>
#include <SPI.h>

//      ++++++++++++++++++  1. SENSORES Y PERIFERICOS  ++++++++
// Definiciones para MAX6675 [Termopar]
#define MAX6675_SO   19     // Pin de salida de datos
#define MAX6675_CS   5      // Chip Select para la comunicación SPI
#define MAX6675_SCK  18     // Reloj del bus SPI

// Definiciones para HT11 [Sensor de Temp y Humedad]
DHTesp dht;                 // Instancia de sensor DHT
#define pinDHT        23    // Pin para el sensor DHT

// Definiciones para el piranómetro analógico
#define pinRADIACION  4     // Pin para el sensor de radiación (piranómetro)

// Definiciones para sensor de flujo másico del líquido
#define pinFlujoLiquido 15  // Pin para el sensor de flujo másico del líquido

// Definiciones para GPIOs
#define ldrPin        16    // Pin para el sensor LDR, detecta día/noche
#define ledPin        2     // Pin del LED

// Definiciones para el muestreo
#define tiempoDeMuestreo 15000  // Tiempo de muestreo: 15 segundos

// Estructura para guardar las variables por muestra
class RegistroData {
public:
    float Tc;       // Temperatura del colector
    float Ta;       // Temperatura ambiental
    float Ha;       // Humedad relativa del ambiente
    float R;        // Radiación
    float Fl;       // Flujo másico del líquido
    float E;        // Eficiencia del concentrador solar
    String timestamp; // Marca de tiempo de la muestra

    RegistroData() : Tc(0), Ta(0), Ha(0), R(0), Fl(0), E(0), timestamp("") {}

    void updateData(float tempColector, float tempAmb, float humAmb, float radiacion, 
                    float flujoLiquido, String time) {
      Tc = tempColector;
      Ta = tempAmb;
      Ha = humAmb;
      R = radiacion;
      Fl = flujoLiquido;
//      E = calcularEficiencia(); // Calcula la eficiencia del concentrador solar
      timestamp = time;
    }

    String toCSVString() {
      // Formatea los datos en un string CSV
      return timestamp + "," + String(R) + "," + String(Tc) + "," + String(Ta) + 
             "," + String(Ha) + "," + String(Fl) + "," + String(E);
    }

/*     float calcularEficiencia() {
      // Implementa el cálculo de la eficiencia del concentrador solar aquí
      return (R * Fl) / (Tc + Ta); // Fórmula de ejemplo
    } */
};

RegistroData registroData; // Instancia de RegistroData para almacenar y gestionar datos de muestreo

void setup() {
    Serial.begin(115200);
    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);
    dht.setup(pinDHT, DHTesp::DHT11);
}

void loop() {
    TempAndHumidity data = dht.getTempAndHumidity();
    registroData.updateData(leer_termopar(), data.temperature, data.humidity, leer_piranometro(), leer_flujoLiquido(), "fecha y hora");
    Serial.println(registroData.toCSVString());
    delay(tiempoDeMuestreo);
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ FUNCIONES DE SENSORES $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//! falta agregar calculos para conocer la radiación
float leer_piranometro() {
    return analogRead(pinRADIACION);
}

float leer_flujoLiquido() {
    return analogRead(pinFlujoLiquido);
}

double leer_termopar() {
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
    if (v & 0x4) {
        return NAN; // Indica desconexión del termopar
    }

    v >>= 3; // Descarta los bits de estado
    return v * 0.25; // Calcula la temperatura
}

void blinkLed(int pin, int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}
