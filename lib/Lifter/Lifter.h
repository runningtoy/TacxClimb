/*
 * 
 * 
 * 
 */
#ifndef Lifter_h
#define Lifter_h

#include "Arduino.h"
// #include "VL6180X.h"
#include "Adafruit_VL53L0X.h"
#define ToF_ADDR 0x29//the iic address of tof

class Lifter {
 
  Adafruit_VL53L0X sensor;
  bool _IsBrakeOn;
  bool _IsMovingUp;
  bool _IsMovingDown;
  int _actuatorOutPin1;
  int _actuatorOutPin2;
  int16_t _TargetPosition;
  int16_t _CurrentPosition;
  int _BANDWIDTH;
  int _MINPOSITION;
  int _MAXPOSITION;
  
public:
 
  Lifter();
  void Init(int OutPin1, int OutPin2, int MINPOS, int MAXPOS, int BANDWTH);
  int16_t GetVL53L0X_Range_Reading();
  int16_t GetPosition();
  void SetTargetPosition(int16_t Tpos);
  bool TestBasicMotorFunctions();
  int GetOffsetPosition();
  void moveActuatorUp();
  void moveActuatorDown();
  void brakeActuator();
  void gotoTargetPosition();
  void autoCalibrate();
  int16_t getMaxLowerPositon();
  int16_t getMaxUpperPositon();
};

#endif
