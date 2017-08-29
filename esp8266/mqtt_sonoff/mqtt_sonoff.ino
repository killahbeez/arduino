#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <explode.h>

const char* ssid = "UPC3165272";
const char* password = "DAGKFDCC";

const char* www_username = "adi";
const char* www_password = "dune1234";

IPAddress ip(192, 168, 0, 15);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

/* MQTT Settings */
WiFiClient wificlient;
PubSubClient client(wificlient);

// this are topics for which publish or subscribe
struct Topics {
  char* command = "/home/birou/lampa/command"; //subscribe for commands && publish on physical lamp switch
  char* state = "/home/birou/lampa/state"; //publish the state 
  char* debug = "/home/debug"; //publish debug message
} lampaTopic;

//command format ( command#debug message )
struct JSON {
  uint8_t command;
  const char* debug;
};

struct JSON jsonLampa;

IPAddress broker(192, 168, 0, 16);       // Address of the MQTT broker
#define CLIENT_ID "client-lampa" // Client ID to send to the broker

uint32_t eta;
boolean isON = 0;

/* Sonoff Outputs */
const int relayPin = 12;  // Active high
const int ledPin = 13; // Active low
const int button = 0;
boolean lastRead = 1;

/**
   MQTT callback to process messages
*/
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';

  if (strcmp(topic, lampaTopic.command) == 0) {
    char **arr = NULL;
    int count = 0;
    explode((char*) payload, "#", &arr, &count);

    jsonLampa.command = atoi(arr[0]);
    jsonLampa.debug = arr[1];
    
    if (jsonLampa.command == 1) {            // (turn outputs ON)
      onRelay();
    }
    else if (jsonLampa.command == 0) {     // (turn outputs OFF)
      offRelay();
    }
    else {
      Serial.println("Unknown value");
    }
  }

}

void onRelay() {
  Serial.println("RELAY ON");
  analogWrite(ledPin, 0);
  digitalWrite(ledPin, HIGH); // LED is active-low, so this turns it off
  digitalWrite(relayPin, HIGH);

  client.publish(lampaTopic.debug, jsonLampa.debug, true);
  client.publish(lampaTopic.state, "1", true);
  isON = 1;
}

void offRelay() {
  Serial.println("RELAY OFF");
  analogWrite(ledPin, 0);
  digitalWrite(ledPin, HIGH); // LED is active-low, so this turns it off
  digitalWrite(relayPin, LOW);

  client.publish(lampaTopic.debug, jsonLampa.debug, true);
  client.publish(lampaTopic.state, "0", true);
  isON = 0;
}

void pulsate() {
  static int8_t is_UpLed = 1;
  static uint16_t cnt = 300;

  //lights down gradually until is all off
  if (is_UpLed == 1 && cnt < 1023 && (millis() - eta) > 1) {
    cnt += 4;
    analogWrite(ledPin, cnt);

    if (cnt >= 1023) {
      analogWrite(ledPin, 1023);
      is_UpLed = -1;
    }
    eta = millis();
  }

  //wait for a time as all off
  if (is_UpLed == -1 && (millis() - eta) > 300) {
    is_UpLed = 0;
  }

  //lights up gradually (active low led)
  if (is_UpLed == 0 && cnt > 300 && (millis() - eta) > 1) {
    cnt -= 2;
    analogWrite(ledPin, cnt);

    if (cnt <= 300) {
      is_UpLed = 1;
    }
    eta = millis();
  }
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
  /* Set up the outputs. LED is active-low */
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, LOW);

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(callback);

  eta = millis();

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
      client.subscribe(lampaTopic.command, 1);
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

  if (!isON) {
    pulsate();
  }

  if (digitalRead(button) == 0 && lastRead == 1) {

    if (digitalRead(relayPin) == 1) {
      client.publish(lampaTopic.command, "0#Lamp OFF - physical switch", true);
      isON = 0;
    }
    else {
      client.publish(lampaTopic.command, "1#Lamp ON - physical switch", true);
      isON = 1;
    }
    lastRead = 0;
    delay(50);
  }

  if (digitalRead(button) == 1 && lastRead == 0) {
    lastRead = 1;
    delay(50);
  }
}
