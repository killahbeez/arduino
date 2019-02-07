#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <HTTPSRedirect.h>
#include <Ticker.h>
#include <ArduinoJson.h>

Ticker ticker;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, D2, NEO_GRB + NEO_KHZ800);

const char* ssid = "UPCD476AA2";
const char* password = "Fr4vebAeepuk";

IPAddress ip(192, 168, 0, 13);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

/* MQTT Settings */
WiFiClient wificlient;
PubSubClient client(wificlient);

IPAddress broker(192, 168, 0, 16);       // Address of the MQTT broker
#define CLIENT_ID "client-gmail" // Client ID to send to the broker

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const char *GScriptId = "AKfycbx5nZ67KUeHx8BPJKHOvgf5r8GLCK9Ieyqa1v7GUeHljwWziAs"; /// GS Exe Google Script App

const int httpsPort = 443;

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
// SHA1 fingerprint of the certificate
//SHA1 Fingerprint=DA:5C:50:06:98:6D:61:3F:C5:D0:0E:C6:68:AE:CD:D3:86:B0:D5:6B

//const char* fingerprint = "FD 8C AC 55 64 BE 30 57 9A 27 53 52 62 E1 CD 26 82 15 A2 DB";
const char* fingerprint = "DA:5C:50:06:98:6D:61:3F:C5:D0:0E:C6:68:AE:CD:D3:86:B0:D5:6B";

HTTPSRedirect clientAppsGoogle(httpsPort);

boolean restart = 1;
boolean get_restart_cnt = 0;
char* topic_restart_cnt = "/home/check_email_restart_cnt";
uint32_t cnt_restart = 0;
/**
   MQTT callback to process messages
*/
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  if (strcmp(topic, topic_restart_cnt) == 0 ) {
    cnt_restart = atoi((char*) payload);
    get_restart_cnt = 1;
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  ArduinoOTA.setPassword("dune1234");
  ArduinoOTA.begin();

  pinMode(D5, OUTPUT);
  digitalWrite(D5,LOW);
  
  pixels.begin();
  pixels.setBrightness(10);
  pixels.clear();
  pixels.show();
  
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

  // Use HTTPSRedirect class to create TLS connection to Google Apps Script
  reconnectGoogleApps();

  ticker.attach(10, callback_ticker);
}

int8_t getMSBpos(uint8_t nr) {
  static int8_t pos;

  for (int8_t cnt = 0; cnt < 8; cnt++) {
    if ( (nr >> cnt) >= 1) {
      pos = cnt;
    }
  }
  return (pos + 1);
}

void setNeopixel(uint8_t nr_emails) {
  static uint32_t colorRed  = pixels.Color(255, 0, 0);
  static uint32_t colorBlue  = pixels.Color(0, 0, 255);
  static int8_t posMSB = 0;

  pixels.clear();

  posMSB = getMSBpos(nr_emails);
  //posMSB = 8;
  for (uint8_t cnt = 0; cnt < posMSB; cnt++) {
    pixels.setPixelColor(cnt, (nr_emails >> cnt & 1) ? colorRed : colorBlue);
  }
  pixels.show();
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
      client.subscribe(topic_restart_cnt);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



/**
   Attempt connection to Google Apps Script
*/
void reconnectGoogleApps() {
  Serial.print("Connecting to ");
  Serial.println(host);

  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = clientAppsGoogle.connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  Serial.flush();
  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }

  Serial.flush();
  if (clientAppsGoogle.verify(fingerprint, host)) {
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }
}

volatile boolean tick = 0;
void callback_ticker() {
  tick = 1;
}

void loop() {
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

    if (!clientAppsGoogle.connected()) {
      reconnectGoogleApps();
    }
  }

  if (client.connected()) {
    client.loop();
  }

  if (clientAppsGoogle.connected() && tick) {
    static uint8_t last_cnt_threads_unread_primary = 0;

    //check for Unread Primary messages
    String urlUnreadPrimary =  String("/macros/s/") + GScriptId + "/exec?write=getUnreadPrimary&toCell=C3";
    clientAppsGoogle.printRedir(urlUnreadPrimary, host, googleRedirHost);

    StaticJsonBuffer<1000> jsonBufferUnreadPrimary;
    JsonObject& rootUnread = jsonBufferUnreadPrimary.parseObject(clientAppsGoogle.getResponse());

    if (last_cnt_threads_unread_primary != rootUnread["threadsPrimary"]) {
      if (rootUnread["threadsPrimary"] > 0) {
        char buffer[100];
        setNeopixel((int)rootUnread["threadsPrimary"]);

        sprintf(buffer, "You have %d INBOX emails", (int)rootUnread["threadsPrimary"]);
        client.publish("/home/debug", buffer);
      }
      else {
        pixels.clear();
        pixels.show();
        client.publish("/home/debug", "NO new emails");
      }
      last_cnt_threads_unread_primary = rootUnread["threadsPrimary"];
    }


    Serial.println("##########################################");
    Serial.println((int)rootUnread["threadsPrimary"]);
    Serial.println("##########################################");
    tick = 0;


    ///ping watchdog timer
    digitalWrite(D5, HIGH);
    delay(20);
    digitalWrite(D5, LOW);
  }
  digitalWrite(D5,LOW);
  
  if (restart && get_restart_cnt) {
    char buffer[30];
    sprintf(buffer, "%d", ++cnt_restart);
    client.publish(topic_restart_cnt, buffer, true);
    restart = 0;
  }

}
