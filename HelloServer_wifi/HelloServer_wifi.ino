#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

const char* ssid = "UPC3165272";
const char* password = "DAGKFDCC";

IPAddress ip(192,168,0,13);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(666);

const int led = D2;

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void onLed() {
  server.send(200, "text/plain", "ON LED!");
  Serial.println("LED ON");
  digitalWrite(led, 1);
}

void offLed() {
  server.send(200, "text/plain", "OFF LED!");
  Serial.println("LED OFF");
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 0);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.config(ip,gateway,subnet);
  WiFi.begin(ssid, password);
  
  ArduinoOTA.setPassword("dune1234");
  ArduinoOTA.begin();
  
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  
  server.on("/on", onLed);
  server.on("/off", offLed);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

}

void loop(void){
  ArduinoOTA.handle();
  server.handleClient();
}

