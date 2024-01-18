#include <WiFi.h>

const char* ssid = "ESP8266_Mesh_Node";
const char* password = "password";
const int ledPin = 2;  // LED pin
WiFiClient client;

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        blinkLed(ledPin, 1, 500); // Parpadeo rápido mientras se conecta
        delay(500);
        Serial.print(".");
    }
    digitalWrite(ledPin, HIGH); // LED ON when connected
    Serial.println("Conectado a la red Wi-Fi");
}

void loop() {
    if (!client.connect(WiFi.gatewayIP(), 80)) {
        Serial.println("Conexión fallida");
        delay(10000);
        return;
    }

    String datos = "registroData.toCSVString() " + String(millis());
    client.println(String("GET /?datos=") + datos + " HTTP/1.1\r\n" +
                 "Host: " + WiFi.gatewayIP().toString() + "\r\n" +
                 "Connection: close\r\n\r\n");

    blinkLed(ledPin, 2, 250); // Parpadeo rápido al enviar datos
    client.stop();
    delay(10000);  // Espera antes del próximo envío
}

void blinkLed(int pin, int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
}
