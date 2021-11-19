# 1 "C:\\Users\\GEORGM~1\\AppData\\Local\\Temp\\tmpv9zro9iy"
#include <Arduino.h>
# 1 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
#include <Arduino.h>
#include "myDefines.h"
#include <M5Core2.h>
#include <Ticker.h>


typedef enum M5BUTTON{
  BTN_A,
  BTN_B,
  BTN_C,
  NONE
};


M5BUTTON menue_Btn=M5BUTTON::NONE;
# 31 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
#include <FS.h>
#include <WiFiManager.h>

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include <ArduinoJson.h>

#include <Lifter.h>



bool shouldSaveConfig = false;





#include <PubSubClient.h>
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
long lastReconnectAttempt = 0;


char mqtt_server[40]="192.168.178.100";
char mqtt_port[6] = "1883";
char mqtt_topic[40] = "TACX2MQTT";
# 68 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
#include <NimBLEDevice.h>




static NimBLEUUID serviceUUID("6E40FEC1-B5A3-F393-E0A9-E50E24DCCA9E");



static NimBLEUUID FCE_RX_UUID("6E40FEC2-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID FCE_TX_UUID("6E40FEC3-B5A3-F393-E0A9-E50E24DCCA9E");


static boolean doConnect = false;
static boolean ble_connected = false;
static boolean doScan = false;
static NimBLERemoteCharacteristic* FCE_TX;
static NimBLERemoteCharacteristic* FCE_RX;
static NimBLEAdvertisedDevice* myDevice;



unsigned long SendRequestPage51Delay = 2960;
unsigned long SendRequestPage51Event = millis();

unsigned long SendRequestPage25Delay = 1850;
unsigned long SendRequestPage25Event = millis();


uint8_t Page51Bytes[13] = {
    0xA4,
    0x09,
    0x4F,
    0x05,

    0x46,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x80,
    0x33,
    0x01,
    0x47};





uint8_t Page25Bytes[13] = {
    0xA4,
    0x09,
    0x4F,
    0x05,

    0x46,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x80,
    0x19,
    0x01,
    0x47};
# 143 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
#define MEASUREOFFSET 100

#define RGVMIN 19000

#define RGVMAX 22000



int aRGVmin = 19500;

int aRGVmax = 21500;



long RawgradeValue = (RGVMIN + 1000) - MEASUREOFFSET;
float gradePercentValue = 0;

int GradeChangeFactor = 100;



long PowerValue = 0;
uint8_t InstantaneousCadence = 0;
long SpeedValue = 0;







Lifter lift;

int16_t TargetPosition = map(RawgradeValue, RGVMIN, RGVMAX, MAXPOSITION, MINPOSITION);
bool IsBasicMotorFunctions = false;
Ticker lifterTicker;



#include "Screen.h"
void SendRequestPage51();
void SendRequestPage25();
void sendRequest();
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
bool connectToServer();
void saveConfigCallback ();
void setupWiFi();
void setupMQTT();
bool reconnect();
void setup();
void loop();
bool ControlUpDownMovement(void);
void SetNeutralValues(void);
void fct_lifterTicker();
void ButtonMenueResetTimer();
void checkButtonPress();
void readSettings();
void saveSettings();
#line 186 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
void SendRequestPage51()
{
#ifdef MYDEBUG
  Serial.printf("%d  Sending Request for data page 51\n ", SendRequestPage51Event);
#endif


  FCE_TX->writeValue(Page51Bytes, sizeof(Page51Bytes),false);
}

void SendRequestPage25()
{
#ifdef MYDEBUG
  Serial.printf("%d  Sending Request for data page 51\n ", SendRequestPage51Event);
#endif


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
  uint8_t PageValue = buffer[4];
#ifdef MYDEBUG
  Serial.print("Got page ID:");
  Serial.println(PageValue);
#endif

  switch (PageValue)
  {
  case 0x47:




    if (buffer[5] == 0x33)
    {
      uint8_t lsb_gradeValue = buffer[9];
      uint8_t msb_gradeValue = buffer[10];
      RawgradeValue = lsb_gradeValue + msb_gradeValue * 256;
      gradePercentValue = float((RawgradeValue - 20000)) / 100;
# 270 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
      long ReferenceValue = (RGVMIN + 1000);
      RawgradeValue = ReferenceValue + long((RawgradeValue - ReferenceValue) * GradeChangeFactor / 100);
      gradePercentValue = float((RawgradeValue - 20000)) / 100;

      RawgradeValue = RawgradeValue - MEASUREOFFSET;

      if (RawgradeValue < aRGVmin)
      {
        RawgradeValue = aRGVmin;
      }
      if (RawgradeValue > aRGVmax)
      {
        RawgradeValue = aRGVmax;
      }

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



    uint8_t UpdateEventCount = buffer[5];
    InstantaneousCadence = buffer[6];
    uint8_t lsb_InstantaneousPower = buffer[9];

    uint8_t msb_InstantaneousPower = (buffer[10] & 0x0F);
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
  }
  break;
  case 0x10:
  {



    uint8_t ElapsedTime = buffer[6];
    uint8_t DistanceTravelled = buffer[7];
    uint8_t lsb_SpeedValue = buffer[8];
    uint8_t msb_SpeedValue = buffer[9];
    SpeedValue = ((lsb_SpeedValue + msb_SpeedValue * 256) / 1000) * 3.6;

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

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    ble_connected = false;
    Serial.println("onDisconnect");
  }
};





bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());


    pClient->connect(myDevice);
    Serial.println(" - Connected to server");



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






    FCE_RX = pRemoteService->getCharacteristic(FCE_RX_UUID);
    if (FCE_RX == nullptr) {
      Serial.print("Failed to find our characteristic FCE_RX_UUID: ");
      Serial.println(FCE_RX_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our FCE_RX_UUID characteristic ");


    if(FCE_RX->canRead()) {
      std::string value = FCE_RX->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }


    if(FCE_RX->canNotify())
      FCE_RX->registerForNotify(notifyCallback);


    FCE_TX = pRemoteService->getCharacteristic(FCE_TX_UUID);
    if (FCE_TX == nullptr) {
      Serial.print("Failed to find our characteristic FCE_TX_UUID: ");
      Serial.println(FCE_TX_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }


    if(!FCE_TX->canWrite()) {
      Serial.print("Can not write our characteristic FCE_TX_UUID: ");
      Serial.println(FCE_TX_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

     Serial.println(" - Found our FCE_TX_UUID characteristic");

    ble_connected = true;
    return true;
}





class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{



  void onResult(NimBLEAdvertisedDevice* advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice->toString().c_str());


    if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID))
    {

      BLEDevice::getScan()->stop();

      myDevice=advertisedDevice;
      doConnect = true;
      doScan = true;
    }
  }
};






void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupWiFi() {

  Serial.println();





  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {

      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();

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
    ShowOnOledLarge("", "prepare Filesystem","", 500,RED);
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





  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 40);





  WiFiManager wifiManager;


  wifiManager.setSaveConfigCallback(saveConfigCallback);





  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);




#ifdef RESET_BTN_PIN

  if (digitalRead(RESET_BTN_PIN) == LOW)
  {

    delay(50);
    if (digitalRead(RESET_BTN_PIN) == LOW)
    {
      Serial.println("Button Pressed");

      delay(3000);
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
# 636 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
  if (!wifiManager.autoConnect("Tacx2MQTT"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);

    ESP.restart();
    delay(5000);
  }


  Serial.println("connected...yeey :)");


  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  Serial.println("The values in the file are: ");
  Serial.println("\tmqtt_server : " + String(mqtt_server));
  Serial.println("\tmqtt_port : " + String(mqtt_port));
  Serial.println("\tmqtt_topic : " + String(mqtt_topic));


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
# 723 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
void setup() {
#ifdef USB_POWERED

  M5.begin(true,true,true,true,kMBusModeOutput);



#else
  M5.begin(true,true,true,true,kMBusModeInput);
#endif

  Serial.begin(115200);
#ifdef RESET_BTN_PIN
  pinMode(RESET_BTN_PIN, INPUT);
#endif
  ShowOnOledLarge("TacxClimb", FIRMWARE_VERSION,"by Runningtoy", 500,RED);

  readSettings();


  lift.Init(actuatorOutPin1, actuatorOutPin2, MINPOSITION, MAXPOSITION, BANDWIDTH);
# 762 "C:/Users/Georg Mihatsch/Documents/PlatformIO/Projects/TacxClimb/src/main.ino"
  lifterTicker.attach_ms(100, fct_lifterTicker);



  Serial.println("Starting Wifi...");

  ShowOnOledLarge("", "Connecting Wifi", "", 500,RED);
  setupWiFi();

  ShowOnOledLarge("", "Connecting MQTT", "", 500,RED);

  setupMQTT();
  ShowOnOledLarge("", "connecting BLE", "", 500,RED);


  Serial.println("Starting Arduino BLE Client application...");
  NimBLEDevice::init("");





  NimBLEScan* pBLEScan = NimBLEDevice::getScan();
  Serial.println("[1] Starting Arduino BLE Client application...");
  pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  Serial.println("[2] Starting Arduino BLE Client application...");
  pBLEScan->setInterval(1349);
  Serial.println("[3] Starting Arduino BLE Client application...");
  pBLEScan->setWindow(449);
  Serial.println("[4] Starting Arduino BLE Client application...");
  pBLEScan->setActiveScan(true);
  Serial.println("[5] Starting Arduino BLE Client application...");
  pBLEScan->start(30, false);
  Serial.println("[6] Starting Arduino BLE Client application...");

  updateDisplay=true;
}






void loop() {

    if (!mqttClient.connected()){
      long now = millis();
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;

        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
        updateDisplay=true;
      }
    }else{
      mqttClient.loop();
    }




  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");

      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%s", myDevice->toString());
      ShowOnOledLarge("", "Connected to ", buffer, 500,GREEN);
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    updateDisplay=true;
    doConnect = false;
  }



  if (ble_connected) {
    sendRequest();
  }else if(doScan){
    NimBLEDevice::getScan()->start(0);
  }

  M5.update();
  checkButtonPress();

  ShowValuesOnOled();

}






bool ControlUpDownMovement(void) {




  RawgradeValue = constrain(RawgradeValue, RGVMIN, RGVMAX);
  TargetPosition = map(RawgradeValue, RGVMIN, RGVMAX, MAXPOSITION, MINPOSITION);
#ifdef MYDEBUG
  Serial.println(); Serial.printf("RawgradeValue: %05d ", RawgradeValue, DEC); Serial.printf(" TargetPosition: %03d", TargetPosition, DEC);
#endif
  lift.SetTargetPosition(TargetPosition);
  lift.gotoTargetPosition();
  return (lift.GetOffsetPosition() == 0);
}


void SetNeutralValues(void) {
  RawgradeValue = (RGVMIN + 1000) - MEASUREOFFSET;
  TargetPosition = map(RawgradeValue, RGVMIN, RGVMAX, MAXPOSITION, MINPOSITION);
  lift.SetTargetPosition(TargetPosition);
}


void fct_lifterTicker(){
  if (IsBasicMotorFunctions) {
    while (ControlUpDownMovement()) {
    }
  }
}

unsigned long buttonMenueReset=ULONG_MAX;
void ButtonMenueResetTimer(){
  buttonMenueReset=millis()+(1000*15);
  updateDisplay=true;
}

void checkButtonPress()
{
  if (menue_Btn == M5BUTTON::NONE)
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
  if(buttonMenueReset<millis()){
      menue_Btn = M5BUTTON::NONE;
      Serial.println("Reset buttonMenueReset ");
      saveSettings();
      buttonMenueReset=ULONG_MAX;
      updateDisplay=true;
  }
}




void readSettings(){
   if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/Taxc.json")) {

      Serial.println("reading config file");
      File configFile = SPIFFS.open("/Taxc.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();

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
          aRGVmin=json["aRGVmin"];
          aRGVmax=json["aRGVmax"];
          GradeChangeFactor=json["GradeChangeFactor"];
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}


void saveSettings(){

  if (SPIFFS.begin())
  {
    Serial.println("saving config");
#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["aRGVmin"] = aRGVmin;
    json["aRGVmax"] = aRGVmax;
    json["GradeChangeFactor"] = GradeChangeFactor;
    File configFile = SPIFFS.open("/Taxc.json", "w");
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

  }
}