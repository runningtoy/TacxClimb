#include <Arduino.h>
#include "myDefines.h"
#include <M5Core2.h>
#include <Ticker.h>
#ifdef OTA
#include "myOTA.h"
#endif

typedef enum M5BUTTON
{ // <-- the use of typedef is optional
  BTN_A,
  BTN_B,
  BTN_C,
  BTN_NONE
};

M5BUTTON menue_Btn = M5BUTTON::BTN_NONE; // <-- the actual instance

/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

/****************************************
 *  WIFIMANAGER Settings
 * ***************************************/

#include <FS.h>          //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#ifdef ESP32
#include <SPIFFS.h>
#endif

#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

#include <Lifter.h>

//flag for saving data
bool shouldSaveConfig = false;

/****************************************
 *  MQTT Settings
 * ***************************************/
#include <PubSubClient.h>
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
long lastReconnectAttempt = 0;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "192.168.178.100";
char mqtt_port[6] = "1883";
char mqtt_topic[40] = "TACX2MQTT";
//#define RESET_BTN_PIN 2

/****************************************
 *  BLE Settings
 * ***************************************/

// #include "BLEDevice.h"
//#include "BLEScan.h"
#include <NimBLEDevice.h>

// The remote service we wish to connect to.
//static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static NimBLEUUID serviceUUID("6E40FEC1-B5A3-F393-E0A9-E50E24DCCA9E");
// The characteristic of the remote service we are interested in.
// static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static NimBLEUUID FCE_RX_UUID("6E40FEC2-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID FCE_TX_UUID("6E40FEC3-B5A3-F393-E0A9-E50E24DCCA9E");

static boolean doConnect = false;
static boolean ble_connected = false;
static boolean doScan = false;
static NimBLERemoteCharacteristic *FCE_TX;
static NimBLERemoteCharacteristic *FCE_RX;
static NimBLEAdvertisedDevice *myDevice;

unsigned long SendRequestPage51Delay = 2960;     //Sample rate for Page 51 requests 4 seconds ?
unsigned long SendRequestPage51Event = millis(); //Millis of last Page 51 request event

unsigned long SendRequestPage25Delay = 1850;     //Sample rate for Page 51 requests 4 seconds ?
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
    0xFF,  //Descriptor byte 1 (0xFF for no value)
    0xFF,  //Descriptor byte 2 (0xFF for no value)
    0x80,  //Requested transmission response
    0x33,  //Requested page number 51
    0x01,  //Command type (0x01 for request data page, 0x02 for request ANT-FS session)
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
    0xFF,  //Descriptor byte 1 (0xFF for no value)
    0xFF,  //Descriptor byte 2 (0xFF for no value)
    0x80,  //Requested transmission response
    0x19,  //Requested page number 25
    0x01,  //Command type (0x01 for request data page, 0x02 for request ANT-FS session)
    0x47}; //Checksum;

/****************************************
 *  TACX Settings
 * ***************************************/

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
int aRGVmin = 19500;
// Maximally Allowed Raw Grade Value that should not be exceeded: 15%!
int aRGVmax = 21500;
// set value for a flat road = 0% grade
// 1000 is a 10% road grade --> added to the minimal position is equiv. of 0% road grade
// result needs to be corrected for the measure offset
long RawgradeValue = (RGVMIN + 1000) - MEASUREOFFSET;
float gradePercentValue = 0;

int GradeChangeFactor = 100; // 100% means no effect, 50% means only halved up/down steps --> Road Grade Change Factor
// Grade of a road is defined as a measure of the road's steepness as it rises and falls along its route

// TACX trainer calculated & measured basic cycling data
long PowerValue = 0;
uint8_t InstantaneousCadence = 0;
long SpeedValue = 0;

/****************************************
 *  Lifter Settings
 * ***************************************/
// Decalaration of Lifter Class for control of the low level up/down movement
Lifter lift;
// Global variables for LIFTER position control --> RawGradeValue has been defined/set previously!!
int16_t TargetPosition = map(RawgradeValue, RGVMIN, RGVMAX, MAXPOSITION, MINPOSITION);
bool IsBasicMotorFunctions = false; // Mechanical motor functions
Ticker lifterTicker;
char bufferLiftPositon[128];

//------------ include Screen ---------------
#include "Screen.h"

void SendRequestPage51()
{
#ifdef MYDEBUG
  Serial.printf("%d  Sending Request for data page 51\n ", SendRequestPage51Event);
#endif
  // Page51Bytes are globally defined
  // uint16_t write(const void* data, uint16_t len);
  FCE_TX->writeValue(Page51Bytes, sizeof(Page51Bytes), false);
}

void SendRequestPage25()
{
#ifdef MYDEBUG
  Serial.printf("%d  Sending Request for data page 51\n ", SendRequestPage51Event);
#endif
  // Page51Bytes are globally defined
  // uint16_t write(const void* data, uint16_t len);
  FCE_TX->writeValue(Page25Bytes, sizeof(Page25Bytes), false);
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

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  uint8_t buffer[20 + 1];
  memset(buffer, 0, sizeof(buffer));
#ifdef MYDEBUG
  Serial.printf("Dump of FE-C Data packets [length: %02d]  ", length);
#endif
  for (int i = 0; i < length; i++)
  {
    if (i <= sizeof(buffer))
    {
      buffer[i] = *pData++;
#ifdef MYDEBUG
      Serial.printf("%02X ", buffer[i], HEX);
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
      long ReferenceValue = (RGVMIN + 1000); // Reference is a flat road 0% inclination
      RawgradeValue = ReferenceValue + long((RawgradeValue - ReferenceValue) * GradeChangeFactor / 100);
      gradePercentValue = float((RawgradeValue - 20000)) / 100;

      RawgradeValue = RawgradeValue - MEASUREOFFSET;
      // Test for Maximally en Minimally Allowed Raw Grade Values ----------------------------------------
      if (RawgradeValue < aRGVmin)
      {
        RawgradeValue = aRGVmin;
      } // Do not allow lower values than aRGVmin  !!
      if (RawgradeValue > aRGVmax)
      {
        RawgradeValue = aRGVmax;
      } // Do not allow values to exceed aRGVmax !!
        // End test --> continue ---------------------------------------------------------------------------
#ifdef MYDEBUG
      Serial.printf("--Page 51 received - RawgradeValue: %05d  ", RawgradeValue);
      Serial.printf("Grade percentage: %02.1f %%", gradePercentValue);
      Serial.println();
#endif

      if (mqttClient.connected())
      {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s/Gradient", mqtt_topic);
        mqttClient.publish(buffer, String(gradePercentValue).c_str());
      }
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
    if (mqttClient.connected())
    {
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%s/Power", mqtt_topic);
      mqttClient.publish(buffer, String(PowerValue).c_str());
      snprintf(buffer, sizeof(buffer), "%s/Cadence", mqtt_topic);
      mqttClient.publish(buffer, String(InstantaneousCadence).c_str());
      snprintf(buffer, sizeof(buffer), "%s/UpdateEventCount", mqtt_topic);
      mqttClient.publish(buffer, String(UpdateEventCount).c_str());
    }
#ifdef MYDEBUG
    Serial.printf("Event count: %03d  ", UpdateEventCount);
    Serial.printf(" - Cadence: %03d  ", InstantaneousCadence);
    Serial.printf(" - Power in Watts: %04d  ", PowerValue);
    Serial.println();
#endif
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

    if (mqttClient.connected())
    {
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%s/DistanceTravelled", mqtt_topic);
      mqttClient.publish(buffer, String(DistanceTravelled).c_str());
      snprintf(buffer, sizeof(buffer), "%s/SpeedValue", mqtt_topic);
      mqttClient.publish(buffer, String(SpeedValue).c_str());
      snprintf(buffer, sizeof(buffer), "%s/ElapsedTime", mqtt_topic);
      mqttClient.publish(buffer, String(ElapsedTime).c_str());
    }

#ifdef MYDEBUG
    Serial.printf("Elapsed time: %05d s  ", ElapsedTime);
    Serial.printf(" - Distance travelled: %05d m ", DistanceTravelled);
    Serial.printf(" - Speed: %02d km/h", SpeedValue);
    Serial.println();
#endif
  }
  break;
  case 0x80:
  {
    // Manufacturer Identification Page
  }
  default:
  {
#ifdef MYDEBUG
    Serial.printf("Page: %2d ", PageValue);
    Serial.println(" Received");
#endif
    return;
  }
  }
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
  }

  void onDisconnect(BLEClient *pclient)
  {
    ble_connected = false;
    Serial.println("onDisconnect");
  }
};

/****************************************
 *  BLE Connect to Tacx
 * ***************************************/
bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  // pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
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
  if (FCE_RX == nullptr)
  {
    Serial.print("Failed to find our characteristic FCE_RX_UUID: ");
    Serial.println(FCE_RX_UUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our FCE_RX_UUID characteristic ");

  // Read the value of the characteristic.
  if (FCE_RX->canRead())
  {
    std::string value = FCE_RX->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (FCE_RX->canNotify())
    FCE_RX->registerForNotify(notifyCallback);

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  FCE_TX = pRemoteService->getCharacteristic(FCE_TX_UUID);
  if (FCE_TX == nullptr)
  {
    Serial.print("Failed to find our characteristic FCE_TX_UUID: ");
    Serial.println(FCE_TX_UUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  // Read the value of the characteristic.
  if (!FCE_TX->canWrite())
  {
    Serial.print("Can not write our characteristic FCE_TX_UUID: ");
    Serial.println(FCE_TX_UUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  Serial.println(" - Found our FCE_TX_UUID characteristic");

  ble_connected = true;
  return true;
}

/****************************************
 *  Scan for BLE servers and find the first one that advertises the service we are looking for.
 * ***************************************/
// class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */
  void onResult(NimBLEAdvertisedDevice *advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice->toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID))
    {

      BLEDevice::getScan()->stop();
      // myDevice = new BLEAdvertisedDevice(advertisedDevice);
      myDevice = advertisedDevice;
      doConnect = true;
      doScan = true;
    } // Found our server
  }   // onResult
};    // MyAdvertisedDeviceCallbacks

/****************************************
 *  WiFiManager Function
 * ***************************************/

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupWiFi()
{
  // put your setup code here, to run once:
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin())
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError)
        {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
#endif
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
        }
        else
        {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  }
  else
  {
    ShowOnOledLarge("", "prepare Filesystem", "", 500, RED, ICON::ICO_FILE);
    bool formatted = SPIFFS.format();
    if (formatted)
    {
      Serial.println("SPIFFS formatted successfully");
    }
    else
    {
      Serial.println("Error formatting");
    }
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

  M5.update();
  if (M5.BtnA.read())
  {
    ShowOnOledLarge("", "WifiSettings", "RESET", 5000, BLACK, ICON::ICO_DELETE);
    // wifiManager.resetSettings();
    ESP.restart();
  }
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
    JsonObject &json = jsonBuffer.createObject();
#endif
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
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
  WiFi.setHostname(FIRMWARE_NAME); //define hostname
  // ShowOnOledLarge(WiFi.localIP().toString(),wifiLogo_bits,wifiLogo_width,wifiLogo_height);
}

void setupMQTT()
{
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
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s/IP", mqtt_topic);
        mqttClient.publish(buffer, WiFi.localIP().toString().c_str());
        Serial.println("Connected.");
      }
    }
  }
  return mqttClient.connected();
}

/****************************************
 *  Setup everything
 * ***************************************/

void setup()
{
#ifdef USB_POWERED
  // M5.begin();
  M5.begin(true, true, true, true, kMBusModeOutput);
  // M5.begin(true,true,true,true,kMBusModeInput);
  //  kMBusModeOutput,powered by USB or Battery
  //  kMBusModeInput,powered by outside input
#else
  M5.begin(true, true, true, true, kMBusModeInput);
#endif

  Serial.begin(115200);

  ShowOnOledLarge(FIRMWARE_NAME, FIRMWARE_VERSION, "by Runningtoy", 500, RED);

  readSettings();

  //SetupWifi
  Serial.println("Starting Wifi...");
  // ShowOnOledLarge("Connecting Wifi",wifi_icon40x40,40,40);
  ShowOnOledLarge("", "Connecting Wifi", "", 500, RED, ICON::ICO_WIFI);
  setupWiFi();
  // ShowOnOledLarge("Connecting MQTT",mqtt_icon32x28,32,28);
  ShowOnOledLarge("", "Connecting MQTT", "", 500, RED, ICON::ICO_MQTT);
  setupMQTT();

  // Initialize Lifter Class data, variables, test and set to work !
  lift.Init(actuatorOutPin1, actuatorOutPin2, MINPOSITION, MAXPOSITION, BANDWIDTH);
  ShowOnOledLarge("", "Motortest", "...", 100, TFT_BLACK, ICON::ICO_LIFTER);
  if (!lift.TestBasicMotorFunctions())
  {
    ShowOnOledLarge("", "Motortest", "FAILED", 500, TFT_RED, ICON::ICO_FAIL);
    IsBasicMotorFunctions = false; // Not working properly
  }
  else
  {
    ShowOnOledLarge("", "Motortest", "Success", 500, TFT_GREEN, ICON::ICO_CHECK);
    // Is working properly
    IsBasicMotorFunctions = true;
    // Put Simcline in neutral: flat road position
    SetNeutralValues();             // set relevant flat road values
    while (ControlUpDownMovement()) // wait until flat road position is reached
    {
      delay(1);
    }
  }
  snprintf(bufferLiftPositon, sizeof(bufferLiftPositon), "%s/LiftPositon", mqtt_topic);

  ShowOnOledLarge("", "connecting BLE", "", 500, RED, ICON::ICO_BLE);
  // ShowOnOledLarge("Connecting BLE",bluetooth_icon16x16,16,16);

  Serial.println("Starting Arduino BLE Client application...");
  NimBLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  NimBLEScan *pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(BLE_SCAN_TIMEOUT, false);


#ifdef OTA
  ShowOnOledLarge("", "OTA", "", 500, TFT_GREEN, ICON::ICO_WIFI);
  OTASetup();
#endif

  lifterTicker.attach_ms(30, fct_lifterTicker);
  ShowOnOledLarge("", FIRMWARE_NAME, "Ready", 500, TFT_GREEN, ICON::ICO_CHECK);

  updateDisplay = true;
} // End of setup.

/****************************************
 *  Main Loop
 * ***************************************/
void loop()
{
  //check MQTT Connection and try to reconnect if not connected
  if (!mqttClient.connected())
  {
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
      updateDisplay = true;
    }
  }
  else
  {
    mqttClient.loop();
  }

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
      // ShowOnOledLarge("BLE connected",bluetooth_icon16x16,16,16,GREEN);
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%c", myDevice->toString());
      ShowOnOledLarge("", "Connected to ", buffer, 500, GREEN);
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    updateDisplay = true;
    doConnect = false;
  }

  // If we are ble_connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (ble_connected)
  {
    sendRequest(); //send request every xx ms
  }
  else if (doScan)
  {
    NimBLEDevice::getScan()->start(BLE_SCAN_TIMEOUT, false); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  M5.update(); //Read the press state of the key.
  checkButtonPress();

  ShowValuesOnOled();

  #ifdef OTA
    OTAloop();
  #endif

} // End of loop

bool ControlUpDownMovement(void)
{
  // Handle mechanical movement i.e. wheel position in accordance with Road Inclination
  // Map RawgradeValue ranging from 0 to 40.000 on the
  // TargetPosition (between MINPOSITION and MAXPOSITION) of the Lifter
  // Notice 22000 is equivalent to +20% incline and 19000 to -10% incline
  RawgradeValue = constrain(RawgradeValue, RGVMIN, RGVMAX); // Keep values within the safe range
  TargetPosition = map(RawgradeValue, RGVMIN, RGVMAX, MAXPOSITION, MINPOSITION);
// #ifdef MYDEBUG
  Serial.println();
  Serial.printf("RawgradeValue: %05d ", RawgradeValue, DEC);
  Serial.printf(" TargetPosition: %03d", TargetPosition, DEC);
  Serial.printf(" CurrentPosition: %03d", lift.GetPosition(), DEC);
  Serial.println();
// #endif
  lift.SetTargetPosition(TargetPosition);
  lift.gotoTargetPosition();
  return (lift.GetOffsetPosition() == 0); //return true if target/error is reached
}

void SetNeutralValues(void)
{
  RawgradeValue = (RGVMIN + 1000) - MEASUREOFFSET;
  TargetPosition = map(RawgradeValue, RGVMIN, RGVMAX, MAXPOSITION, MINPOSITION);
  lift.SetTargetPosition(TargetPosition);
}

void fct_lifterTicker()
{
  if (IsBasicMotorFunctions)
  {
    ControlUpDownMovement();
    // while (ControlUpDownMovement())
    // {
    // }
  }
  // #ifdef MYDEBUG
  Serial.print("LiftPositon:");
  Serial.println(lift.GetPosition());
  // #endif
  mqttClient.publish(bufferLiftPositon, String(lift.GetPosition()).c_str());
}

unsigned long buttonMenueReset = ULONG_MAX;
void ButtonMenueResetTimer()
{
  buttonMenueReset = millis() + (1000 * 15);
  updateDisplay = true;
}

void checkButtonPress()
{
  if (menue_Btn == M5BUTTON::BTN_NONE)
  {
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))
    {
      Serial.println('A');
      menue_Btn = M5BUTTON::BTN_A;
      ButtonMenueResetTimer();
    }
    else if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))
    {
      Serial.println('B');
      menue_Btn = M5BUTTON::BTN_B;
      ButtonMenueResetTimer();
    }
    else if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200))
    {
      Serial.println('C');
      menue_Btn = M5BUTTON::BTN_C;
      ButtonMenueResetTimer();
    }
  }
  else
  {
    if (menue_Btn == M5BUTTON::BTN_A)
    {
      if (M5.BtnA.wasReleased())
      {
        GradeChangeFactor++;
        ButtonMenueResetTimer();
      }
      if (M5.BtnC.wasReleased())
      {
        GradeChangeFactor--;
        ButtonMenueResetTimer();
      }
    }

    if (menue_Btn == M5BUTTON::BTN_B)
    {
      if (M5.BtnA.wasReleased())
      {
        aRGVmax++;
        ButtonMenueResetTimer();
      }
      if (M5.BtnC.wasReleased())
      {
        aRGVmax--;
        ButtonMenueResetTimer();
      }
    }

    if (menue_Btn == M5BUTTON::BTN_C)
    {
      if (M5.BtnA.wasReleased())
      {
        aRGVmin++;
        ButtonMenueResetTimer();
      }
      if (M5.BtnC.wasReleased())
      {
        aRGVmin--;
        ButtonMenueResetTimer();
      }
    }
  }
  if (buttonMenueReset < millis())
  {
    menue_Btn = M5BUTTON::BTN_NONE;
    Serial.println("Reset buttonMenueReset ");
    saveSettings();
    buttonMenueReset = ULONG_MAX;
    updateDisplay = true;
  }
}

void readSettings()
{
  if (SPIFFS.begin())
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/Taxc.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/Taxc.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError)
        {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
#endif
          Serial.println("\nparsed json");
          aRGVmin = json["aRGVmin"];
          aRGVmax = json["aRGVmax"];
          GradeChangeFactor = json["GradeChangeFactor"];
        }
        else
        {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
}

void saveSettings()
{
  //save the custom parameters to FS
  if (SPIFFS.begin())
  {
    Serial.println("saving config");
#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
#endif
    json["aRGVmin"] = aRGVmin;
    json["aRGVmax"] = aRGVmax;
    json["GradeChangeFactor"] = GradeChangeFactor;
    File configFile = SPIFFS.open("/Taxc.json", "w");
    if (!configFile)
    {
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
