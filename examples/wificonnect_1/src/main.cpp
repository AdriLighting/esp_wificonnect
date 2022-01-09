#include <Arduino.h>
#include <wificonect.h>

WifiConnect * _DeviceWifi;

#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
// #include <LittleFS.h>
// #include <WiFiUdp.h>
// #include <NTPClient.h>

void setup() {
  Serial.begin(115200);

  for(unsigned long const serialBeginTime = millis(); !Serial && (millis() - serialBeginTime > 5000); ) { }
  delay(300);

  Serial.println(F("\n##################################################\n"));  

  Serial.setDebugOutput(true);

  // LittleFS.begin();

  _DeviceWifi   = new WifiConnect();
   
}

void loop() {
    _DeviceWifi->handleConnection();

    if (_DeviceWifi->WIFIsetupIsReady()) ArduinoOTA.handle();

    if (_DeviceWifi->needSTA()) {
      if (_DeviceWifi->STAisReady()) {

        MDNS.update();
        // __httpserver.loop();

      }
    } else {
      // if (_DeviceWifi->WIFIsetupIsReady()) __httpserver.loop();
    }


}
