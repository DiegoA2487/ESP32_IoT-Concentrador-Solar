/*
  Este programa para ESP32 se enfoca en la recolección y procesamiento de datos ambientales y de rendimiento de un concentrador solar. Las características principales incluyen:

  - Lectura de datos de temperatura y humedad a través del sensor DHT.
  - Medición de la temperatura del colector solar utilizando un termopar MAX6675.
  - Medición de la radiación solar con un piranómetro analógico y la medición del flujo másico del líquido.
  - Cálculo de la eficiencia del concentrador solar en base a los datos recopilados.
  - manejar el ciclo de sueño y vigilia de un ESP32 en función de la luz ambiental, detectada a través de un sensor LDR. El propósito principal es ahorrar energía al poner el dispositivo en modo de sueño profundo durante la noche y despertarlo durante el día

  Este código está diseñado para ser utilizado en aplicaciones relacionadas con la energía solar y la monitorización ambiental, proporcionando una base para la recopilación de datos precisa y en tiempo real.
*/

#include <Arduino.h>
#include "DHTesp.h"
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>

//      ++++++++++++++++++  1. SENSORES Y PERIFERICOS  ++++++++
// Definiciones para MAX6675 [Termopar]
#define MAX6675_SO   19     // Pin de salida de datos
#define MAX6675_CS   5      // Chip Select para la comunicación SPI
#define MAX6675_SCK  18     // Reloj del bus SPI

// Definiciones para HT11 [Sensor de Temp y Humedad]
DHTesp dht;                 // Instancia de sensor DHT
#define pinDHT        23    // Pin para el sensor DHT

// Definiciones para el piranómetro analógico
#define pinRADIACION  34     // Pin para el sensor de radiación (piranómetro)

/*// Definiciones para sensor de flujo másico del líquido
#define pinFlujoLiquido 15  // Pin para el sensor de flujo másico del líquido
volatile int pulsos = 0; // Contador de pulsos, modificado por la interrupción
bool midiendoFlujo = false; // Bandera para indicar si se está midiendo el flujo
unsigned long tiempoInicioMedicion; // Tiempo de inicio de la medición del flujo
unsigned long ultimoTiempoMuestreo = 0; // Tiempo del último muestreo
*/

// Definiciones para GPIOs
#define ldrPin        GPIO_NUM_15    // Pin para el sensor LDR, detecta día/noche
#define ledPin        2     // Pin del LED

// Definiciones para Wifi
const char* ssid = "ESP8266_Mesh_Node";
const char* password = "password";
WiFiClient client;

// Definiciones para el muestreo
const unsigned long tiempoDeMuestreo = (5)*(60000);// Tiempo de muestreo: (min)*(60seg*1000ms) ; 1 hora > (60)*(60*1000)= (3600000 milisegundos)
unsigned long tiempoAnterior = 0; // Guarda el último momento en que se tomó una muestra

// CASO 1: Eficiencia según calor en material del punto focal
//Definiciones para el cálculo de eficiencia según el Calor en el punto focal
#define masaMaterialFocal  0.2                    // Masa del material en el punto focal, en kilogramos : 0.2 kg
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
             "," + String(Ha) + "," + String(E);
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
/*void IRAM_ATTR flujoInterrupcion() {
    if (midiendoFlujo) {
        pulsos++; // Incrementa solo si estamos en período de medición
    }
}
*/
//°°°°°°°°°°°°°°°°°°°°°°°°°°°°°         PROGRAMA PRINCIPAL      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void setup() {
    Serial.begin(115200);

    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);

    dht.setup(pinDHT, DHTesp::DHT11);

    // Configura el ESP32 para que se despierte con un cambio en el pin del LDR
    esp_sleep_enable_ext0_wakeup(ldrPin, LOW);  // Despierta si el pin va a LOW, es decir es de día
    
    conectarWiFi(); // Intentar conectar al iniciar

    //Bandera de reinicio
    Serial.println("Sistema reiniciado");
}

void loop() {
    // NOCHE?. Verifica si es de noche para entrar en modo de sueño profundo
    if (digitalRead(ldrPin) == HIGH) { // asumiendo que HIGH significa "noche"
        Serial.printf(" Es de noche, entrando en modo de sueño profundo = %i \n" + digitalRead(ldrPin));
        sleepingLed();  // Efecto visual antes de entrar en hibernación
        esp_deep_sleep_start();  // Entrada en modo de sueño profundo
    }else {
        unsigned long tiempoActual = millis();
        
        // Verificar si ha pasado el tiempo de muestreo
        if (tiempoActual - tiempoAnterior >= tiempoDeMuestreo) {
            tiempoAnterior = tiempoActual; // Actualizar el último momento de muestreo

            // CONEXION. Verifica la conexión
            if (!client.connect(WiFi.gatewayIP(), 80)) {
                Serial.println("Conexión fallida");
                delay(10000);
                return;
            }

            // Realizar acciones de muestreo
            // SENSORES. Lectura de sensores
            TempAndHumidity data = dht.getTempAndHumidity();    //Sensor de temperatura y humedad
            Serial.printf("\n Temp_amb: %0.2f , Hum_amb: %0.2f ", data.temperature, data.humidity);

            // REGISTRO. Registro de datos
            registroData.updateData(leer_termopar(), data.temperature, data.humidity, leer_piranometro(), 999 /*leer_flujoLiquido()*/, "fecha y hora");
            
            // ENVIO. Envio de datos
            enviarDatos(registroData.toCSVString());

        }

    }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ FUNCIONES DE SENSORES $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
float leer_piranometro() {
    int lecturaADC = analogRead(pinRADIACION);
    float voltaje = (lecturaADC / 4095.0) * 3.33; // Asumiendo que el rango máximo del ADC es 5V
    float irradiacion = voltaje * (1800.0 / 3.33); // Conversión basada en la calibración del piranómetro
    Serial.printf("\n Radiacion: %0.2f \n", irradiacion);
    return irradiacion;
}

/*
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

*/
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
    Serial.printf("\n Temp colector: %0.2f", v * 0.25);
    return v * 0.25; // Calcula la temperatura
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ FUNCIONES PARA INDICADORES LED $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
void sleepingLed(){
    // Efecto visual de "dormirse" del LED
    for (int i = 255; i >= 0; i--) {
        analogWrite(ledPin, i);
        delay(15);
    }
}

void blinkLed(int pin, int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ FUNCIONES DE COMUNICACION $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
void conectarWiFi() {   //! no entra al if que verifica si es de dia o noche y se queda esperando la conexión: mejor usar un do while por tiempo limitado o algo así
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        digitalWrite(ledPin, !digitalRead(ledPin)); // Parpadeo continuo
    }
    Serial.println("Conectado a Wi-Fi");
    digitalWrite(ledPin, HIGH); // LED constante cuando está conectado
}


void enviarDatos(String registro) {
    // Enviar datos mediante HTTP GET
    // Reemplaza espacios en blanco con %20 en la cadena registro
    registro.replace(" ", "%20");
    Serial.println("Enviando datos..."+ registro);
    client.println(String("GET /?datos=") + registro + " HTTP/1.1\r\n" +
                 "Host: " + WiFi.gatewayIP().toString() + "\r\n" +
                 "Connection: close\r\n\r\n");

    blinkLed(ledPin, 2, 250); // Parpadeo rápido al enviar datos
    client.stop();
}