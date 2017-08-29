#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>
#include <Wire.h>

const char* ssid = "UPC3165272";
const char* password = "DAGKFDCC";

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);


#define I2CAddressESPWifi 16

void setup() {
  Wire.begin(D2, D1);

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
  }

  timeClient.begin();
  
  Wire.beginTransmission(I2CAddressESPWifi);
  Wire.write("Starting transmission...");
  Wire.endTransmission();
}

void loop() {

  timeClient.update();
  String ts = timeClient.getFormattedTime();
  Wire.beginTransmission(I2CAddressESPWifi);
  Wire.write(ts.c_str());
  Wire.endTransmission();

  delay(1000);
}
