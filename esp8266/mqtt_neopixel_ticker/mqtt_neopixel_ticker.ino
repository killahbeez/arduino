#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <explode.h>

#include <Ticker.h>
Ticker ticker;
boolean isNeopixelON = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, D2, NEO_GRB + NEO_KHZ800);

const char* ssid = "UPC3165272";
const char* password = "DAGKFDCC";

const char* www_username = "adi";
const char* www_password = "dune1234";

IPAddress ip(192, 168, 0, 14);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

/* MQTT Settings */
WiFiClient wificlient;
PubSubClient client(wificlient);

// this are topics for which to publish or subscribe
// command format   turn#1#R  => turn neopixel ON with color RED
//                  dim#50    => dim neopixel 50%
struct Topics {
  char* command = "/home/birou/neopixel/command"; //subscribe for commands
  char* state = "/home/birou/neopixel/state"; //publish the state
  char* commandDim = "/home/birou/neopixel/commandDim"; //subscribe for command Dim
  char* stateDim = "/home/birou/neopixel/stateDim"; //publish the state
  char* debug = "/home/debug"; //publish debug message
} neopixelTopic;

IPAddress broker(192, 168, 0, 16);       // Address of the MQTT broker
#define CLIENT_ID "client-neopixel" // Client ID to send to the broker

/**
   MQTT callback to process messages
*/
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';

  if (strcmp(topic, neopixelTopic.command) == 0) {
    char **arr = NULL;
    int count = 0;
    explode((char*) payload, "#", &arr, &count);

    if ((String)arr[0] == "turn") { //if the command is turn

      if (atoi(arr[1]) == 1) { //if is to turn ON
        setNeopixel((String)arr[2]);
      }
      else {
        offNeopixel((String)arr[2]);
      }
    }
  }

  if (strcmp(topic, neopixelTopic.commandDim) == 0) {
    Serial.println(atoi((char*)payload));
    dimNeopixel((char*)payload);
  }
}

void dimNeopixel(char* dim_value) {
  uint16_t dim_value_to_int = atoi(dim_value);
  char buffer[30];

  sprintf(buffer, "%s", dim_value);
  client.publish(neopixelTopic.stateDim, buffer, true);

  sprintf(buffer, "Neopixel Dim %d", dim_value_to_int);
  client.publish(neopixelTopic.debug, buffer, true);

  pixels.setBrightness(dim_value_to_int);
  pixels.show();

}

void offNeopixel(String RGB) {
  pixels.clear();
  pixels.show();

  if (RGB == "R") {
    client.publish(neopixelTopic.state, "turn#0#R", true);
    client.publish(neopixelTopic.debug, "Neopixel RED turned OFF", true);
  }
  if (RGB == "G") {
    client.publish(neopixelTopic.state, "turn#0#G", true);
    client.publish(neopixelTopic.debug, "Neopixel GREEN turned OFF", true);
  }
  if (RGB == "B") {
    client.publish(neopixelTopic.state, "turn#0#B", true);
    client.publish(neopixelTopic.debug, "Neopixel BLUE turned OFF", true);
  }

  isNeopixelON = 0;
}

void setNeopixel(String RGB) {
  static uint32_t color;
  Serial.println(RGB);

  if (RGB == "R") {
    pixels.clear();
    color  = pixels.Color(255, 0, 0);
    for (uint8_t cnt = 0; cnt < 8; cnt++) {
      pixels.setPixelColor(cnt, color);
    }
    pixels.show();

    client.publish(neopixelTopic.state, "turn#1#R", true);
    client.publish(neopixelTopic.debug, "Neopixel RED turned ON", true);
  }

  if (RGB == "G") {
    pixels.clear();
    color = pixels.Color(0, 255, 0);
    for (uint8_t cnt = 0; cnt < 8; cnt++) {
      pixels.setPixelColor(cnt, color);
    }
    pixels.show();

    client.publish(neopixelTopic.state, "turn#1#G", true);
    client.publish(neopixelTopic.debug, "Neopixel GREEN turned ON", true);
  }

  if (RGB == "B") {
    pixels.clear();
    color = pixels.Color(0, 0, 255);
    for (uint8_t cnt = 0; cnt < 8; cnt++) {
      pixels.setPixelColor(cnt, color);
    }
    pixels.show();

    client.publish(neopixelTopic.state, "turn#1#B", true);
    client.publish(neopixelTopic.debug, "Neopixel BLUE turned ON", true);
  }
  isNeopixelON = 1;
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
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

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(callback);

  pixels.begin();
  pixels.clear();
  pixels.show();

  ticker.attach(0.001, callback_ticker);
}

volatile uint16_t cnt = 0;
volatile boolean tick = 0;
void callback_ticker() {
  cnt++;
  tick = 1;
}
/**
   Attempt connection to MQTT broker and subscribe to command topic
*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe(neopixelTopic.command);
      client.subscribe(neopixelTopic.commandDim);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop(void) {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
  }

  if (client.connected()) {
    client.loop();
  }

  /*
    static uint32_t last_tmst = 0;
    static uint16_t adc = 0;
    static uint16_t last_adc = 0;
    char voltage[10];

    if ((millis() - last_tmst) > 10) {
    adc = analogRead(A0);
    if (adc >= (last_adc + 10) || adc <= (last_adc - 10)) {
      dtostrf((adc * 3.3) / 1023, 4, 2, voltage);
      client.publish("/home/didi", voltage);
      last_adc = adc;
    }
    last_tmst = millis();
    }
  */

  if (tick && isNeopixelON) {
    static char buffer[10];
    snprintf(buffer, 10, "%d", cnt);
    
    client.publish("/home/didi", buffer);
    tick = 0;
    /*
    static char buffer[20];
    static String color = "RGB";

    snprintf(buffer, 10, "turn#1#%c", color[cnt % 3]);
    client.publish("/home/birou/neopixel/command", buffer);

    snprintf(buffer, 10, "%d %d", millis(), cnt % 3);
    client.publish("/home/debug", buffer);
    trigger = 0;
    */
  }
  if(!isNeopixelON) cnt = 0;
}

