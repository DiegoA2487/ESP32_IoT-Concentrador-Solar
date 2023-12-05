/*
  Este código para ESP32 gestiona la conexión a una red Wi-Fi y el almacenamiento de datos
  usando el sistema de archivos LittleFS. Si el dispositivo no está conectado a Wi-Fi,
  almacena un contador en la memoria interna. Cuando se conecta a Wi-Fi, envía todos los
  datos almacenados, así como el valor actual del contador, a la consola, simulando un
  envío a la nube. Los datos se manejan en un formato FIFO para mantener su orden.
*/

#include <WiFi.h>
#include "FS.h"
#include "LittleFS.h"

// Configuración de Wi-Fi
const char *ssid = "DIEGO-PC";
const char *password = "wifidiego";
const int ledPin = 2;       // LED para indicar el estado de la conexión Wi-Fi
unsigned long contador = 0; // Contador que incrementa en cada ciclo

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
}

void loop()
{
    // Verifica el estado de la conexión Wi-Fi
    if (WiFi.status() != WL_CONNECTED)
    {
        guardarDatosLocalmente(String(contador)); // Guarda el contador en LittleFS si no está conectado
    }
    else
    {
        enviarDatosALaConsola(); // Envía datos a la consola cuando está conectado
    }
    contador++;  // Incrementa el contador
    delay(500); // Espera 5 segundos antes del próximo ciclo
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
        Serial.print(archivoEscritura.name());
        Serial.println(">LittleFs:" + dataString);
        delay(100);
    }
}

// Envía los datos almacenados y el contador actual a la consola
void enviarDatosALaConsola()
{
    if (!LittleFS.exists("/datos.csv"))
    {
        Serial.println("Archivo /datos.csv no existe, nada que enviar");
        return;
    }
    else
    {
        File archivo = LittleFS.open("/datos.csv", "r");

        Serial.println("Enviando datos almacenados a la consola:"); // Resto de tu lógica para leer y enviar datos
        while (archivo.available() )
        {
            String data = archivo.readStringUntil('\n');
            if (data.length() > 0)
            { // Verifica si la cadena leída tiene contenido
                Serial.println("[" + data + "]");
                delay(100);
                if ( data == "10")
                {
                    archivo.close();
                    Serial.println("maximo alcanzado; removiendo archivo"); // Envía el valor actual del contador
                    LittleFS.remove("/datos.csv");
                    return;
                }
            }
            if (!archivo.available()) { // Si ya no hay más datos
                Serial.println("Fin del archivo alcanzado.");
                break;
            }
        }
        Serial.println("Eliminando archivo...."); // Envía el valor actual del contador
        archivo.close();                                        // Cierra el archivo
        LittleFS.remove("/datos.csv");
    } // Opcional: elimina el archivo después de enviar los datos
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
