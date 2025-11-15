#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <ArduinoJson.h>

Ticker ticker;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, D2, NEO_GRB + NEO_KHZ800);

const char* ssid = "UPCD476AA2";
const char* password = "Fr4vebAeepuk";

/*IPAddress ip(192, 168, 0, 122);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);*/

BearSSL::WiFiClientSecure wificlient;
WiFiClient wificlientNOSSL;

const char* host = "script.google.com";
const int httpsPort = 443;
const char* googleRedirHost = "script.googleusercontent.com";
const char *GScriptId = "AKfycbwCPTk8AKsi2lMBSooBb2sD8F7moVpN-co3scUSc_MhTlf-Xs66oxI5pna1BLErjMV9"; /// GS Exe Google Script App

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
// SHA1 fingerprint of the certificate
//const char* fingerprint = "2C:0B:CF:F3:2B:F7:F0:76:D4:B1:1F:DB:E2:21:58:B5:EC:B7:6A:FF";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  //nu-i convine lui script.google.comsa setez eu un ip dedicat
  //WiFi.config(ip, gateway, subnet);
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

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Connected to WiFi");
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Use HTTPSRedirect class to create TLS connection to Google Apps Script
  Serial.println("Try connecting to Google Script API");
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
   Attempt connection to Google Apps Script
*/
void reconnectGoogleApps() {
  Serial.print("Connecting to ");
  Serial.println(host);

 // Setează fingerprint-ul pentru a verifica serverul
  //wificlient.setFingerprint(fingerprint);
  //wificlient.setCACert(root_ca);
  
  wificlient.setInsecure();

  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = wificlient.connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      Serial.println("Connection OK!!");
      break;
    }
    else{
      Serial.println("Connection failed. Retrying...");
      delay(500);
    }
  }

  Serial.flush();
  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }

  Serial.flush();
}

volatile boolean tick = 0;
void callback_ticker() {
  tick = 1;
}

String removeChunkedEncoding(String body) {
    String result = "";
    int index = 0;
    
    while (true) {
      // Caut prima linie: dimensiunea chunk-ului în hex
      int endOfSize = body.indexOf("\r\n", index);
      if (endOfSize == -1) break;
    
      // Extrage dimensiunea chunk-ului
      String chunkSizeStr = body.substring(index, endOfSize);
      chunkSizeStr.trim();  // Elimină spații
      int chunkSize = strtol(chunkSizeStr.c_str(), NULL, 16);  // hex → int
    
      if (chunkSize == 0) break;  // ultimul chunk
    
      // Mută indexul după linia dimensiunii
      index = endOfSize + 2;
    
      // Extrage conținutul chunk-ului
      String chunkData = body.substring(index, index + chunkSize);
      result += chunkData;   // Adaugă conținut real
    
      // Mutăm indexul după conținut + "\r\n"
      index += chunkSize + 2;
    }
    
    return result;
}

String httpGETFollowRedirect(const char* host, const String &path) {
    String currentHost = host;
    String currentPath = path;
    //Serial.println("\n\n");


    wificlient.setInsecure();
    for (int i = 0; i < 10; i++) { // maxim 3 redirect-uri
        if (!wificlient.connect(/*currentHost.c_str()*/currentHost, 443)) {
            return "Eroare conectare";
        }
        /*else{
          Serial.println("Conexiune ok la google procedd cu GET\n\n");
        }*/

        wificlient.print(String("GET ") + currentPath + " HTTP/1.1\r\n" +
                     "Host: " + currentHost + "\r\n" +
                     "Connection: close\r\n\r\n");
                     
        //Serial.println(    String("GET ") + currentPath + " HTTP/1.1\r\n" + "Host: " + currentHost + "\r\n" + "Connection: close\r\n\r\n");

        String statusLine = wificlient.readStringUntil('\n');
        //Serial.println(statusLine);
        if (statusLine.indexOf("302") != -1) {
            //Serial.println("302 - urmeaza parsarea Location");
            
            // Citim header-ele
            String location = "";
            while (true) {
              String line = wificlient.readStringUntil('\n');
              line.trim();
              if (line.length() == 0) break; // sfârșit header
              if (line.startsWith("Location:")) {
                location = line.substring(9);
                location.trim();
              }
            }
            
            //Serial.println("Redirect detected to: " + location);
            // parse host și path din location
            int slashIndex = location.indexOf('/', 8); // după https://
            currentHost = location.substring(8, slashIndex);
            currentPath = location.substring(slashIndex);
            wificlient.stop(); // închide conexiunea și repetă
        } else {
            String response = "";
            while (wificlient.available()) {
                response += char(wificlient.read());
            }
            
            int bodyIndex = response.indexOf("\r\n\r\n"); // caută sfârșitul headerelor
            if (bodyIndex != -1) {
              return removeChunkedEncoding(response.substring(bodyIndex + 4));  // sari peste "\r\n\r\n"
            }


            return "No body";
        }
    }
    return "Too many redirects";
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
    if (!wificlient.connected()) {
      reconnectGoogleApps();
    }
  }

  if (wificlient.connected() && tick) {
    static uint8_t last_cnt_threads_unread_primary = 0;

    //check for Unread Primary messages
    String urlUnreadPrimary =  String("/macros/s/") + GScriptId + "/exec?write=getUnreadPrimary&toCell=C3";
    
    String serverResponse = httpGETFollowRedirect(host, urlUnreadPrimary);
    Serial.println(urlUnreadPrimary);
    Serial.println("___________________________________________________________________");  // Afișează răspunsul complet în Serial Monitor
    Serial.println(serverResponse);  // Afișează răspunsul complet în Serial Monitor

    StaticJsonBuffer<1000> jsonBufferUnreadPrimary;
    JsonObject& rootUnread = jsonBufferUnreadPrimary.parseObject(serverResponse);

    if (rootUnread["threadsPrimary"].is<int>()) {
      Serial.println("threadsPrimary: " + String(rootUnread["threadsPrimary"]));
  
      if (last_cnt_threads_unread_primary != rootUnread["threadsPrimary"]) {
        if (rootUnread["threadsPrimary"] > 0) {
          char buffer[100];
          setNeopixel((int)rootUnread["threadsPrimary"]);
          sprintf(buffer, "You have %d INBOX emails", (int)rootUnread["threadsPrimary"]);
        }
        else {
          pixels.clear();
          pixels.show();
        }
        last_cnt_threads_unread_primary = rootUnread["threadsPrimary"];
      }
      Serial.println("##########################################");
      Serial.println((int)rootUnread["threadsPrimary"]);
      Serial.println("##########################################");
    }
    else{
      Serial.println("##########################################");
      Serial.println("          Nu updateaza ledurile,ca nu a returnat scriptul json-ul scontat diverse motive");
      Serial.println("##########################################");
      
    }
    
    tick = 0;


    ///ping watchdog timer
    digitalWrite(D5, HIGH);
    delay(20);
    digitalWrite(D5, LOW);
  }
  digitalWrite(D5,LOW);

}
