/*Este código está diseñado para manejar el ciclo de sueño y vigilia de un ESP32 
en función de la luz ambiental, detectada a través de un sensor LDR. 
El propósito principal es ahorrar energía al poner el dispositivo en modo 
de sueño profundo durante la noche y despertarlo durante el día.*/

#include <Arduino.h>

// Definiciones para GPIOs (LDR: Noche y día = GPIO 15)
const gpio_num_t ldrPin = GPIO_NUM_15;  // Pin para sensar día/noche
const int ledPin = 2;                   // Pin del LED para indicadores

void setup() {
    Serial.begin(115200);
    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);

    // Configura el ESP32 para que se despierte con un cambio en el pin del LDR
    esp_sleep_enable_ext0_wakeup(ldrPin, LOW);  // Despierta si el pin va a LOW, es decir es de día
    
    digitalWrite(ledPin, HIGH);
    Serial.println("ESP32 en modo de ejecución normal.");
}

void loop() {
    // Verifica si es de noche para entrar en modo de sueño profundo
    if (digitalRead(ldrPin) == HIGH) { // asumiendo que HIGH significa "noche"
        Serial.printf(" Es de noche, entrando en modo de sueño profundo = %i \n" + digitalRead(ldrPin));
        sleepingLed();  // Efecto visual antes de entrar en hibernación
        esp_deep_sleep_start();  // Entrada en modo de sueño profundo
    }

    Serial.println("Haciendo cualquier cosa...");
    delay(2000);
}

void sleepingLed() {
    // Efecto visual de "dormirse" del LED
    for (int i = 255; i >= 0; i--) {
        analogWrite(ledPin, i);
        delay(15);
    }
}
