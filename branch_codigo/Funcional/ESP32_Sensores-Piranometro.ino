#include <Arduino.h>

//      ++++++++++++++++++  1. SENSORES Y PERIFERICOS  ++++++++
// Definiciones para el piranómetro analógico
#define pinRADIACION  4     // Pin para el sensor de radiación (piranómetro)

// Definiciones para GPIOs
#define ledPin        2     // Pin del LED

// Definiciones para el muestreo
#define tiempoDeMuestreo 3000  // Tiempo de muestreo: 15 segundos

//°°°°°°°°°°°°°°°°°°°°°°°°°°°°°         PROGRAMA PRINCIPAL      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
}

void loop() {
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

void blinkLed(int pin, int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}
