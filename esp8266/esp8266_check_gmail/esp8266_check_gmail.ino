#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <ArduinoJson.h>

Ticker ticker;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, D2, NEO_GRB + NEO_KHZ800);

const char* ssid = "UPCD476AA2";
const char* password = "Fr4vebAeepuk";

IPAddress ip(192, 168, 0, 122);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

/* MQTT Settings */
//std::unique_ptr<BearSSL::WiFiClientSecure> wificlient(new BearSSL::WiFiClientSecure());
BearSSL::WiFiClientSecure wificlient;


WiFiClient wificlientNOSSL;
PubSubClient client(wificlientNOSSL);

IPAddress broker(192, 168, 0, 16);       // Address of the MQTT broker
#define CLIENT_ID "client-gmail" // Client ID to send to the broker

const char* host = "script.google.com";
const int httpsPort = 443;
const char* googleRedirHost = "script.googleusercontent.com";
const char *GScriptId = "AKfycbwCPTk8AKsi2lMBSooBb2sD8F7moVpN-co3scUSc_MhTlf-Xs66oxI5pna1BLErjMV9"; /// GS Exe Google Script App

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
// SHA1 fingerprint of the certificate
/*const char* fingerprint = "2C:0B:CF:F3:2B:F7:F0:76:D4:B1:1F:DB:E2:21:58:B5:EC:B7:6A:FF";

const char* root_ca PROGMEM = R"EOF(
        -----BEGIN CERTIFICATE-----
        MIINhzCCDS6gAwIBAgIRANVXCHkKVuT7ErpqC4E8o+AwCgYIKoZIzj0EAwIwOzEL
        MAkGA1UEBhMCVVMxHjAcBgNVBAoTFUdvb2dsZSBUcnVzdCBTZXJ2aWNlczEMMAoG
        A1UEAxMDV0UyMB4XDTI1MTAxMzA4Mzc0NloXDTI2MDEwNTA4Mzc0NVowFzEVMBMG
        A1UEAwwMKi5nb29nbGUuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE1l22
        CpbQywaaPnyQ2BKjQL3vkkV52/28sUO+gq4hxEkkD5kmhK8s7J/95ugE7Okjj+oi
        mEeom+ZVjfE1NZMNIKOCDDUwggwxMA4GA1UdDwEB/wQEAwIHgDATBgNVHSUEDDAK
        BggrBgEFBQcDATAMBgNVHRMBAf8EAjAAMB0GA1UdDgQWBBSd0+bmm/WgOrvdwD6d
        v821J6HxFzAfBgNVHSMEGDAWgBR1vsR3ron2RDd9z7FoHx0a69w0WTBYBggrBgEF
        BQcBAQRMMEowIQYIKwYBBQUHMAGGFWh0dHA6Ly9vLnBraS5nb29nL3dlMjAlBggr
        BgEFBQcwAoYZaHR0cDovL2kucGtpLmdvb2cvd2UyLmNydDCCCgsGA1UdEQSCCgIw
        ggn+ggwqLmdvb2dsZS5jb22CFiouYXBwZW5naW5lLmdvb2dsZS5jb22CCSouYmRu
        LmRldoIVKi5vcmlnaW4tdGVzdC5iZG4uZGV2ghIqLmNsb3VkLmdvb2dsZS5jb22C
        GCouY3Jvd2Rzb3VyY2UuZ29vZ2xlLmNvbYIYKi5kYXRhY29tcHV0ZS5nb29nbGUu
        Y29tggsqLmdvb2dsZS5jYYILKi5nb29nbGUuY2yCDiouZ29vZ2xlLmNvLmlugg4q
        Lmdvb2dsZS5jby5qcIIOKi5nb29nbGUuY28udWuCDyouZ29vZ2xlLmNvbS5hcoIP
        Ki5nb29nbGUuY29tLmF1gg8qLmdvb2dsZS5jb20uYnKCDyouZ29vZ2xlLmNvbS5j
        b4IPKi5nb29nbGUuY29tLm14gg8qLmdvb2dsZS5jb20udHKCDyouZ29vZ2xlLmNv
        bS52boILKi5nb29nbGUuZGWCCyouZ29vZ2xlLmVzggsqLmdvb2dsZS5mcoILKi5n
        b29nbGUuaHWCCyouZ29vZ2xlLml0ggsqLmdvb2dsZS5ubIILKi5nb29nbGUucGyC
        CyouZ29vZ2xlLnB0gg8qLmdvb2dsZWFwaXMuY26CESouZ29vZ2xldmlkZW8uY29t
        ggwqLmdzdGF0aWMuY26CECouZ3N0YXRpYy1jbi5jb22CD2dvb2dsZWNuYXBwcy5j
        boIRKi5nb29nbGVjbmFwcHMuY26CEWdvb2dsZWFwcHMtY24uY29tghMqLmdvb2ds
        ZWFwcHMtY24uY29tggxna2VjbmFwcHMuY26CDiouZ2tlY25hcHBzLmNughJnb29n
        bGVkb3dubG9hZHMuY26CFCouZ29vZ2xlZG93bmxvYWRzLmNughByZWNhcHRjaGEu
        bmV0LmNughIqLnJlY2FwdGNoYS5uZXQuY26CEHJlY2FwdGNoYS1jbi5uZXSCEiou
        cmVjYXB0Y2hhLWNuLm5ldIILd2lkZXZpbmUuY26CDSoud2lkZXZpbmUuY26CEWFt
        cHByb2plY3Qub3JnLmNughMqLmFtcHByb2plY3Qub3JnLmNughFhbXBwcm9qZWN0
        Lm5ldC5jboITKi5hbXBwcm9qZWN0Lm5ldC5jboIXZ29vZ2xlLWFuYWx5dGljcy1j
        bi5jb22CGSouZ29vZ2xlLWFuYWx5dGljcy1jbi5jb22CF2dvb2dsZWFkc2Vydmlj
        ZXMtY24uY29tghkqLmdvb2dsZWFkc2VydmljZXMtY24uY29tghFnb29nbGV2YWRz
        LWNuLmNvbYITKi5nb29nbGV2YWRzLWNuLmNvbYIRZ29vZ2xlYXBpcy1jbi5jb22C
        EyouZ29vZ2xlYXBpcy1jbi5jb22CFWdvb2dsZW9wdGltaXplLWNuLmNvbYIXKi5n
        b29nbGVvcHRpbWl6ZS1jbi5jb22CEmRvdWJsZWNsaWNrLWNuLm5ldIIUKi5kb3Vi
        bGVjbGljay1jbi5uZXSCGCouZmxzLmRvdWJsZWNsaWNrLWNuLm5ldIIWKi5nLmRv
        dWJsZWNsaWNrLWNuLm5ldIIOZG91YmxlY2xpY2suY26CECouZG91YmxlY2xpY2su
        Y26CFCouZmxzLmRvdWJsZWNsaWNrLmNughIqLmcuZG91YmxlY2xpY2suY26CEWRh
        cnRzZWFyY2gtY24ubmV0ghMqLmRhcnRzZWFyY2gtY24ubmV0gh1nb29nbGV0cmF2
        ZWxhZHNlcnZpY2VzLWNuLmNvbYIfKi5nb29nbGV0cmF2ZWxhZHNlcnZpY2VzLWNu
        LmNvbYIYZ29vZ2xldGFnc2VydmljZXMtY24uY29tghoqLmdvb2dsZXRhZ3NlcnZp
        Y2VzLWNuLmNvbYIXZ29vZ2xldGFnbWFuYWdlci1jbi5jb22CGSouZ29vZ2xldGFn
        bWFuYWdlci1jbi5jb22CGGdvb2dsZXN5bmRpY2F0aW9uLWNuLmNvbYIaKi5nb29n
        bGVzeW5kaWNhdGlvbi1jbi5jb22CJCouc2FmZWZyYW1lLmdvb2dsZXN5bmRpY2F0
        aW9uLWNuLmNvbYIWYXBwLW1lYXN1cmVtZW50LWNuLmNvbYIYKi5hcHAtbWVhc3Vy
        ZW1lbnQtY24uY29tggtndnQxLWNuLmNvbYINKi5ndnQxLWNuLmNvbYILZ3Z0Mi1j
        bi5jb22CDSouZ3Z0Mi1jbi5jb22CCzJtZG4tY24ubmV0gg0qLjJtZG4tY24ubmV0
        ghRnb29nbGVmbGlnaHRzLWNuLm5ldIIWKi5nb29nbGVmbGlnaHRzLWNuLm5ldIIM
        YWRtb2ItY24uY29tgg4qLmFkbW9iLWNuLmNvbYIZKi5nZW1pbmkuY2xvdWQuZ29v
        Z2xlLmNvbYIUZ29vZ2xlc2FuZGJveC1jbi5jb22CFiouZ29vZ2xlc2FuZGJveC1j
        bi5jb22CHiouc2FmZW51cC5nb29nbGVzYW5kYm94LWNuLmNvbYINKi5nc3RhdGlj
        LmNvbYIUKi5tZXRyaWMuZ3N0YXRpYy5jb22CCiouZ3Z0MS5jb22CESouZ2NwY2Ru
        Lmd2dDEuY29tggoqLmd2dDIuY29tgg4qLmdjcC5ndnQyLmNvbYIQKi51cmwuZ29v
        Z2xlLmNvbYIWKi55b3V0dWJlLW5vY29va2llLmNvbYILKi55dGltZy5jb22CCmFp
        LmFuZHJvaWSCC2FuZHJvaWQuY29tgg0qLmFuZHJvaWQuY29tghMqLmZsYXNoLmFu
        ZHJvaWQuY29tggRnLmNuggYqLmcuY26CBGcuY2+CBiouZy5jb4IGZ29vLmdsggp3
        d3cuZ29vLmdsghRnb29nbGUtYW5hbHl0aWNzLmNvbYIWKi5nb29nbGUtYW5hbHl0
        aWNzLmNvbYIKZ29vZ2xlLmNvbYISZ29vZ2xlY29tbWVyY2UuY29tghQqLmdvb2ds
        ZWNvbW1lcmNlLmNvbYIIZ2dwaHQuY26CCiouZ2dwaHQuY26CCnVyY2hpbi5jb22C
        DCoudXJjaGluLmNvbYIIeW91dHUuYmWCC3lvdXR1YmUuY29tgg0qLnlvdXR1YmUu
        Y29tghFtdXNpYy55b3V0dWJlLmNvbYITKi5tdXNpYy55b3V0dWJlLmNvbYIUeW91
        dHViZWVkdWNhdGlvbi5jb22CFioueW91dHViZWVkdWNhdGlvbi5jb22CD3lvdXR1
        YmVraWRzLmNvbYIRKi55b3V0dWJla2lkcy5jb22CBXl0LmJlggcqLnl0LmJlghph
        bmRyb2lkLmNsaWVudHMuZ29vZ2xlLmNvbYITKi5hbmRyb2lkLmdvb2dsZS5jboIS
        Ki5jaHJvbWUuZ29vZ2xlLmNughYqLmRldmVsb3BlcnMuZ29vZ2xlLmNughUqLmFp
        c3R1ZGlvLmdvb2dsZS5jb20wEwYDVR0gBAwwCjAIBgZngQwBAgEwNgYDVR0fBC8w
        LTAroCmgJ4YlaHR0cDovL2MucGtpLmdvb2cvd2UyL3h1enQzUFU5Rl93LmNybDCC
        AQQGCisGAQQB1nkCBAIEgfUEgfIA8AB2ABaDLavwqSUPD/A6pUX/yL/II9CHS/YE
        KSf45x8zE/X6AAABmdzu6SYAAAQDAEcwRQIgUbaGHlT0xnvcjqlVk4D59sPiCu1f
        eS0z5pOn9ZqBm74CIQDZOa37lSBcFbWumMDyEzgzMxrpjFNbusm4l3dI+FVL2AB2
        AA5XlLzzrqk+MxssmQez95Dfm8I9cTIl3SGpJaxhxU4hAAABmdzu6P0AAAQDAEcw
        RQIgfnbgxYmgdomns1SKkwQr5ksFJ8RfAJyVnUSNQ8cWlTgCIQCyAo5BdGkLBZ/B
        fCSLdpKuJ+3iqmny3SKF94k7DW7KqDAKBggqhkjOPQQDAgNHADBEAiAEBt+Rx2tH
        /R8PKusV/1uThSH4WCXBGU6Qw99R74efxQIgY+/RWxYdtDMzx5RPtBEusAcTVddY
        lGVecHDvi51K9kg=
        -----END CERTIFICATE-----
        )EOF";
*/



boolean restart = 1;
boolean get_restart_cnt = 0;
const char* topic_restart_cnt = "/home/check_email_restart_cnt";
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

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(callback);

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
      delay(1000);
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

String getResponse(WiFiClientSecure& client) {
  String response = "";
  
  // Așteaptă până când clientul este conectat
  while (client.connected()) {
    // Citește fiecare caracter disponibil și adaugă-l la response
    while (client.available()) {
      char c = client.read();
      response += c;  // Adaugă caracterul citit la răspuns
    }
  }
  
  return response;
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

    if (!client.connected()) {
      reconnect();
    }
    
    if (!wificlient.connected()) {
      reconnectGoogleApps();
    }
  }

  if (client.connected()) {
    client.loop();
  }

  if (wificlient.connected() && tick) {
    static uint8_t last_cnt_threads_unread_primary = 0;

    //check for Unread Primary messages
    String urlUnreadPrimary =  String("/macros/s/") + GScriptId + "/exec?write=getUnreadPrimary&toCell=C3";
    /*
    wificlient.print("GET " + String(urlUnreadPrimary) + " HTTP/1.1\r\n");
    wificlient.print("Host: " + String(host) + "\r\n");
    wificlient.print("Connection: close\r\n\r\n");*/

    /*StaticJsonBuffer<1000> jsonBufferUnreadPrimary;

    String serverResponse = getResponse(wificlient);
    Serial.println(serverResponse);  // Afișează răspunsul complet în Serial Monitor*/

    
    String serverResponse = httpGETFollowRedirect(host, urlUnreadPrimary);
    Serial.println(urlUnreadPrimary);
    Serial.println("___________________________________________________________________");  // Afișează răspunsul complet în Serial Monitor
    Serial.println(serverResponse);  // Afișează răspunsul complet în Serial Monitor

    StaticJsonBuffer<1000> jsonBufferUnreadPrimary;
    JsonObject& rootUnread = jsonBufferUnreadPrimary.parseObject(serverResponse);
    Serial.println("threadsPrimary: " + String(rootUnread["threadsPrimary"]));

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
