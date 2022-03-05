/**
 * @file      wifi.h
 *
 * @brief     main file
 * @see       https://github.com/AdriLighting
 * 
 * @author    Adrien Grellard   
 * @date      sam. 08 dec. 2021 18:40:11
 *
 */

#include "wifi.h"

#ifdef ESP32
  #include <FS.h>
  #include <SPIFFS.h>
  #include <ESPmDNS.h>
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266mDNS.h>
#endif

extern "C"
{
  #include <user_interface.h>
}
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>

namespace {
  String ip2string(const IPAddress &a) {
      char buf[18];
      sprintf(buf,"%d.%d.%d.%d",a[0],a[1],a[2],a[3]);
      return String(buf);
  }
  String ch_toString(const char * c){
      return String((const __FlashStringHelper*) c);
  }  
}

void build_host_name(char * name, char * hostnamePrefix) {
  unsigned char mac[6];
  WiFi.macAddress(mac);
  strcpy(name, hostnamePrefix);
  for (int i=3; i<6; i++) {
    char b[3]; 
    sprintf(b,"%02x",mac[i]); 
    strcat(name, b); 
  }
}

WifiConnect::WifiConnect(){
  credential_1();

  sprintf(apPass, "%s", DEFAULT_AP_PASS);
  sprintf(otaPass, "%s", DEFAULT_OTA_PASS);
  sprintf(clientSSID, "%s", CLIENT_SSID);
  sprintf(clientPass, "%s", CLIENT_PASS);
  sprintf(hostName, "%s", ADS_NAME);

  String hostname = ch_toString(ADS_NAME);
  credential_2(hostname);
}
WifiConnect::WifiConnect(
  const char * const & Host, 
  const char * const & SSid, 
  const char * const & SSidPass, 
  const char * const & APpass , 
  const char * const & OTApass  ){
  credential_1();

  sprintf(apPass, "%s", APpass);
  sprintf(otaPass, "%s", OTApass);
  sprintf(clientSSID, "%s", SSid);
  sprintf(clientPass, "%s", SSidPass);
  sprintf(hostName, "%s", Host);

  String hostname = ch_toString(Host);
  credential_2(hostname);
}

void WifiConnect::credential_1(){
  apPass            = new char[65];
  otaPass           = new char[33];
  clientSSID        = new char[33];
  clientPass        = new char[65];
  cmDNS             = new char[33];
  serverDescription = new char[80];
  hostName          = new char[80];

  sprintf(cmDNS, "%s", "x");

  IPAddress staticIP      (  0,   0,  0,  0); // static IP of ESP
  IPAddress staticGateway (  0,   0,  0,  0); // gateway (router) IP
  IPAddress staticSubnet  (255, 255, 255, 0); 
  String staticIP_str       = ip2string(staticIP);
  String staticGateway_str  = ip2string(staticGateway);
  String staticSubnet_str   = ip2string(staticSubnet);  

  Serial.printf_P(PSTR("\n[staticIP: %s]"),       staticIP_str.c_str());
  Serial.printf_P(PSTR("\n[staticGateway: %s]"),  staticGateway_str.c_str());
  Serial.printf_P(PSTR("\n[staticSubnet: %s]"),   staticSubnet_str.c_str());  
}
void WifiConnect::credential_2(String hostname){
  String result = "";
  String apHost   = hostname;
  apHost.replace("_", "");
  apHost.toLowerCase();  
  byte  apHostLen = apHost.length();
  char  ch[apHostLen+1];
  byte  apHostMaxLen  = 8;
        apSSID        = new char[80];
  char  buffer[80];
  sprintf(ch, "%s", apHost.c_str());
  for(int i=0; i < 8; i++){ 
    result +=  String(ch[i]);
  }
  sprintf(buffer, "%s", result.c_str());
  sprintf(serverDescription, "%s", buffer);
  strcat(buffer, "-");
  build_host_name(apSSID, buffer) ;


  Serial.printf_P(PSTR("\n[otaPass: %s]"), otaPass);

  Serial.printf_P(PSTR("\n[clientSSID: %s]"), clientSSID);
  Serial.printf_P(PSTR("\n[clientPass: %s]"), clientPass);

  Serial.printf_P(PSTR("\n[hostname: %s]"),           hostname.c_str());
  Serial.printf_P(PSTR("\n[cmDNS: %s]"),              cmDNS);
  Serial.printf_P(PSTR("\n[serverDescription: %s]"),  serverDescription);

  Serial.printf_P(PSTR("\n[apSSID: %s]"), apSSID);
  Serial.printf_P(PSTR("\n[apPass: %s]"), apPass);

  escapedMac = WiFi.macAddress();
  escapedMac.replace(":", "");
  escapedMac.toLowerCase();
  Serial.printf_P(PSTR("\n[escapedMac: %s]"), escapedMac.c_str());

  Serial.printf("\n");

  delay(2);    
}
void WifiConnect::initSTA() {

  WiFi.disconnect(true);       

  #ifdef ESP8266
    WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  #endif

  if (staticIP[0] != 0 && staticGateway[0] != 0) {
    WiFi.config(staticIP, staticGateway, staticSubnet, IPAddress(1, 1, 1, 1));
  } else {
    WiFi.config(0U, 0U, 0U);
  }

  _STA.lastReconnectAttempt = millis();
  Serial.printf_P(PSTR("\n[IS] Connecting to %s ...\n"), clientSSID);

  if (_AP.active) {
    Serial.printf_P(PSTR("[IS] Access point disabled.\n"));
    Serial.printf_P(PSTR("\t[IS] Set _AP.active to FALSE\n"));
    WiFi.softAPdisconnect(true);
    _AP.active = false;
  }

  if (!_STA.active) {
    Serial.printf_P(PSTR("[IS] Wifi.mod set to WIFI_STA.\n"));
    Serial.printf_P(PSTR("\t[IS] Set _STA.active to TRUE\n"));
    WiFi.mode(WIFI_STA);
    _STA.active = true;
  }

  #ifdef ESP8266
    WiFi.hostname(hostName);
  #endif

  WiFi.begin(clientSSID, clientPass);

  #ifdef ARDUINO_ARCH_ESP32
    WiFi.setSleep(false);
    WiFi.setHostname(hostName);
  #else
    wifi_set_sleep_type(MODEM_SLEEP_T);
  #endif
}

void WifiConnect::initAP()
{

  Serial.printf_P(PSTR("[IA] Opening access point %s\n"), apSSID);
  // WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  
  IPAddress _staticIP(192, 168, 4, 1);; 
  IPAddress _subnet(255, 255, 255, 0);
  WiFi.softAPConfig(_staticIP, _staticIP, _subnet);

  WiFi.softAP(apSSID, apPass, apChannel, apHide);

  if (!_AP.active) {
    if (!_AP.serverInitialized) {
      Serial.printf_P(PSTR("[IA] Init AP interfaces\n"));
      _AP.serverInitialized = true;
      if (_APFUNC_INIT_SERVER!=nullptr) _APFUNC_INIT_SERVER();
    }
  }

  Serial.printf_P(PSTR("[IA] Set _AP.active to TRUE\n"));
  _AP.active = true;
}

void WifiConnect::loop_sta(const unsigned long & now ){
  static byte stacO = 0;

  if (_STA.lastReconnectAttempt == 0) initSTA();

  byte stac = 0;
  if (_AP.active) {
    #ifdef ESP8266
        stac = wifi_softap_get_station_num();
    #else
      wifi_sta_list_t stationList;
      esp_wifi_ap_get_sta_list(&stationList);
      stac = stationList.num;
    #endif
    if (stac != stacO) {
      stacO = stac;
      Serial.printf_P(PSTR("[LS] Connected AP clients: %d\n"), stac);
      if (!ADS_CONNECTED) {   
        if (stac){
          WiFi.disconnect();  
        }
      }
    }
  }

  if (!isConnected()) {
    if  ((_MOD == ADSWM_STA ) || (_MOD == ADSWM_STA_AP && !_AP.active)) {
      if (_STA.serverInitialized) {
        Serial.printf_P(PSTR("[LS] Disconnected!\n"));
        _STA.serverInitialized = false;
        _STA.reconnectAttempt = 0;
        initSTA();
      }
      if (now - _STA.lastReconnectAttempt > 18000) {
        _STA.reconnectAttempt++;
        Serial.printf_P(PSTR("[LS] ReconnectAttempt: %d\n"), _STA.reconnectAttempt );
        initSTA();
      }
    }
    if (_MOD == ADSWM_STA_AP) {
      if (_STA.reconnectAttempt>3) {
        _STA.reconnectAttempt = 0;
        WiFi.disconnect(true);
        #ifdef ESP8266
          WiFi.setPhyMode(WIFI_PHY_MODE_11N);
        #endif  
        WiFi.mode(WIFI_AP); 
        setup();       
        initAP();
      }     
    }
  } else if (!_STA.serverInitialized) {      
    setup(); 

    Serial.printf_P(PSTR("[LS] Connected! IP address:"));
    Serial.println(localIP());


    if (_AP.active) {
      WiFi.softAPdisconnect(true);
      _AP.active            = false;
      _AP.serverInitialized = false;
      Serial.printf_P(PSTR("[LS] Access point disabled.\n"));
    }

    // Set up mDNS responder:
    // "end" must be called before "begin" is called a 2nd time
    // see https://github.com/esp8266/Arduino/issues/7213
    MDNS.end(); 
    MDNS.begin(hostName);
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ads", "tcp", 80);
    MDNS.addServiceTxt("ads", "tcp", "mac", escapedMac.c_str());
    Serial.printf_P(PSTR("[LS] mDNS started\n"));

    if (_STAFUNC_INIT_SERVER!=nullptr) _STAFUNC_INIT_SERVER();

    _STA.serverInitialized  = true;
    _STA.wasConnected       = true;

  }
}

void WifiConnect::handleConnection(){
  static  unsigned long   heapTime  = 0;
  static  uint32_t        lastHeap  = UINT32_MAX;
          unsigned long   now       = millis();

  if (now - heapTime > 5000) {
    uint32_t heap = ESP.getFreeHeap();
    // Serial.printf_P(PSTR("[-L] Heap: %d\n"), heap);
    if (heap < JSON_BUFFER_SIZE+512 && lastHeap < JSON_BUFFER_SIZE+512) {
      Serial.printf_P(PSTR("[-L] Heap too low! %d\n"), heap);
      _forceReconnect = true;
    }
    lastHeap = heap;
    heapTime = now;
  }

  if (_forceReconnect) {
    Serial.printf_P(PSTR("[-L] Forcing reconnect.\n"));


    if (_MOD == ADSWM_STA_AP)  {
      _AP.serverInitialized     = false;
      _AP.wasConnected          = false;
      _STA.wasConnected         = false;
      _STA.serverInitialized    = false;
      _STA.lastReconnectAttempt = 0;
      _STA.reconnectAttempt     = 0;
      // initSTA();
    }
    if (_MOD == ADSWM_STA)  {
      _AP.serverInitialized     = false;
      _AP.wasConnected          = false;
      _STA.serverInitialized    = false;
      _STA.lastReconnectAttempt = 0;
      _STA.reconnectAttempt     = 0;
      // initSTA();
    }
    if (_MOD == ADSWM_AP)   {
      // _AP.serverInitialized   = false;
      // _AP.wasConnected        = false;
      // WiFi.disconnect(true);
      // WiFi.mode(WIFI_AP);       
      // initAP();
    } 

    _forceReconnect         = false;
    return;
  }

  if (_MOD == ADSWM_STA ||_MOD == ADSWM_STA_AP) {
    loop_sta(now);
  }
  if (_MOD == ADSWM_AP && !_AP.active ) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_AP);       
      initAP();    
  }

}

void WifiConnect::forceReconnect(){
  _forceReconnect = true;
}


void WifiConnect::setFunc_STAinitServer(callback_function_t f)  {_STAFUNC_INIT_SERVER = std::move(f);}
void WifiConnect::setFunc_APinitServer(callback_function_t f)   {_APFUNC_INIT_SERVER  = std::move(f);}


boolean WifiConnect::needSTA(){
  if (_MOD == ADSWM_STA ||_MOD == ADSWM_STA_AP) return true;
  return false;
}
boolean WifiConnect::STAisReady(){
  if (_STA.serverInitialized && _STA.wasConnected) return true;
  return false;
}
boolean WifiConnect::WIFIsetupIsReady(){
  return _isSetup;
}

void WifiConnect::get_staSSID(String & result)  {result = String(clientSSID);}
void WifiConnect::get_staPsk(String & result)   {result = String(clientPass);}
void WifiConnect::get_apSSID(String & result)   {result = String(apSSID);}
void WifiConnect::get_apPsk(String & result)    {result = String(apPass);}
void WifiConnect::get_mod(adsWifiMod & result)  {result = _MOD;}

void WifiConnect::set_mod(adsWifiMod result)    {_MOD = result;}

void WifiConnect::setup(){
  if (_isSetup) return;
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();  
  _isSetup = true;
}

boolean WifiConnect::isConnected(){
  return (WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED);
}
IPAddress WifiConnect::localIP() {
  IPAddress localIP;
  localIP = WiFi.localIP();
  if (localIP[0] != 0) {
    return localIP;
  }

  return INADDR_NONE;
}
