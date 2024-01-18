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
volatile int pulsos = 0; // Contador de pulsos, modificado por la interrupción
bool midiendoFlujo = false; // Bandera para indicar si se está midiendo el flujo
unsigned long tiempoInicioMedicion; // Tiempo de inicio de la medición del flujo
unsigned long ultimoTiempoMuestreo = 0; // Tiempo del último muestreo

// Definiciones para GPIOs
#define ldrPin        16    // Pin para el sensor LDR, detecta día/noche
#define ledPin        2     // Pin del LED

// Definiciones para el muestreo
#define tiempoDeMuestreo 15000  // Tiempo de muestreo: 15 segundos

// CASO 1: Eficiencia según calor en material del punto focal
//Definiciones para el cálculo de eficiencia según el Calor en el punto focal
#define masaMaterialFocal  0.5                    // Masa del material en el punto focal, en kilogramos
#define calorEspecificoMaterial  0.9              // Calor específico del material en el punto focal, en J/kg°C

// CASO 2: Eficiencia según calor en liquido
// Definiciones para el cálculo de eficiencia
#define calorEspecificoLiquido  4.18    // Calor específico del líquido en el colector, en J/g°C

#define radioDiscoParabolico  0.41                // Radio del disco parabólico en metros (82 cm de diámetro)
#define areaDiscoParabolico  (PI * pow(radioDiscoParabolico, 2)) // Área del disco parabólico en m²
float temperaturaInicial = 25.0;                // Temperatura inicial del material en el punto focal, en °C


// Estructura para guardar las variables por muestra
class RegistroData {
public:
    float Tc;       // Temperatura del colector
    float Ta;       // Temperatura ambiental
    float Ha;       // Humedad relativa del ambiente
    float R;        // Radiación
    float Fl;       // Flujo másico del líquido
    float E;        // Eficiencia del concentrador solar**
    float Ti;       // Temperatura inicial en termopar**

    String timestamp; // Marca de tiempo de la muestra

    RegistroData() : Tc(0), Ta(0), Ha(0), R(0), Fl(0), E(0), timestamp("") {}

    void updateData(float tempColector, float tempAmb, float humAmb, float radiacion, 
                    float flujoLiquido, String time) {
      Ti= Tc; // Asigna la temperatura inicial a Ti
      Tc = tempColector;
      Ta = tempAmb;
      Ha = humAmb;
      R = radiacion;
      Fl = flujoLiquido;
      E = calcularEficienciaConTermopar(Ti, tempColector, radiacion);
      //CASO 2:
      //E = calcularEficienciaConLiquido(radiacion, flujoLiquido);         

      timestamp = time;
    }

    String toCSVString() {
      // Formatea los datos en un string CSV
      return timestamp + "," + String(R) + "," + String(Tc) + "," + String(Ta) + 
             "," + String(Ha) + "," + String(Fl) + "," + String(E);
    }

    float calcularEficienciaConTermopar(float tempInicial, float tempFinal, float irradiancia) {
        float deltaTemp = tempFinal - tempInicial;
        float energiaAbsorbida = masaMaterialFocal * calorEspecificoMaterial * deltaTemp;
        float energiaIncidente = irradiancia * areaDiscoParabolico; // Energía solar incidente en J

        return (energiaAbsorbida / energiaIncidente) * 100.0;
    }
/*CASO 2: 
    float calcularEficienciaConLiquido(float irradiancia, float flujoLiquido) {
        // Energía absorbida por el líquido (suponiendo un aumento de temperatura constante)
        float energiaAbsorbida = flujoLiquido * calorEspecificoLiquido * (Tc - Ta); // Tc y Ta deben ser la entrada y salida de temp del líquido
        float energiaIncidente = irradiancia * areaDiscoParabolico; // Energía solar incidente en J

        return (energiaAbsorbida / energiaIncidente) * 100.0;
    }
*/

};

RegistroData registroData; // Instancia de RegistroData para almacenar y gestionar datos de muestreo

// &&&&&&&&&&&&&&&&&&&&&&&&&&&&&        INTERRUPCIONES      &&&&&&&&&&&&&&&&&&&&&&&&&&&&& 
// Manejador de la interrupción para contar los pulsos del sensor de flujo
void IRAM_ATTR flujoInterrupcion() {
    if (midiendoFlujo) {
        pulsos++; // Incrementa solo si estamos en período de medición
    }
}

//°°°°°°°°°°°°°°°°°°°°°°°°°°°°°         PROGRAMA PRINCIPAL      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°
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
float leer_piranometro() {
    int lecturaADC = analogRead(pinRADIACION);
    float voltaje = (lecturaADC / 4095.0) * 5.0; // Asumiendo que el rango máximo del ADC es 5V
    float irradiacion = voltaje * (1800.0 / 5.0); // Conversión basada en la calibración del piranómetro
    return irradiacion;
}

// Realiza la lectura del sensor de flujo
float leer_flujoLiquido() {
    void iniciarMedicionFlujo();
    void finalizarMedicionFlujo();

    if (millis() - ultimoTiempoMuestreo > tiempoDeMuestreo) {
        ultimoTiempoMuestreo = millis();
        iniciarMedicionFlujo(); // Inicia la medición del flujo
    }

    if (midiendoFlujo && (millis() - tiempoInicioMedicion > 1000)) {
        finalizarMedicionFlujo(); // Finaliza la medición del flujo después de un segundo
    }
}
            // Inicia la medición del flujo
            void iniciarMedicionFlujo() {
                pulsos = 0; // Reinicia el contador de pulsos
                midiendoFlujo = true; // Comienza a medir
                tiempoInicioMedicion = millis(); // Marca el inicio de la medición
                attachInterrupt(digitalPinToInterrupt(pinFlujoLiquido), flujoInterrupcion, RISING); // Activa la interrupción
            }

            // Finaliza la medición del flujo y calcula el caudal
            void finalizarMedicionFlujo() {
                detachInterrupt(pinFlujoLiquido); // Desactiva la interrupción
                midiendoFlujo = false;
                float caudal = (pulsos / 7.5); // Calcula el caudal basado en los pulsos contados
                registroData.Fl= caudal;
                Serial.println("caudal: " + String(registroData.Fl));
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