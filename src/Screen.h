
#include <M5Core2.h>


//https://www.mischianti.org/2021/07/14/ssd1306-oled-display-draw-images-splash-and-animations-2/#Image_format

uint8_t bluetooth_icon16x16[] PROGMEM = {
  0b00000000, 0b00000000, //                 
  0b00000001, 0b10000000, //        ##       
  0b00000001, 0b11000000, //        ###      
  0b00000001, 0b01100000, //        # ##     
  0b00001001, 0b00110000, //     #  #  ##    
  0b00001101, 0b00110000, //     ## #  ##    
  0b00000111, 0b01100000, //      ### ##     
  0b00000011, 0b11000000, //       ####      
  0b00000001, 0b10000000, //        ##       
  0b00000011, 0b11000000, //       ####      
  0b00000111, 0b01100000, //      ### ##     
  0b00001101, 0b00110000, //     ## #  ##    
  0b00001001, 0b00110000, //     #  #  ##    
  0b00000001, 0b01100000, //        # ##     
  0b00000001, 0b11000000, //        ###      
  0b00000001, 0b10000000, //        ##       
};


// 'mqtt-ver__small', 32x28px
uint8_t mqtt_icon32x28 [112] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xf1, 0xff, 0xff, 0xfc, 0x39, 0xff, 
	0xff, 0xff, 0x1f, 0xff, 0xff, 0x9f, 0x8f, 0xff, 0xff, 0x83, 0xc7, 0xff, 0xff, 0xe1, 0xe7, 0xff, 
	0xff, 0xfc, 0x73, 0xff, 0xff, 0xfe, 0x73, 0xff, 0xff, 0x8f, 0x39, 0xff, 0xff, 0x87, 0x19, 0xff, 
	0xff, 0x83, 0x99, 0xff, 0xff, 0x81, 0x9d, 0xff, 0xff, 0x81, 0x9d, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xb8, 0xc1, 0x87, 0xe7, 0x37, 0x77, 0xcf, 
	0xe7, 0x2f, 0x37, 0xdf, 0xea, 0x8f, 0x37, 0xdf, 0xe8, 0xaf, 0x37, 0xdf, 0xed, 0xb7, 0x77, 0xdf, 
	0xff, 0xb8, 0x77, 0xdf, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


// 'logo-wifi-clipart-3', 40x40px
uint8_t wifi_icon40x40 [200] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x1f, 0xff, 0xff, 
	0xc0, 0x00, 0x03, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x03, 
	0xff, 0xc0, 0x0f, 0xe0, 0x1f, 0xff, 0xf8, 0x07, 0xc0, 0xff, 0xff, 0xfe, 0x03, 0xe1, 0xff, 0xff, 
	0xff, 0x87, 0xe7, 0xff, 0x81, 0xff, 0xcf, 0xff, 0xf8, 0x00, 0x1f, 0xff, 0xff, 0xc0, 0x00, 0x03, 
	0xff, 0xff, 0x80, 0x00, 0x01, 0xff, 0xfe, 0x00, 0x7e, 0x00, 0x7f, 0xfe, 0x03, 0xff, 0xc0, 0x7f, 
	0xff, 0x0f, 0xff, 0xf0, 0xff, 0xff, 0x3f, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x01, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x3f, 0xff, 0xff, 0xf0, 0x00, 0x1f, 0xff, 0xff, 0xf0, 
	0x00, 0x1f, 0xff, 0xff, 0xf8, 0xfe, 0x3f, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, 0xff, 0xff, 0xff, 0xff, 0x83, 0xff, 
	0xff, 0xff, 0xff, 0x81, 0xff, 0xff, 0xff, 0xff, 0x83, 0xff, 0xff, 0xff, 0xff, 0x83, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

uint16_t screenColor = 12;

void clearScreen(uint16_t _screenColor){
  // M5.Lcd.clearDisplay();
  // if(screenColor!=_screenColor){
  //   screenColor=_screenColor;
  //   M5.Lcd.fillScreen(screenColor);
  // }
  M5.Lcd.fillScreen(WHITE);
}



void ShowOnOledLarge(char *Line1, uint8_t* icon,int16_t w, int16_t h,uint16_t Pause=1,uint16_t headerColor=BLACK){
  clearScreen(WHITE);
  // M5.Lcd.clearDisplay();
  M5.Lcd.setTextColor(headerColor);
  M5.Lcd.drawBitmap(116, 0, w, h, icon);
  if (Line1) {
     int pos = round( ((M5.Lcd.width()-1) - (12 * strlen(Line1))) / 2 );
    M5.Lcd.setCursor(pos, 4); // 16
    M5.Lcd.print(Line1);
  }
  M5.Lcd.display();
  delay(Pause);  // Pause indicated time in ms
}

void ShowOnOledLarge(String Line1, uint8_t* icon,int16_t w, int16_t h,uint16_t Pause=1,uint16_t headerColor=BLACK){
  clearScreen(WHITE);  // M5.Lcd.clearDisplay();
  M5.Lcd.setTextColor(headerColor);
  M5.Lcd.drawBitmap(116, 0, w, h, icon);
  if (Line1) {
     int pos = round( ((M5.Lcd.width()-1) - (12 * Line1.length())) / 2 );
    M5.Lcd.setCursor(pos, 4); // 16
    M5.Lcd.print(Line1);
  }
  M5.Lcd.display();
  delay(Pause);  // Pause indicated time in ms
}



void ShowOnOledLarge(char *Line1, char *Line2, char *Line3, uint16_t Pause=1,uint16_t headerColor=BLACK) {
  // Clear and set Oled to display 3 line info -> centered
  int pos = 1;
  clearScreen(WHITE);
  M5.Lcd.setTextColor(headerColor);
  if (mqttClient.connected()) { // show BLE icon
    // M5.Lcd.drawBitmap(116, 0, 16, 16, bluetooth_icon16x16);
    M5.Lcd.drawBitmap(116, 0, 32, 28, mqtt_icon32x28);
  } // shift icon to the right as much as possible
  M5.Lcd.setTextSize(2);  // Large characters 11 pixels wide
  if (Line1) {
    pos = round( ((M5.Lcd.width()-1) - (12 * strlen(Line1))) / 2 );
    M5.Lcd.setCursor(pos, 2); // 16
    M5.Lcd.print(Line1);
  }
  if (Line2) {
    pos = round( ((M5.Lcd.width()-1) - (12 * strlen(Line2))) / 2 );
    M5.Lcd.setCursor(pos, 22); // 16
    M5.Lcd.print(Line2);
  }
  if (Line3) {
    pos = round( ((M5.Lcd.width()-1) - (12 * strlen(Line3))) / 2 );
    M5.Lcd.setCursor(pos, 44); // 16
    M5.Lcd.print(Line3);
  }
  M5.Lcd.display();
  delay(Pause);  // Pause indicated time in ms
}




void BuildBasicOledScreen(void) {
  // M5.Lcd.clearDisplay(); // clean the oled screen
  clearScreen(WHITE);
  M5.Lcd.setTextColor(BLACK);
  if (mqttClient.connected()) { // show BLE icon
    //  M5.Lcd.drawBitmap(116, 0, 16, 16, bluetooth_icon16x16);
    M5.Lcd.drawBitmap(116, 0, 32, 28, mqtt_icon32x28);
  } // shift icon to the right as much as possible
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(14, 32);
  M5.Lcd.print(F("Watt"));
  M5.Lcd.setCursor(62, 32);
  M5.Lcd.print(F("Rpm"));
  M5.Lcd.setCursor(99, 32);
  M5.Lcd.print(F("Kph"));
  M5.Lcd.setCursor(102, 10);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print(F("%"));
} // ---------------------------

unsigned long lastDisplayUpdateValue = 0;
// Funtion to show measurement data: Grade, Power, Cadence and Speed on Oled screen
void ShowValuesOnOled(void) {
   if (millis() - lastDisplayUpdateValue > 100) {

    BuildBasicOledScreen();
    M5.Lcd.setTextColor(RED);
    char tmp[30];
    dtostrf(gradePercentValue, 5, 1, tmp); // show sign only if negative
    M5.Lcd.setCursor(10, 6);
    M5.Lcd.setTextSize(3);
    M5.Lcd.print(tmp);
    sprintf(tmp, "%03d %03d %02d", PowerValue, InstantaneousCadence, int(SpeedValue + 0.5));
    M5.Lcd.setCursor(4, 44);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(tmp);
    if(menue_Btn!=M5BUTTON::NONE){
        switch(menue_Btn){
            case M5BUTTON::BTN_A:
                  M5.Lcd.setCursor(6, 80);
                  M5.Lcd.setTextSize(2);
                  sprintf(tmp, "GradeChangeFactor\n\n+    %03d%     -",GradeChangeFactor);
                  // sprintf(tmp, "+    GradeChangeFactor     -");
                  // Serial.println(tmp);
                  // M5.Lcd.drawString(String(tmp),120,M5.Lcd.width()/2);
                  M5.Lcd.print(tmp);
            break;
            case M5BUTTON::BTN_B:
                  M5.Lcd.setCursor(6, 80);
                  M5.Lcd.setTextSize(2);
                  sprintf(tmp, "aRGVmax\n\n+    %03d%     -",aRGVmax);
                  // sprintf(tmp, "+    aRGVmax     -");
                  // Serial.println(tmp);
                  M5.Lcd.print(tmp);
                  // M5.Lcd.drawString(String(tmp),120,M5.Lcd.width()/2);
            break;
            case M5BUTTON::BTN_C:
                  M5.Lcd.setCursor(6, 80);
                  M5.Lcd.setTextSize(2);
                  sprintf(tmp, "aRGVin\n\n+    %03d%     -",aRGVmax);
                  // sprintf(tmp, "+    aRGVin     -");
                  // Serial.println(tmp);
                  M5.Lcd.print(tmp);
                  // M5.Lcd.drawString(String(tmp),120,M5.Lcd.width()/2);
            break;
            default:
            break;
        }
    }
    M5.Lcd.display();
    lastDisplayUpdateValue = millis();
   }
}// -----------------------------------