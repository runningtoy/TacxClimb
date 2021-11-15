
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


void ShowOnOledLarge(char *Line1, char *Line2, char *Line3, uint16_t Pause=1,uint16_t headerColor=BLACK) {
  // Clear and set Oled to display 3 line info -> centered
  int pos = 1;
  M5.Lcd.fillScreen(WHITE); 
  M5.Lcd.clearDisplay();
  M5.Lcd.setTextColor(headerColor);
  if (mqttClient.connected()) { // show BLE icon
    M5.Lcd.drawBitmap(116, 0, 16, 16, bluetooth_icon16x16);
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
  M5.Lcd.clearDisplay(); // clean the oled screen
  M5.Lcd.fillScreen(WHITE); 
  M5.Lcd.setTextColor(BLACK);
  if (mqttClient.connected()) { // show BLE icon
     M5.Lcd.drawBitmap(116, 0, 16, 16, bluetooth_icon16x16);
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


// Funtion to show measurement data: Grade, Power, Cadence and Speed on Oled screen
void ShowValuesOnOled(void) {
  BuildBasicOledScreen();
  M5.Lcd.setTextColor(RED);
  char tmp[12];
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
                M5.Lcd.setCursor(12, 44);
                M5.Lcd.setTextSize(2);
                //sprintf(tmp, "+    %03d%     -",GradeChangeFactor);
                sprintf(tmp, "+    GradeChangeFactor     -");
                M5.Lcd.print(tmp);
          break;
          case M5BUTTON::BTN_B:
                M5.Lcd.setCursor(12, 44);
                M5.Lcd.setTextSize(2);
                sprintf(tmp, "+    aRGVmax     -");
                M5.Lcd.print(tmp);
          break;
          case M5BUTTON::BTN_C:
                M5.Lcd.setCursor(12, 44);
                M5.Lcd.setTextSize(2);
                sprintf(tmp, "+    aRGVin     -");
                M5.Lcd.print(tmp);
          break;
          default:
          break;
      }
  }
  M5.Lcd.display();
}// -----------------------------------