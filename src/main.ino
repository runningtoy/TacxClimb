#include <Arduino.h>
/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

// #define MYDEBUG
#define DIRECTFANCONTROL

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <PubSubClient.h>
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40]="192.168.178.100";
char mqtt_port[6] = "1883";
char mqtt_topic[40] = "TACX2MQTT";
//#define RESET_BTN_PIN 2

#include "BLEDevice.h"
//#include "BLEScan.h"


// The remote service we wish to connect to.
//static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID serviceUUID("6E40FEC1-B5A3-F393-E0A9-E50E24DCCA9E");
// The characteristic of the remote service we are interested in.
// static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static BLEUUID    FCE_RX_UUID("6E40FEC2-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID    FCE_TX_UUID("6E40FEC3-B5A3-F393-E0A9-E50E24DCCA9E");


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* FCE_TX;
static BLERemoteCharacteristic* FCE_RX;
static BLEAdvertisedDevice* myDevice;



unsigned long SendRequestPage51Delay = 2960; //Sample rate for Page 51 requests 4 seconds ?
unsigned long SendRequestPage51Event = millis(); //Millis of last Page 51 request event

unsigned long SendRequestPage25Delay = 1850; //Sample rate for Page 51 requests 4 seconds ?
unsigned long SendRequestPage25Event = millis(); //Millis of last Page 51 request event

//Define the Request Page 51 Command to send
uint8_t Page51Bytes[13] = {
    0xA4, //Sync
    0x09, //Length
    0x4F, //Acknowledge message type
    0x05, //Channel 
          //Data
    0x46, //Common Page 70
    0xFF,
    0xFF,
    0xFF, //Descriptor byte 1 (0xFF for no value)
    0xFF, //Descriptor byte 2 (0xFF for no value)
    0x80, //Requested transmission response
    0x33, //Requested page number 51 
    0x01, //Command type (0x01 for request data page, 0x02 for request ANT-FS session)
    0x47}; //Checksum;




  //Define the Request Page 51 Command to send
uint8_t Page25Bytes[13] = {
    0xA4, //Sync
    0x09, //Length
    0x4F, //Acknowledge message type
    0x05, //Channel 
          //Data
    0x46, //Common Page 70
    0xFF,
    0xFF,
    0xFF, //Descriptor byte 1 (0xFF for no value)
    0xFF, //Descriptor byte 2 (0xFF for no value)
    0x80, //Requested transmission response
    0x19, //Requested page number 25 
    0x01, //Command type (0x01 for request data page, 0x02 for request ANT-FS session)
    0x47}; //Checksum;




// RawgradeValue varies between 0 (-200% grade) and 40000 (+200% grade)
// SIMCLINE is mechanically working between -10% and +20% --> 19000 and 22000
// correction for measuring plane difference and midth wheel axis position (2 cm offset is an MEASUREOFFSET of 200)
#define MEASUREOFFSET 100 
// Raw Grade Value Minimally (Mechanically: the lowest position of wheel axis)  19000 is equiv. of -10% road grade
#define RGVMIN 19000 
// Raw Grade Value Maximally (Mechanically: the highest position of wheel axis) 22000 is equiv. of +20% road grade
#define RGVMAX 22000 
// Besides what is mechanically possible there are also limits in what is physically pleasant
// The following Min and Max values should be within the limits of the mechanically feasible values of above !!!
// Minimally Allowed Raw Grade Value that should not be exceeded: -5%!
#define ARGVMIN 19500
// Maximally Allowed Raw Grade Value that should not be exceeded: 15%!
#define ARGVMAX 21500
// set value for a flat road = 0% grade 
// 1000 is a 10% road grade --> added to the minimal position is equiv. of 0% road grade
// result needs to be corrected for the measure offset
long RawgradeValue = (RGVMIN + 1000) - MEASUREOFFSET; 
float gradePercentValue = 0;

// TACX trainer calculated & measured basic cycling data
long PowerValue = 0;
uint8_t InstantaneousCadence = 0;
long SpeedValue = 0;

void SendRequestPage51()
{
#ifdef MYDEBUG
  Serial.printf("%d  Sending Request for data page 51\n ", SendRequestPage51Event);
#endif
  // Page51Bytes are globally defined
  // uint16_t write(const void* data, uint16_t len);
  FCE_TX->writeValue(Page51Bytes, sizeof(Page51Bytes),false);
}

void SendRequestPage25()
{
#ifdef MYDEBUG
  Serial.printf("%d  Sending Request for data page 51\n ", SendRequestPage51Event);
#endif
  // Page51Bytes are globally defined
  // uint16_t write(const void* data, uint16_t len);
  FCE_TX->writeValue(Page25Bytes, sizeof(Page25Bytes),false);
}

void sendRequest()
{
  long tmp = millis() - SendRequestPage51Delay;
  if (tmp >= SendRequestPage51Event)
  {
    SendRequestPage51Event = millis();
    SendRequestPage51();
    Serial.println("request page 51\n");
  }

  tmp = millis() - SendRequestPage25Delay;
  if (tmp >= SendRequestPage25Event)
  {
    SendRequestPage25Event = millis();
    SendRequestPage25();
    Serial.println("request page 25\n");
  }
}


/////////////////////////////////////////////////////
#ifdef DIRECTFANCONTROL
void controlFAN(){
  if(PowerValue<10){
    mqttClient.publish("cmnd/ZwiftFan/POWER1","OFF");
    mqttClient.publish("cmnd/ZwiftFan/POWER2","OFF");
    mqttClient.publish("cmnd/ZwiftFan/POWER3","OFF");
    mqttClient.publish("cmnd/ZwiftFan/POWER4","OFF");
    return;
  }
  if(PowerValue<150){
    mqttClient.publish("cmnd/ZwiftFan/POWER4","ON");
    return;
  }
  if(PowerValue<300){
    mqttClient.publish("cmnd/ZwiftFan/POWER3","ON");
    return;
  }
  if(PowerValue<500){
    mqttClient.publish("cmnd/ZwiftFan/POWER2","ON");
    return;
  }
}
#endif
/////////////////////////////////////////////////////

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  uint8_t buffer[20 + 1];
  memset(buffer, 0, sizeof(buffer));
#ifdef MYDEBUG
  //Serial.printf("Dump of FE-C Data packets [length: %02d]  ", length);
#endif
  for (int i = 0; i < length; i++)
  {
    if (i <= sizeof(buffer))
    {
      buffer[i] = *pData++;
#ifdef MYDEBUG
      //Serial.printf("%02X ", buffer[i], HEX);
#endif
    }
  }
  uint8_t PageValue = buffer[4]; // Get Page number from packet
#ifdef MYDEBUG
  Serial.print("Got page ID:");
  Serial.println(PageValue);
#endif

  switch (PageValue)
  {
  case 0x47:
    ////////////////////////////////////////////////////////////
    //////////////////// Handle PAGE 71 ////////////////////////
    ////////////// Requested PAGE 51 for grade info ////////////
    ////////////////////////////////////////////////////////////
    if (buffer[5] == 0x33) // check for requested page 51
    {
      uint8_t lsb_gradeValue = buffer[9];
      uint8_t msb_gradeValue = buffer[10];
      RawgradeValue = lsb_gradeValue + msb_gradeValue * 256;
      gradePercentValue = float((RawgradeValue - 20000)) / 100;
      // in steps of 0.01% and with an offset of -200%
      // gradeValue     gradePercentValue
      //     0                 -200%
      //  19000                 -10%
      //  20000                   0%
      //  22000                 +20%
      //  40000                +200%
      // -------------------------------------
      // Take into account the measuring offset
      RawgradeValue = RawgradeValue - MEASUREOFFSET;
      // Test for Maximally en Minimally Allowed Raw Grade Values ----------------------------------------
      if (RawgradeValue < ARGVMIN)
      {
        RawgradeValue = ARGVMIN;
      } // Do not allow lower values than ARGVMIN !!
      if (RawgradeValue > ARGVMAX)
      {
        RawgradeValue = ARGVMAX;
      } // Do not allow values to exceed ARGVMAX !!
        // End test --> continue ---------------------------------------------------------------------------
// #ifdef DEBUGSIM
      Serial.printf("--Page 51 received - RawgradeValue: %05d  ", RawgradeValue);
      Serial.printf("Grade percentage: %02.1f %%", gradePercentValue);
      Serial.println();


    if(mqttClient.connected()){
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%s/Gradient", mqtt_topic);
      mqttClient.publish(buffer,String(gradePercentValue).c_str());
    }

// #endif
    }
    break;
  case 0x19:
  {
    /////////////////////////////////////////////////
    /////////// Handle PAGE 25 Trainer/Bike Data ////
    /////////////////////////////////////////////////
    uint8_t UpdateEventCount = buffer[5];
    InstantaneousCadence = buffer[6];
    uint8_t lsb_InstantaneousPower = buffer[9];
    // POWER is stored in 1.5 byte !!!
    uint8_t msb_InstantaneousPower = (buffer[10] & 0x0F); // bits 0:3 --> MSNibble only!!!
    PowerValue = lsb_InstantaneousPower + msb_InstantaneousPower * 256;
    if(mqttClient.connected()){
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%S/Power", mqtt_topic);
      mqttClient.publish(buffer,String(PowerValue).c_str());
      snprintf(buffer, sizeof(buffer), "%S/Cadence", mqtt_topic);
      mqttClient.publish(buffer,String(InstantaneousCadence).c_str());
    }
// #ifdef DEBUGSIM
    Serial.printf("Event count: %03d  ", UpdateEventCount);
    Serial.printf(" - Cadence: %03d  ", InstantaneousCadence);
    Serial.printf(" - Power in Watts: %04d  ", PowerValue);
    Serial.println();
// #endif
#ifdef DIRECTFANCONTROL
  controlFAN();
#endif
  }
  break;
  case 0x10:
  {
    //////////////////////////////////////////////
    //////////// Handle PAGE 16 General FE Data //
    //////////////////////////////////////////////
    uint8_t ElapsedTime = buffer[6];       // units of 0.25 seconds
    uint8_t DistanceTravelled = buffer[7]; // in meters 256 m rollover
    uint8_t lsb_SpeedValue = buffer[8];
    uint8_t msb_SpeedValue = buffer[9];
    SpeedValue = ((lsb_SpeedValue + msb_SpeedValue * 256) / 1000) * 3.6; // in units of 0,001 m/s naar km/h

    if(mqttClient.connected()){
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%S/DistanceTravelled", mqtt_topic);
      mqttClient.publish(buffer,String(DistanceTravelled).c_str());
      snprintf(buffer, sizeof(buffer), "%S/SpeedValue", mqtt_topic);
      mqttClient.publish(buffer,String(SpeedValue).c_str());
    }

// #ifdef DEBUGSIM
    Serial.printf("Elapsed time: %05d s  ", ElapsedTime);
    Serial.printf(" - Distance travelled: %05d m ", DistanceTravelled);
    Serial.printf(" - Speed: %02d km/h", SpeedValue);
    Serial.println();
// #endif
  }
  break;
  case 0x80:
  {
    // Manufacturer Identification Page
  }
  default:
  {
#ifdef DEBUGSIM
    Serial.printf("Page: %2d ", PageValue);
    Serial.println(" Received");
#endif
    return;
  }
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    // pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");




#ifdef MYDEBUG
    std::map<std::__cxx11::basic_string<char>, BLERemoteCharacteristic *> *mapOfWordCount;
    mapOfWordCount = pRemoteService->getCharacteristics();
    std::map<std::__cxx11::basic_string<char>, BLERemoteCharacteristic *>::iterator it = mapOfWordCount->begin();
    Serial.println(mapOfWordCount->size());
    while (it != mapOfWordCount->end())
    {
      std::string word = it->first;
      Serial.print(word.c_str());
      BLERemoteCharacteristic *chara = it->second;
      Serial.print("UUID=");
      Serial.println(chara->toString().c_str());
      it++;
    }
#endif





    // Obtain a reference to the characteristic in the service of the remote BLE server.
    FCE_RX = pRemoteService->getCharacteristic(FCE_RX_UUID);
    if (FCE_RX == nullptr) {
      Serial.print("Failed to find our characteristic FCE_RX_UUID: ");
      Serial.println(FCE_RX_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our FCE_RX_UUID characteristic ");

    // Read the value of the characteristic.
    if(FCE_RX->canRead()) {
      std::string value = FCE_RX->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }


    if(FCE_RX->canNotify())
      FCE_RX->registerForNotify(notifyCallback);

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    FCE_TX = pRemoteService->getCharacteristic(FCE_TX_UUID);
    if (FCE_TX == nullptr) {
      Serial.print("Failed to find our characteristic FCE_TX_UUID: ");
      Serial.println(FCE_TX_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
   
    // Read the value of the characteristic.
    if(!FCE_TX->canWrite()) {
      Serial.print("Can not write our characteristic FCE_TX_UUID: ");
      Serial.println(FCE_TX_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

     Serial.println(" - Found our FCE_TX_UUID characteristic");

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks






///////////////////
///////////////////



//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupWiFi() {
  // put your setup code here, to run once:
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
          
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 40);

  

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  // wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);

  //reset settings - for testing
  //wifiManager.resetSettings();

#ifdef RESET_BTN_PIN
  //reset settings button press - for testing
  if (digitalRead(RESET_BTN_PIN) == LOW)
  {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(RESET_BTN_PIN) == LOW)
    {
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if (digitalRead(RESET_BTN_PIN) == LOW)
      {
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wifiManager.resetSettings();
        ESP.restart();
      }
    }
  }
#endif
  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Tacx2MQTT"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  Serial.println("The values in the file are: ");
  Serial.println("\tmqtt_server : " + String(mqtt_server));
  Serial.println("\tmqtt_port : " + String(mqtt_port));
  Serial.println("\tmqtt_topic : " + String(mqtt_topic));

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Serial.println("saving config");
#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;
    

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  }

void setupMQTT() {
  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
}

bool reconnect()
{
  if (WiFi.isConnected())
  {
    Serial.println("Connecting to MQTT Broker...");
    while (!mqttClient.connected())
    {
      Serial.println("Reconnecting to MQTT Broker..");
      if (mqttClient.connect("Tacx2Mqtt"))
      {
        Serial.println("Connected.");
      }
    }
  }
  return mqttClient.connected();
}

///////////////////
///////////////////






void setup() {
  Serial.begin(115200);
#ifdef RESET_BTN_PIN
  pinMode(RESET_BTN_PIN, INPUT);
#endif
  //SetupWifi
  Serial.println("Starting Wifi...");
  setupWiFi();
  setupMQTT();
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30, false);
} // End of setup.

long lastReconnectAttempt = 0;

// This is the Arduino main loop function.
void loop() {
    if (!mqttClient.connected()){
      long now = millis();
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        // Attempt to reconnect
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    }else{
      mqttClient.loop(); 
    }

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    sendRequest();      //send request every xx ms

  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  

} // End of loop