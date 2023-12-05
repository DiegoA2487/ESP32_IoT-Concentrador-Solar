#include <ArduinoUnit.h>

// Prueba para verificar que se establezca correctamente la conexión WiFi
test(sincronizarRTC_conexionWiFi) {
  WiFi.begin(ssid, password);
  sincronizarRTC();
  assertEqual(WiFi.status(), WL_CONNECTED);
}

// Prueba para verificar que se sincronice correctamente el RTC con NTP
test(sincronizarRTC_sincronizacionRTC) {
  sincronizarRTC();
  assertNotEqual(rtc.getDateTime(), "0000-00-00T00:00:00");
}

// Prueba para verificar que se muestre correctamente la fecha y hora actual del RTC
test(loop_mostrarFechaHoraActual) {
  sincronizarRTC();
  String fechaHoraActual = rtc.getDateTime();
  assertEqual(Serial.available(), 0);  // Verificar que no haya datos disponibles en el puerto serie
  loop();  // Llamar a la función loop para mostrar la fecha y hora actual
  assertEqual(Serial.available(), fechaHoraActual.length() + 2);  // Verificar que se muestre la fecha y hora actual en el puerto serie
  assertEqual(Serial.readString(), "Timestamp: " + fechaHoraActual);  // Verificar que la fecha y hora actual se muestre correctamente
}

// Prueba para verificar que se realice el parpadeo del LED correctamente
test(blinkLed_parpadeoLED) {
  int pin = 2;
  int times = 3;
  int delayTime = 500;
  blinkLed(pin, times, delayTime);
  for (int i = 0; i < times; i++) {
    assertEqual(digitalRead(pin), HIGH);  // Verificar que el pin del LED esté en estado alto
    delay(delayTime);
    assertEqual(digitalRead(pin), LOW);  // Verificar que el pin del LED esté en estado bajo
    delay(delayTime);
  }
}

unittest_main()