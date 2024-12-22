/*
  Este programa para ESP32 se enfoca en la recolección y procesamiento de datos ambientales y de rendimiento de un concentrador solar. Las características principales incluyen:

  - Lectura de datos de temperatura y humedad a través del sensor DHT.
  - Medición de la temperatura del colector solar utilizando un termopar MAX6675.
  - Medición de la radiación solar con un piranómetro analógico y la medición del flujo másico del líquido.
  - Cálculo de la eficiencia del concentrador solar en base a los datos recopilados.
  - manejar el ciclo de sueño y vigilia de un ESP32 en función de la luz ambiental, detectada a través de un sensor LDR. El propósito principal es ahorrar energía al poner el dispositivo en modo de sueño profundo durante la noche y despertarlo durante el día

  Este código está diseñado para ser utilizado en aplicaciones relacionadas con la energía solar y la monitorización ambiental, proporcionando una base para la recopilación de datos precisa y en tiempo real.

!Pendientes:
    //! No enviar NAN sino -1 o 0, ya que esto compromete los datos en la app movil
    ! Trabajar por tareas
    ! Registro de tiempo del envio de los datos
    ! Log de fallas o mensajes en consola, especificando el id del dispositivo, para que este pueda ser reconocido en la app
    ! Mejorar tarea de reconexión a red Mesh: Entrar a modo sueño despues de ciertos intentos, despertar despúes de un tiempo transcurrido para intentar reconectar
    ! Que envie mensaje de que está dormido
    ! Manejo de notificaciones y código de errores
?Aprendizajes:
    ? Cada vez que uses client.println(), también llamer a client.stop() después para cerrar la conexión y liberar los recursos

*/

#include <Arduino.h>
#include "DHTesp.h"
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <ESP32Time.h>

//sensor medicion de corriente 
#include <Adafruit_INA219.h> //librería para el uso del sensor agregada en el gestor de bibliotecas
Adafruit_INA219 ina219; //declaración del objeto en la libreria que representa al sensor INA219 

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

// Configuración del servidor NTP
const char* ntpServer = "mx.pool.ntp.org";
const long gmtOffset_sec = (-6)*3600;  // GMT -6 para México
const int daylightOffset_sec = 3600;  // Horario de verano
ESP32Time rtc;

// Definiciones para el muestreo
const unsigned long tiempoDeMuestreo = (5)*(60)*(1000);// Tiempo de muestreo: (min)*(seg)*(1000ms) ; 1 hora > (60)*(60*1000)= (3600000 milisegundos)
unsigned long tiempoAnterior = 0; // Guarda el último momento en que se tomó una muestra

//Definiciones para el tiempo
const unsigned long tiempoDeEspera = (10)*(1000); //

// CASO 1: Eficiencia según calor en material del punto focal
//Definiciones para el cálculo de eficiencia según el Calor en el punto focal
#define masaMaterialFocal  0.75                    // Masa del material en el punto focal, en kilogramos : 0.75 kg
#define calorEspecificoMaterial  500              // Calor específico del material en el punto focal, en J/kg°C

// CASO 2: Eficiencia según calor en liquido
// Definiciones para el cálculo de eficiencia
//#define calorEspecificoLiquido  4.18    // Calor específico del líquido en el colector, en J/g°C
#define radioDiscoParabolico  0.405                // Radio del disco parabólico en metros {81 cm de diámetro}
#define profundidadDiscoParabolico 0.06           // Profundidad del disco parabólico en metros {6cm}
#define areaDiscoParabolico  (PI * radioDiscoParabolico * sqrt(pow(radioDiscoParabolico, 2) + 4 * pow(profundidadDiscoParabolico, 2))) // Área del disco parabólico en m²
float temperaturaInicial = 25.0;                // Temperatura inicial del material en el punto focal, en °C


// Estructura para guardar las variables por muestra
class RegistroData {
public:
    float Tc;       // Temperatura del colector
    float Ta;       // Temperatura ambiental
    float Ha;       // Humedad relativa del ambiente
    float R;        // Radiación
    float Fl;       // Flujo másico del líquido
    float E;        // Eficiencia del concentrador solar
    float Ti;       // Temperatura inicial en termopar

    // Variables del INA219
    float shuntVoltage;
    float busVoltage;
    float current_mA;
    float power_mW;
    float loadVoltage;

    String timestamp; // Marca de tiempo de la muestra

      RegistroData() : Tc(temperaturaInicial), Ta(0), Ha(0), R(0), Fl(0), E(0), Ti(temperaturaInicial),
                     shuntVoltage(0), busVoltage(0), current_mA(0), power_mW(0), loadVoltage(0), timestamp("") {}

    void updateData(float tempColector, float tempAmb, float humAmb, float radiacion, 
                    float flujoLiquido, String time,
                    float shuntV, float busV, float curr_mA, float power_mW_val, float loadV) {
        Ti = Tc; // Asigna la temperatura de la muestra anterior
        Tc = tempColector;
        Ta = tempAmb;
        Ha = humAmb;
        R = radiacion;
        Fl = flujoLiquido;
        E = calcularEficienciaConTermopar(Ti, tempColector, radiacion);
        timestamp = time;

        // Asignar los valores del INA219 a los atributos correspondientes
        shuntVoltage = shuntV;
        busVoltage = busV;
        current_mA = curr_mA;
        power_mW = power_mW_val;
        loadVoltage = loadV;
    }

    void adquirirDatosINA219() {
        shuntVoltage = ina219.getShuntVoltage_mV();
        busVoltage = ina219.getBusVoltage_V();
        current_mA = ina219.getCurrent_mA();
        power_mW = ina219.getPower_mW();
        loadVoltage = busVoltage + (shuntVoltage / 1000);
    }

    void imprimirValores() { // para imprimir en consola al estilo tabla
        // Imprimir valores en consola
        Serial.println("\n      TimeStamp          |   R    |   Tc   |   Ta   |   Ha   |    E   |   Ti   |   Fl   |  ShuntV  |  BusV  |  CurrmA  |  PowermW  |  LoadV");
        Serial.printf("%s | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f | %6.2f\n\n",
                      timestamp.c_str(), R, Tc, Ta, Ha, E, Ti, Fl, shuntVoltage, busVoltage, current_mA, power_mW, loadVoltage);
    }

    String toCSVString() {
        // Formatea los datos en un string CSV
        String registroConcatenado = timestamp + "," + String(R) + "," + String(Tc) + "," + String(Ta) +
                                     "," + String(Ha) + "," + String(E) + "," + String(Ti) + "," + String(Fl) +
                                     "," + String(shuntVoltage) + "," + String(busVoltage) + "," + String(current_mA) +
                                     "," + String(power_mW) + "," + String(loadVoltage);

        // Reemplaza cualquier "NAN" por "0.001"
        registroConcatenado.replace("nan", "0.001");
        registroConcatenado.replace("inf", "0.002");

        return registroConcatenado;
    }

    // Cálculo de la eficiencia: Eout/Ein
    float calcularEficienciaConTermopar(float tempInicial, float tempFinal, float irradiancia) {
        // Calculo de Energía absorbida (Eout [J] = m[kg] x C[J/kg°C] x (Tfinal-Tinicial)[°C])
        float deltaTemp = tempFinal - tempInicial;
        float energiaAbsorbida = masaMaterialFocal * calorEspecificoMaterial * deltaTemp;
        printf("\nEnergia Absorbida (Eout [J]) = %.2f [kg] x %.2f [J/kg°C] x (%.2f [°C] - %.2f [°C]) = %.2f [J]\n",
               masaMaterialFocal, calorEspecificoMaterial, tempFinal, tempInicial, energiaAbsorbida);

        // Calculo de Energía incidente (Ein [J] = I[W/m2] x A[m2] x t[seg])
        float energiaIncidente = irradiancia * areaDiscoParabolico * (tiempoDeMuestreo / 1000);
        printf("Energia Incidente (Ein [J]) = %.2f [W/m2] x %.2f [m2] x %d [seg] = %.2f [J]\n",
               irradiancia, areaDiscoParabolico, tiempoDeMuestreo / 1000, energiaIncidente);
        
        // Calculo de la eficiencia
        float n = fabs(energiaAbsorbida / energiaIncidente);
        printf("Eficiencia (n) = fabs(%.2f [J] / %.2f [J]) = %.2f\n", energiaAbsorbida, energiaIncidente, n);
        return n;
    }
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


    Wire.begin(); // Inicializar el bus I2C
    Serial.begin(115200); // Inicializar comunicación serial
     if (! ina219.begin()) {
    Serial.println("No se encontro el chip INA219");
    while (1) { delay(10); }
    }
    // Para mayor precision en mediciones en el rango menor a 32V, 1A:
    ina219.setCalibration_32V_1A();
    // Para mediciones en el rango menor a 16V, 400mA:
    //ina219.setCalibration_16V_400mA();   

    Serial.println("Midiendo voltage y corriente con el INA219 ...");

    //Bandera de reinicio
    Serial.println("Sistema reiniciado");
    sincronizarRTC();
}

void loop() {
    // NOCHE?. Verifica si es de noche para entrar en modo de sueño profundo
    if (digitalRead(ldrPin) == HIGH) { // asumiendo que HIGH significa "noche"
        Serial.printf(" Es de noche, entrando en modo de sueño profundo = %i \n", digitalRead(ldrPin));
        sleepingLed();  // Efecto visual antes de entrar en hibernación
        esp_deep_sleep_start();  // Entrada en modo de sueño profundo
    }else {
        unsigned long tiempoActual = millis();

        // Verificar si ha pasado el tiempo de muestreo
        if (tiempoActual - tiempoAnterior >= tiempoDeMuestreo) {
            tiempoAnterior = tiempoActual; // Actualizar el último momento de muestreo

            // Realizar acciones de muestreo
            // REGISTRO. Registro de datos
            String timeStampLocal= String(rtc.getDateTime());
            timeStampLocal.replace(",","");        //Remplaza la coma "," para no tener conflictos con otras funciones en Gsheet

            // SENSORES. Lectura de sensores
            TempAndHumidity data = dht.getTempAndHumidity();    //Sensor de temperatura y humedad

            /*
            Cálculo de la eficiencia de un horno solar de disco parabólico

            Variables y Valores de Ejemplo:
            - Irradiación Solar: 500 W/m²
            - Diámetro del Disco Parabólico: 0.81 m (81 cm)
            - Profundidad del Disco Parabólico: 0.06 m (6 cm)
            - Masa del Tazón de Acero Inoxidable: 0.73 kg
            - Calor Específico del Acero Inoxidable: 500 J/kg°C
            - Temperatura Inicial punto focal: 72°C
            - Temperatura Final punto focal: 100°C
            - Tiempo de Muestreo: 5 minutos (convertido a horas en el cálculo)

            Eficiencia debe ser de 13%

            ?registroData.updateData(100, data.temperature, data.humidity, 500, 999, timeStampLocal)
            */
            
      

            // Adquirir los valores del INA219
            float shuntV = ina219.getShuntVoltage_mV();
            float busV = ina219.getBusVoltage_V();
            float curr_mA = ina219.getCurrent_mA();
            float power_mW_val = ina219.getPower_mW();
            float loadV = busV + (shuntV / 1000);

            // Actualizar los datos, incluyendo los del INA219
            registroData.updateData(leer_termopar(), data.temperature, data.humidity, leer_piranometro(), 
                                    999 /*leer_flujoLiquido()*/, timeStampLocal, 
                                    shuntV, busV, curr_mA, power_mW_val, loadV);

            // Imprimir los valores
            registroData.imprimirValores();

            // Enviar los datos en formato CSV
            enviarDatos(registroData.toCSVString());
      
        }

    }
}

//$$$$$$$$$$$$$$$$$$$$$$$$$$$ FUNCIONES DE SENSORES $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
float leer_piranometro() {
    int lecturaADC = analogRead(pinRADIACION);
    float voltaje = (lecturaADC / 4095.0) * 3.33; // Asumiendo que el rango máximo del ADC es 5V
    float irradiacion = voltaje * (1800.0 / 3.33); // Conversión basada en la calibración del piranómetro
    //Serial.printf("\n Radiacion: %0.2f \n", irradiacion);
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
    //Serial.printf("\n Temp colector: %0.2f", v * 0.25);
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
    String respuesta;
    // Enviar datos mediante HTTP GET
    // Reemplaza espacios en blanco con %20 en la cadena registro
    registro.replace(" ", "%20");
    Serial.println("Enviando datos..."+ registro);

    if (client.connect(WiFi.gatewayIP(), 80)) {     // CONEXION. Verifica la conexión
        client.println(String("GET /?datos=") + registro + " HTTP/1.1\r\n" +
                     "Host: " + WiFi.gatewayIP().toString() + "\r\n" +
                     "Connection: close\r\n\r\n");

        // Espera a que el servidor responda
        while(client.connected() && !client.available()){
            delay(10); // Pequeña espera para la disponibilidad de datos
        }

        // Leer y mostrar la respuesta
        while(client.available()){
            respuesta = client.readStringUntil('\r');       //se queda únicamente con el último texto del salto de renglón que corresponde al mensaje final.
        }
        Serial.print("Respuesta del servidor: " + respuesta);

        blinkLed(ledPin, 2, 250); // Parpadeo rápido al enviar datos
        client.stop(); // Cierra la conexión
    } else {
        Serial.println("Fallo la conexion con el servidor.");
    }
}

void sincronizarRTC() {
    Serial.println("Sincronización de RTC con NTP en curso");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(6000); // Espera para que la hora se sincronice

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Fallo en la sincronización con NTP");
    } else {
        Serial.println("RTC sincronizado: " + rtc.getDateTime());
    }
}


