#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>


const char* ssid = "UPC3165272";
const char* password = "DAGKFDCC";

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);

  ArduinoOTA.setPassword("dune1234");
  ArduinoOTA.begin();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
}

void loop() {

  ArduinoOTA.handle();
  // wait for WiFi connection

  HTTPClient http_on;
  http_on.begin("http://192.168.0.13:666/on"); //HTTP
  Serial.println("http://192.168.0.13:666/on");
  // start connection and send HTTP header
  http_on.GET();
  http_on.end();

  delay(50);
  
  HTTPClient http_off;
  http_off.begin("http://192.168.0.13:666/off"); //HTTP
  Serial.println("http://192.168.0.13:666/off");
  // start connection and send HTTP header
  http_off.GET();
  http_off.end();

  delay(1000);
}

