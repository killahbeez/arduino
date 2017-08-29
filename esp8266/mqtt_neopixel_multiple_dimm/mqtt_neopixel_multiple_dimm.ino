#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <explode.h>
#include <Gsender.h>


static boolean send_email = 0;

Adafruit_NeoPixel pixels_1 = Adafruit_NeoPixel(8, D2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(8, D1, NEO_GRB + NEO_KHZ800);

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
// command format   turn#1#FF0000  => turn neopixel ON with color RED
//                  dim#50    => dim neopixel 50%
struct Topics {
  char* command; //subscribe for commands
  char* state; //publish the state
  char* commandDim; //subscribe for command Dim
  char* stateDim; //publish the state
  char* debug; //publish debug message
  uint8_t cnt;
};

struct Topics neopixel_1_topic = {
    "/home/birou/neopixel_1/command",
    "/home/birou/neopixel_1/state",
    "/home/birou/neopixel_1/commandDim",
    "/home/birou/neopixel_1/stateDim",
    "/home/debug",
    1
};

struct Topics neopixel_2_topic = {
    "/home/birou/neopixel_2/command",
    "/home/birou/neopixel_2/state",
    "/home/birou/neopixel_2/commandDim",
    "/home/birou/neopixel_2/stateDim",
    "/home/debug",
    2
};

struct hex_colors {
  uint8_t R;
  uint8_t G;
  uint8_t B;
};

IPAddress broker(192, 168, 0, 16);       // Address of the MQTT broker
#define CLIENT_ID "client-neopixel" // Client ID to send to the broker

/**
   MQTT callback to process messages
*/
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';

  if (strcmp(topic, neopixel_1_topic.command) == 0 || strcmp(topic, neopixel_2_topic.command) == 0) {
    char **arr = NULL;
    int count = 0;
    explode((char*) payload, "#", &arr, &count);

    if ((String)arr[0] == "turn") { //if the command is turn

      if(strcmp(topic, neopixel_1_topic.command) == 0){
        setNeopixel(neopixel_1_topic,pixels_1,(String)arr[2],atoi(arr[1]));
      }
      if(strcmp(topic, neopixel_2_topic.command) == 0){
        setNeopixel(neopixel_2_topic,pixels_2,(String)arr[2],atoi(arr[1]));
      }
      send_email = 1;
    }
  }

  if (strcmp(topic, neopixel_1_topic.commandDim) == 0 || strcmp(topic, neopixel_2_topic.commandDim) == 0) {
    Serial.println(atoi((char*)payload));
    if(strcmp(topic, neopixel_1_topic.commandDim) == 0){
      dimNeopixel(neopixel_1_topic,pixels_1,(char*)payload);
    }
    if(strcmp(topic, neopixel_2_topic.commandDim) == 0){
      dimNeopixel(neopixel_2_topic,pixels_2,(char*)payload);
    }
  }
}

void dimNeopixel(struct Topics neopixelTopic, Adafruit_NeoPixel& pixels, char* dim_value) {
  uint16_t dim_value_to_int = atoi(dim_value);
  char buffer[30];

  sprintf(buffer, "%s", dim_value);
  client.publish(neopixelTopic.stateDim, buffer, true);

  sprintf(buffer, "Neopixel %d Dim %d", neopixelTopic.cnt, dim_value_to_int);
  client.publish(neopixelTopic.debug, buffer, true);

  pixels.setBrightness(dim_value_to_int);
  pixels.show();

}

void setNeopixel(struct Topics neopixelTopic, Adafruit_NeoPixel& pixels, String RGB, uint8_t is_turn) {
  static uint32_t color;
  static struct hex_colors RGB_colors;
  
  RGB_colors = getRGB((char*) RGB.c_str());

  pixels.clear();
  if(is_turn == 1){
    color  = pixels.Color(RGB_colors.R, RGB_colors.G, RGB_colors.B);
    for (uint8_t cnt = 0; cnt < 8; cnt++) {
      pixels.setPixelColor(cnt, color);
    }
  }
  pixels.show();

  char buffer[100];
  sprintf(buffer, "turn#%d#%s", is_turn, RGB.c_str());
  
  client.publish(neopixelTopic.state, buffer, true);
  
  sprintf(buffer, "Neopixel %d <div class='debug_color' style='background:#%s;'></div> %s", neopixelTopic.cnt, RGB.c_str(), (is_turn == 1)?"ON":"OFF");
  client.publish(neopixelTopic.debug, buffer, true);
}

struct hex_colors getRGB(char* RGB){
  struct hex_colors RGB_colors;
  
  char buffer[2];
  sprintf(buffer,"%c%c",RGB[0],RGB[1]);
  RGB_colors.R = (uint8_t)strtol(buffer, NULL, 16);
  sprintf(buffer,"%c%c",RGB[2],RGB[3]);
  RGB_colors.G = (uint8_t)strtol(buffer, NULL, 16);
  sprintf(buffer,"%c%c",RGB[4],RGB[5]);
  RGB_colors.B = (uint8_t)strtol(buffer, NULL, 16);

  return RGB_colors;
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

  pixels_1.begin();
  pixels_1.setBrightness(10);
  pixels_1.clear();
  pixels_1.show();
  pixels_2.begin();
  pixels_2.setBrightness(10);
  pixels_2.clear();
  pixels_2.show();
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
      client.subscribe(neopixel_1_topic.command);
      client.subscribe(neopixel_1_topic.commandDim);
      client.subscribe(neopixel_2_topic.command);
      client.subscribe(neopixel_2_topic.commandDim);
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
    // SENDING EMAIL
    
    if (send_email) {
      Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
      String subject = "Test";
      if (gsender->Subject(subject)->Send("adrian.carstea@gmail.com", "<div style='border:1px solid black;font-weight:bold;'>Neopixel ON</div>")) {
        Serial.println("Message send.");
        client.publish("/home/debug", "Neopixel ON send email");
      } else {
        Serial.print("Error sending message: ");
        Serial.println(gsender->getError());
        client.publish("/home/debug", gsender->getError());
      }
      send_email = 0;
    }

    // ADC
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
}

