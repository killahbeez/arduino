#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Gsender.h>
#include <Ticker.h>

Ticker ticker;
Ticker ticker1;
static boolean send_email = 0;

Adafruit_NeoPixel pixels_1 = Adafruit_NeoPixel(68, D1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(26, D2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_3 = Adafruit_NeoPixel(40, D3, NEO_GRB + NEO_KHZ800);

const char* ssid = "UPCD476AA2";
const char* password = "Fr4vebAeepuk";

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
typedef struct {
  char* command; //subscribe for commands
  char* state; //publish the state
  char* commandDim; //subscribe for command Dim
  char* stateDim; //publish the state
  char* debug; //publish debug message
  char* name;
  uint8_t cnt;
} Topics;

Topics neopixel_1_topic = {
  "/home/birou/neopixel_1/command",
  "/home/birou/neopixel_1/state",
  "/home/birou/neopixel_1/commandDim",
  "/home/birou/neopixel_1/stateDim",
  "/home/debug",
  "Led geam",
  1
};

Topics neopixel_2_topic = {
  "/home/birou/neopixel_2/command",
  "/home/birou/neopixel_2/state",
  "/home/birou/neopixel_2/commandDim",
  "/home/birou/neopixel_2/stateDim",
  "/home/debug",
  "Led scule",
  2
};

Topics neopixel_3_topic = {
  "/home/birou/neopixel_3/command",
  "/home/birou/neopixel_3/state",
  "/home/birou/neopixel_3/commandDim",
  "/home/birou/neopixel_3/stateDim",
  "/home/debug",
  "Led comoda",
  2
};

typedef struct {
  uint8_t R;
  uint8_t G;
  uint8_t B;
} hex_colors;

typedef struct {
  uint8_t is_turn;
  String RGB;
  uint8_t min;
  uint8_t max;
} commands;

typedef struct {
  uint8_t is_turn;
  uint8_t from_dim;
  uint8_t to_dim;
  hex_colors RGB;
  uint8_t min;
  uint8_t max;
  boolean from_setup;
} neopixel_state;

typedef struct {
  neopixel_state neopixel_1;
  neopixel_state neopixel_2;
  neopixel_state neopixel_3;
} curr_state;

curr_state current_state;

IPAddress broker(192, 168, 0, 16);       // Address of the MQTT broker
#define CLIENT_ID "client-neopixel" // Client ID to send to the broker

boolean restart = 1;
boolean get_restart_cnt = 0;
char* topic_restart_cnt = "/home/neopixel_restart_cnt";
uint32_t cnt_restart = 0;

typedef struct {
        char **arr;
        uint32_t len;
} str_delim_t;

str_delim_t *explode(char *str, char *delim){
        str_delim_t *str_delim = (str_delim_t *) calloc(1,sizeof(str_delim_t));
        if(str_delim != NULL){
                uint16_t str_len = strlen(str);
                char *initial_str = (char *)malloc(str_len+1);

                if(initial_str != NULL){

                        initial_str[str_len] = '\0';
                        strcpy(initial_str,str);

                        char *token;
                        token = strtok(str, delim);

                        while( token != NULL ) {
                                str_delim->len++;
                                str_delim->arr = (char **)realloc(str_delim->arr,str_delim->len*sizeof(char *));
                                if(str_delim->arr == NULL){
                                        return NULL;
                                }
                                str_delim->arr[str_delim->len-1] = (char *) malloc(strlen(token)+1);
                                strcpy(str_delim->arr[str_delim->len-1],token);
                                token = strtok(NULL, delim);
                        }

                        strcpy(str,initial_str);
                        free(initial_str);
                }
        }

        return str_delim;
}

void free_explode(str_delim_t *str_delim){
        if(str_delim->arr != NULL && str_delim->len > 0){
                for(int i=0;i<str_delim->len;i++){
                        free(str_delim->arr[i]);
                }
                free(str_delim->arr);
        }
        if(str_delim != NULL){
                free(str_delim);
        }
}

/**
   MQTT callback to process messages
*/
void callback(char* topic, byte* payload, unsigned int length) {
  commands getCommand;
  payload[length] = '\0';

  ticker.detach();
  if (strcmp(topic, neopixel_1_topic.command) == 0 || strcmp(topic, neopixel_2_topic.command) == 0 ||  strcmp(topic, neopixel_3_topic.command) == 0) {

    str_delim_t *str_delim = explode((char*) payload,"#");

    if ((String)str_delim->arr[0] == "turn") { //if the command is turn
      getCommand.is_turn = atoi(str_delim->arr[1]);
      getCommand.RGB = (String)str_delim->arr[2];
      getCommand.min = atoi(str_delim->arr[3]);
      getCommand.max = atoi(str_delim->arr[4]);

      if (strcmp(topic, neopixel_1_topic.command) == 0) {
        Serial.println(getCommand.is_turn);
        setNeopixel(neopixel_1_topic, pixels_1, current_state.neopixel_1, getCommand);
      }
      if (strcmp(topic, neopixel_2_topic.command) == 0) {
        Serial.println(getCommand.is_turn);
        setNeopixel(neopixel_2_topic, pixels_2, current_state.neopixel_2, getCommand);
      }
      if (strcmp(topic, neopixel_3_topic.command) == 0) {
        Serial.println(getCommand.is_turn);
        setNeopixel(neopixel_3_topic, pixels_3, current_state.neopixel_3, getCommand);
      }
    }
    
    free_explode(str_delim);
  }

  if (strcmp(topic, neopixel_1_topic.commandDim) == 0 || strcmp(topic, neopixel_2_topic.commandDim) == 0 || strcmp(topic, neopixel_3_topic.commandDim) == 0) {
    //Serial.println(atoi((char*)payload));
    if (strcmp(topic, neopixel_1_topic.commandDim) == 0) {
      dimNeopixel(neopixel_1_topic, pixels_1, current_state.neopixel_1, (char*)payload);
    }
    if (strcmp(topic, neopixel_2_topic.commandDim) == 0) {
      dimNeopixel(neopixel_2_topic, pixels_2, current_state.neopixel_2, (char*)payload);
    }
    if (strcmp(topic, neopixel_3_topic.commandDim) == 0) {
      dimNeopixel(neopixel_3_topic, pixels_3, current_state.neopixel_3, (char*)payload);
    }
  }

  if (strcmp(topic, topic_restart_cnt) == 0 ) {
    cnt_restart = atoi((char*) payload);
    get_restart_cnt = 1;
  }

  ticker.attach(0.01, callback_ticker);
}

void dimNeopixel(Topics neopixelTopic, Adafruit_NeoPixel& pixels, neopixel_state& neopixel_state, char* dim_value) {
  uint8_t dim_value_to_int = atoi(dim_value);
  char buffer[30];

  neopixel_state.to_dim = dim_value_to_int;
  sprintf(buffer, "%s", dim_value);
  client.publish(neopixelTopic.stateDim, buffer, true);

  sprintf(buffer, "%s Dim %d", neopixelTopic.name, dim_value_to_int);
  client.publish(neopixelTopic.debug, buffer, true);
}

void setNeopixel(Topics neopixelTopic, Adafruit_NeoPixel& pixels, neopixel_state& neopixel_state, commands getCommand) {
  static uint32_t color;
  static hex_colors RGB_colors;

  RGB_colors = getRGB((char*) getCommand.RGB.c_str());

  neopixel_state.RGB = RGB_colors;
  neopixel_state.min = getCommand.min;
  neopixel_state.max = getCommand.max;

  if (getCommand.is_turn == 1) {
    pixels.clear();
    Serial.println("dexter");
    if (neopixel_state.is_turn == 0) { //if previous was turned off start from dim 1
      neopixel_state.from_dim = 1;
      pixels.setBrightness(1);
    }
    color  = pixels.Color(RGB_colors.R, RGB_colors.G, RGB_colors.B);
    for (uint8_t cnt = getCommand.min - 1; cnt < getCommand.max; cnt++) {
      pixels.setPixelColor(cnt, color);
    }
  }
  else if(neopixel_state.from_setup){
    neopixel_state.from_setup = 0;
    pixels.clear();
  }
  pixels.show();

  neopixel_state.is_turn = getCommand.is_turn;

  char buffer[100];
  sprintf(buffer, "turn#%d#%s#%d#%d", getCommand.is_turn, getCommand.RGB.c_str(), getCommand.min, getCommand.max);

  client.publish(neopixelTopic.state, buffer, true);

  sprintf(buffer, "%s <div class='debug_color' style='background:#%s;'></div> %s Min: %d Max: %d", neopixelTopic.name, getCommand.RGB.c_str(), (getCommand.is_turn == 1) ? "ON" : "OFF", getCommand.min, getCommand.max);
  client.publish(neopixelTopic.debug, buffer, true);
}

hex_colors getRGB(char* RGB) {
  hex_colors RGB_colors;

  char buffer[2];
  sprintf(buffer, "%c%c", RGB[0], RGB[1]);
  RGB_colors.R = (uint8_t)strtol(buffer, NULL, 16);
  sprintf(buffer, "%c%c", RGB[2], RGB[3]);
  RGB_colors.G = (uint8_t)strtol(buffer, NULL, 16);
  sprintf(buffer, "%c%c", RGB[4], RGB[5]);
  RGB_colors.B = (uint8_t)strtol(buffer, NULL, 16);

  return RGB_colors;
}

void initialize_pixel(Adafruit_NeoPixel& pixels,uint32_t color) {
  
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(5);
  
  for (uint8_t cnt = 0; cnt < pixels.numPixels(); cnt++) {
    pixels.setPixelColor(cnt, color);
  }

  pixels.show();
}


void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  ArduinoOTA.setPassword("dune1234");
  ArduinoOTA.begin();

  Serial.println("");

  pinMode(D5, OUTPUT);
  digitalWrite(D5, LOW);
  
  current_state.neopixel_1.from_setup = 1;
  current_state.neopixel_2.from_setup = 1;
  current_state.neopixel_3.from_setup = 1;
  initialize_pixel(pixels_1, 0xff0000);
  initialize_pixel(pixels_2, 0x00ff00);
  initialize_pixel(pixels_3, 0x0000ff);

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

  ticker.attach(0.01, callback_ticker);
  ticker1.attach(10, callback_ticker_wdt);
}

boolean tick = 0;
void callback_ticker() {
  tick = 1;
}

volatile boolean tick_wdt = 0;
void callback_ticker_wdt() {
  tick_wdt = 1;
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
      client.subscribe(neopixel_1_topic.command);
      client.subscribe(neopixel_1_topic.commandDim);
      client.subscribe(neopixel_2_topic.command);
      client.subscribe(neopixel_2_topic.commandDim);
      client.subscribe(neopixel_3_topic.command);
      client.subscribe(neopixel_3_topic.commandDim);
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


    ///ping watchdog timer
    if (tick_wdt) {
      tick_wdt = 0;
      digitalWrite(D5, HIGH);
      delay(20);
      digitalWrite(D5, LOW);
    }
  }

  if (restart && get_restart_cnt) {
    char buffer[30];
    sprintf(buffer, "%d", ++cnt_restart);
    client.publish(topic_restart_cnt, buffer, true);
    restart = 0;
  }

  if (tick == 1) {
    fadeNeopixel(pixels_1, current_state.neopixel_1);
    fadeNeopixel(pixels_2, current_state.neopixel_2);
    fadeNeopixel(pixels_3, current_state.neopixel_3);
    tick = 0;
  }

  digitalWrite(D5, LOW);
}

void fadeNeopixel(Adafruit_NeoPixel& pixels, neopixel_state& neopixel_state) {
  static uint32_t color;

  if (neopixel_state.is_turn == 1 && neopixel_state.from_dim != neopixel_state.to_dim) {
    if (neopixel_state.from_dim < neopixel_state.to_dim) {
      neopixel_state.from_dim++;
      pixels.clear();
      pixels.setBrightness(neopixel_state.from_dim);

      color  = pixels.Color(neopixel_state.RGB.R, neopixel_state.RGB.G, neopixel_state.RGB.B);
      for (uint8_t cnt = neopixel_state.min - 1; cnt < neopixel_state.max; cnt++) {
        pixels.setPixelColor(cnt, color);
      }

      pixels.show();
      /*Serial.print(neopixel_state.from_dim);
        Serial.print("\t");
        Serial.print(pixels.getBrightness());
        Serial.print("\t");
        Serial.println(neopixel_state.to_dim);*/
    }
    if (neopixel_state.from_dim > neopixel_state.to_dim) {
      neopixel_state.from_dim--;
      pixels.clear();
      pixels.setBrightness(neopixel_state.from_dim);

      color  = pixels.Color(neopixel_state.RGB.R, neopixel_state.RGB.G, neopixel_state.RGB.B);
      for (uint8_t cnt = neopixel_state.min - 1; cnt < neopixel_state.max; cnt++) {
        pixels.setPixelColor(cnt, color);
      }

      pixels.show();
      /*Serial.print(neopixel_state.from_dim);
        Serial.print("\t");
        Serial.print(pixels.getBrightness());
        Serial.print("\t");
        Serial.println(neopixel_state.to_dim);*/
    }
  }

  if (neopixel_state.is_turn == 0 && neopixel_state.from_dim > 0) {
    neopixel_state.from_dim--;
    pixels.clear();
    pixels.setBrightness(neopixel_state.from_dim);

    color  = pixels.Color(neopixel_state.RGB.R, neopixel_state.RGB.G, neopixel_state.RGB.B);
    for (uint8_t cnt = neopixel_state.min - 1; cnt < neopixel_state.max; cnt++) {
      pixels.setPixelColor(cnt, color);
    }

    pixels.show();
    /*Serial.print(neopixel_state.from_dim);
      Serial.print("\t");
      Serial.print(pixels.getBrightness());
      Serial.print("\t");
      Serial.println(neopixel_state.to_dim);*/

    if (neopixel_state.from_dim == 0) {
      //Serial.println("dididi");
      pixels.clear();
      pixels.show();
    }
  }
}

