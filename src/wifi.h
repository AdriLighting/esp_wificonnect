#ifndef WIFI_H
#define WIFI_H
  #include <Arduino.h>
  #include <ESP8266WiFi.h>

  #ifdef ARDUINO_ARCH_ESP32
    #define ADS_CONNECTED (WiFi.status() == WL_CONNECTED || ETH.localIP()[0] != 0)
  #else
    #define ADS_CONNECTED (WiFi.status() == WL_CONNECTED)
  #endif
  #ifdef ESP8266
    #define JSON_BUFFER_SIZE 12000
  #else
    #define JSON_BUFFER_SIZE 20480
  #endif

  #ifndef DEFAULT_AP_PASS
    #define DEFAULT_AP_PASS "adsap1234"
  #endif
  #ifndef DEFAULT_OTA_PASS
    #define DEFAULT_OTA_PASS "adsota1234"
  #endif


  #ifndef CLIENT_SSID
    #define CLIENT_SSID "routeur-ssid"
  #endif

  #ifndef CLIENT_PASS
    #define CLIENT_PASS "routeur-pswd"
  #endif

  #ifndef ADS_NAME
    #define ADS_NAME "esp_wificonnect"
  #endif

  typedef enum adsWifiMod
  {
    ADSWM_STA,
    ADSWM_STA_AP,
    ADSWM_AP
  } adsWifiMod_t;


  class WifiConnect {
    typedef std::function<void()> callback_function_t;

    typedef struct {
      uint32_t  lastReconnectAttempt  = 0;
      uint8_t   reconnectAttempt      = 0;
      boolean   active                = false;
      boolean   connected             = false;
      boolean   reconnect             = false;
      boolean   wasConnected          = false;
      byte      serverInitialized     = 0;
    } STATU;

    char * apPass  ; 
    char * otaPass ; 
    char * clientSSID ; 
    char * clientPass ; 
    char * cmDNS ; 
    char * apSSID ;   
    char * serverDescription;     // Name of module
    String escapedMac;
    
    IPAddress staticIP;           // static IP of ESP
    IPAddress staticGateway ;     // gateway (router) IP
    IPAddress staticSubnet; 

    byte apChannel  = 1;          // 2.4GHz WiFi AP channel (1-13)
    byte apHide     = 0;          // hidden AP SSID
    boolean _forceReconnect = false;

    adsWifiMod _MOD = ADSWM_STA_AP;

    STATU _STA;
    STATU _AP;

    callback_function_t _APFUNC_INIT_SERVER  = nullptr;
    callback_function_t _STAFUNC_INIT_SERVER = nullptr;
     
  public:
    WifiConnect();
    ~WifiConnect();

    boolean _isSetup = false;

    boolean needSTA();
    boolean WIFIsetupIsReady();
    boolean STAisReady();

    boolean isConnected();
    IPAddress localIP();

    void setup();
    void handleConnection();

    void initSTA();
    void initAP();
    void setFunc_STAinitServer(callback_function_t f);
    void setFunc_APinitServer(callback_function_t f);

    void loop_sta(const unsigned long & now );


    void get_staSSID(String & result);  
    void get_staPsk(String & result);  
    void get_apSSID(String & result);  
    void get_apPsk(String & result);  

  };
#endif // WIFI_H