// Este código realiza una prueba sencilla del sistema de archivos LittleFS en ESP32.
// Al iniciar, intenta montar el sistema de archivos LittleFS y, si tiene éxito,
// escribe una cadena de texto en un archivo llamado "datos.csv".
// Esto sirve para probar la capacidad de almacenamiento y recuperación de datos
// en la memoria interna del ESP32 usando LittleFS.

#include "FS.h"      // Incluye la biblioteca de sistema de archivos
#include "LittleFS.h" // Incluye la biblioteca LittleFS específica para ESP32

void setup() {
    Serial.begin(115200); // Inicia la comunicación serial
    // Intenta montar el sistema de archivos LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("Error al montar el sistema de archivos LittleFS");
        return; // Si falla, detiene la ejecución del programa
    }
    guardarDatosLocalmente("Prueba de almacenamiento local"); // Guarda una cadena de prueba en un archivo
}

void loop() {
    // No se realiza ninguna acción en el loop
}

// Función para guardar datos en un archivo en el sistema de archivos LittleFS
void guardarDatosLocalmente(String dataString) {
    File archivo = LittleFS.open("/datos.csv", "a"); // Abre (o crea) un archivo en modo append
    if (!archivo) {
        Serial.println("Error al abrir el archivo para escribir");
        return; // Si no puede abrir el archivo, sale de la función
    }
    archivo.println(dataString); // Escribe la cadena en el archivo
    archivo.close(); // Cierra el archivo
    Serial.println("Datos guardados localmente: " + dataString); // Confirma que los datos se guardaron
}

