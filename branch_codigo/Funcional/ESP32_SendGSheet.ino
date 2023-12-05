#include <WiFi.h>
#include <HTTPClient.h>

const char * ssid = "BISONProDiego"; //"DIEGO-PC"; //Para la PC
const char * password = "wifidiego";

String server = "http://maker.ifttt.com";
String eventName = "temp";
String IFTTT_Key = "bdClU_zgRe2_ERMeSOqEVf";
String IFTTTUrl="https://maker.ifttt.com/trigger/temp/json/with/key/bdClU_zgRe2_ERMeSOqEVf";

//      ++++++++++++++++ 3. VARIABLES  ++++++++
// Definiciones para el muestreo
const int tiempoDeMuestreo = (0.25)*60*1000;                  // Tiempo de muestreo: (1) minuto

// Estructura para guardar las variables por muestra
class RegistroData {
  public:
    float Tc;                                        // Temperatura del colector
    float Ta;                                        // Temperatura ambiental
    float Ha;                                        // Humedad relativa del ambiente
    float R;                                         // Radiación
    float Fl;                                        // Flujo másico del líquido
    float E;                                         // Eficiencia del concentrador solar
    String timestamp;                                // Marca de tiempo de la muestra

    RegistroData() : Tc(0), Ta(0), Ha(0), R(0), Fl(0), E(0), timestamp("") {}

    void updateData(float tempColector, float tempAmb, float humAmb, float radiacion, 
                    float flujoLiquido, float eficiencia, String time) {
      Tc = tempColector;
      Ta = tempAmb;
      Ha = humAmb;
      R = radiacion;
      Fl = flujoLiquido;
      E = eficiencia;
      timestamp = time;
    }

    String toCSVString() {
      // Formatea los datos en un string CSV
      return timestamp + "," + String(R) + "," + String(Tc) + "," + String(Ta) + 
             "," + String(Ha) + "," + String(Fl) + "," + String(E);
    }
};

RegistroData registroData;                           // Instancia de RegistroData para almacenar y gestionar datos de muestreo

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Viola, Connected !!!");
}
void loop() {
    Serial.println(registroData.toCSVString());
    sendDataToSheet();
    delay(tiempoDeMuestreo);
}

void sendDataToSheet(void)
{
  // Construir la URL con los datos a enviar
  String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + registroData.toCSVString();  

  // Imprimir la URL para depuración
  Serial.println(url);
  
  // Iniciar una conexión HTTP
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  http.begin(url); //HTTP

// Enviar una solicitud GET
  Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
  
  // Verificar si la solicitud fue exitosa
  if(httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    
    // Si la respuesta del servidor es OK, imprimir el payload
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
// Si la solicitud falla, imprimir el error
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  // Terminar la conexión HTTP
  http.end();
}