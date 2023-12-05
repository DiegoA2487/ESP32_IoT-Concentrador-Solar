#include <WiFi.h>

const char* ssid = "BISONProDiego";
const char* password = "wifidiego";
const int ledPin = 2; // Asume que el LED2 está conectado al pin GPIO 2

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

    // Aquí va el resto del programa...

    delay(2500); // Un delay para evitar la verificación constante
}

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
