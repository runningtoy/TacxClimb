#include "Lifter.h"
// #include "SPI.h"
// #include "Wire.h"

//#include "Adafruit_GFX.h"
//#include "Adafruit_SSD1306.h"

#include "MovingAverageFilter.h"
// Declare the running average filter for VL53L0X Range measurements
// Filter is only used in the Lifter Class 
// Sampling is at about 10 Hz --> 10 VL53L0X-RANGE-readings per second
#define NUMBER_OF_RANGE_READINGS 20
// Instantiate MovingAverageFilter class
  MovingAverageFilter movingAverageFilter_Range(NUMBER_OF_RANGE_READINGS);
  
// Instantiate Lifter class
  Lifter::Lifter() {
  }

void Lifter::Init(int OutPin1, int OutPin2, int MINPOS, int MAXPOS, int BANDWTH)
{
  _actuatorOutPin1 = OutPin1;
  _actuatorOutPin2 = OutPin2;
  //  setup control pins and set to BRAKE by default
  pinMode(_actuatorOutPin1, OUTPUT);
  pinMode(_actuatorOutPin2, OUTPUT);
  // set brake
  digitalWrite(_actuatorOutPin1, LOW);
  digitalWrite(_actuatorOutPin2, LOW);
  // 
  _BANDWIDTH = BANDWTH;
  _MINPOSITION = MINPOS;
  _MAXPOSITION = MAXPOS;

  // NOTICE: COMPILER DIRECTIVE !!!!
  // that leaves out almost all print statements!
//#define MYDEBUG is uit !!
  
#ifdef MYDEBUG
  Serial.print("Pin1: "); Serial.print(_actuatorOutPin1); Serial.print(" Pin2: "); Serial.print(_actuatorOutPin2); 
  Serial.print(" BandWidth: "); Serial.print(_BANDWIDTH); Serial.print(" MinPosition: "); Serial.print(_MINPOSITION);
  Serial.print(" MaxPosition: "); Serial.print(_MAXPOSITION); Serial.println();  
#endif
// Private variables for position control
  _IsBrakeOn = true;
  _IsMovingUp = false;
  _IsMovingDown = false;
  _TargetPosition = 400; // Choose the safe value of a flat road
  
// setup wire communication and defaults for the VL53L0X
  Wire.begin();
  
  // sensor.configSensor(sensor.VL53L0X_SENSE_LONG_RANGE);
  sensor.begin(ToF_ADDR);

  sensor.startRangeContinuous();
  
// fill the movingAverageFilter with actual values instead of default zero's....
// that blur operation in the early stages (of testing..)
  for (int i = 0; i < 10; i++) {
    _CurrentPosition = GetVL53L0X_Range_Reading();
    } 
}

int16_t Lifter::GetVL53L0X_Range_Reading()
{
  if (sensor.timeoutOccurred()) 
        { 
#ifdef MYDEBUG
        Serial.print(" TIMEOUT"); Serial.println();
#endif
        }
  return movingAverageFilter_Range.process(sensor.readRange());  
}

bool Lifter::TestBasicMotorFunctions()
{
#ifdef MYDEBUG 
  Serial.print("Testing VL53L0X and motor functioning..."); Serial.println();
#endif  
  for (int i = 0; i < NUMBER_OF_RANGE_READINGS; i++) {  // Determine precise position after 800 ms of movement !
    GetVL53L0X_Range_Reading();
  } 
  int16_t PresentPosition01 = GetVL53L0X_Range_Reading();
#ifdef MYDEBUG  
  Serial.print("Start at position: "); Serial.print(PresentPosition01);Serial.println();
#endif
  if (PresentPosition01 != (constrain(PresentPosition01, _MINPOSITION, _MAXPOSITION)) )
  { // VL6108X is out of Range ... ?
#ifdef MYDEBUG
    Serial.print(">> ERROR << -> VL6108X Out of Range at start !!"); Serial.println();
#endif
    return false; 
  }
#ifdef MYDEBUG
  Serial.print("Moving UP ..."); Serial.println();
#endif
  moveActuatorUp();
  delay(800); // Wait for some time
  brakeActuator();
  
  for (int i = 0; i < NUMBER_OF_RANGE_READINGS; i++) {  // Determine precise position after 800 ms of movement !
     _CurrentPosition = GetVL53L0X_Range_Reading();
     } 
 
  int16_t PresentPosition02 = (_CurrentPosition + _BANDWIDTH);
  if (PresentPosition02 != (constrain(PresentPosition02, _MINPOSITION, _MAXPOSITION)) )
  { // VL6108X is out of Range ... ?
#ifdef MYDEBUG
    Serial.print(">> ERROR << -> VL6108X Out of Range"); Serial.println();
#endif
    return false; 
  }
  if (!(PresentPosition02 < PresentPosition01))
    { 
#ifdef MYDEBUG
    Serial.print(">> ERROR << -> VL6108X did not detect an UP movement"); Serial.println();
#endif
    return false;
    }
  // VL6108X is properly working moving UP! ------------------------------------
#ifdef MYDEBUG  
  Serial.print("Moving Down ..."); Serial.println();
#endif
  moveActuatorDown();
  delay(1600); // Wait some time (extra to "undo" the previous Up movement!!)
  brakeActuator();
  
  for (int i = 0; i < NUMBER_OF_RANGE_READINGS; i++) {  // Determine precise position after 2400 ms of movement !
     _CurrentPosition = GetVL53L0X_Range_Reading();
     } 
 
  PresentPosition01 = (_CurrentPosition - _BANDWIDTH);
  if (PresentPosition01 != (constrain(PresentPosition01, _MINPOSITION, _MAXPOSITION)) )
  { // VL6108X is out of Range ... ?
#ifdef MYDEBUG
    Serial.print(">> ERROR << -> VL6108X Out of Range"); Serial.println();
#endif
    return false; 
  }
  if (!(PresentPosition01 > PresentPosition02))
    { 
#ifdef MYDEBUG
    Serial.print(">> ERROR << -> VL6108X did not detect a DOWN movement"); Serial.println();
#endif
    return false;
    }
  // AND VL6108X is properly moving DOWN ! --------------------------------------
#ifdef MYDEBUG  
  Serial.print("VL53L0X and motor properly working ..."); Serial.println();
#endif
  return true;
}

int16_t Lifter::GetPosition()
{
  return GetVL53L0X_Range_Reading();
}




int Lifter::GetOffsetPosition()
{
  _CurrentPosition = GetVL53L0X_Range_Reading();
  int16_t _PositionOffset = _TargetPosition - _CurrentPosition;
  if (sensor.timeoutOccurred()) 
    {
#ifdef MYDEBUG 
      Serial.print(" TIMEOUT"); Serial.println();
#endif
    return 3;
    }
#ifdef MYDEBUG
  Serial.print("Target: "); Serial.print(_TargetPosition);
  Serial.print("  Current: "); Serial.print(_CurrentPosition);
  Serial.print("  Offset: "); Serial.print(_PositionOffset); 
#endif
    
  if ( (_PositionOffset >= -_BANDWIDTH) && (_PositionOffset <= _BANDWIDTH) )
    { // postion = 0 + or - BANDWIDTH so don't move anymore!
#ifdef MYDEBUG
    Serial.print(" offset = 0 (within bandwidth) "); Serial.println();
#endif
    return 0; 
    }
  if ( _PositionOffset < 0 )
  {
#ifdef MYDEBUG
   Serial.print(" offset < 0 "); Serial.println();
#endif
   return 1;    
  }
  else 
  {
#ifdef MYDEBUG
   Serial.print(" offset > 0 "); Serial.println();
#endif
   return -1;     
  }
  // default --> error... stop!
#ifdef MYDEBUG
  Serial.print(" BRAKE --> Offset comparison error!"); Serial.println();
#endif
  return 0; 
}

void Lifter::SetTargetPosition(int16_t Tpos)
{
  _TargetPosition = Tpos;
}

void Lifter::moveActuatorUp()
  { 
  // FORWARD
  if (_CurrentPosition <= (_MINPOSITION + _BANDWIDTH) )
    { // Stop further movement to avoid destruction...
    _IsMovingUp = false;
#ifdef MYDEBUG
    Serial.print(" Stop MovingUp ");
#endif
    brakeActuator();
    return;
    }
    // DO NOT REPEATEDLY set the same motor direction 
    if (_IsMovingUp) {return;}
    
                              
    digitalWrite(_actuatorOutPin2, HIGH);
    digitalWrite(_actuatorOutPin1, LOW); 
    delay(200);
    _IsMovingUp = true;
    _IsMovingDown = false;
    _IsBrakeOn = false;
#ifdef MYDEBUG
    Serial.println(" Set MovingUp ");
#endif 
  }

void Lifter::moveActuatorDown()
  { 
  // REVERSE
  if (_CurrentPosition >= (_MAXPOSITION - _BANDWIDTH) )
    { // Stop further movement to avoid destruction...
    _IsMovingDown = false;
#ifdef MYDEBUG
    Serial.println(" Stop MovingDown ");
#endif
    brakeActuator();
    return;
    }
    // DO NOT REPEATEDLY set the same motor direction
    if (_IsMovingDown) {return;}
    
    // moving in the wrong direction or not moving at all
                               
    digitalWrite(_actuatorOutPin2, LOW);
    digitalWrite(_actuatorOutPin1, HIGH);
    delay(200);
    _IsMovingDown = true;
    _IsMovingUp = false;
    _IsBrakeOn = false;
#ifdef MYDEBUG
    Serial.println(" Set MovingDown ");
#endif
  }

void Lifter::brakeActuator()
  { 
    // BRAKE
    // DO NOT REPEATEDLY stop the motor
    if (_IsBrakeOn) {return;}
    digitalWrite(_actuatorOutPin1, LOW);
    digitalWrite(_actuatorOutPin2, LOW);
    delay(200);
    _IsBrakeOn = true;
    _IsMovingDown = false;
    _IsMovingUp = false; 
#ifdef MYDEBUG   
    Serial.println(" Set Brake On ");
#endif
  }

  void Lifter::gotoTargetPosition()
  {
    switch (GetOffsetPosition())
    {
    case 0:
      brakeActuator();
#ifdef MYDEBUG
      Serial.println(F(" -> Brake"));
#endif
      break;
    case 1:
      moveActuatorUp();
#ifdef MYDEBUG
      Serial.println(F(" -> Upward"));
#endif
      break;
    case -1:
      moveActuatorDown();
#ifdef MYDEBUG
      Serial.println(F(" -> Downward"));
#endif
      break;
    default:
      // Timeout --> OffsetPosition is undetermined --> do nothing
#ifdef MYDEBUG
      brakeActuator();
      Serial.println(F(" -> Timeout"));
#endif
      break;
    }
  }


  int16_t Lifter::getMaxUpperPositon(){
      int16_t _lastPosition = GetVL53L0X_Range_Reading();
      int16_t _actPosition = GetVL53L0X_Range_Reading();
      moveActuatorUp();
      do
      {
        delay(100);
        _actPosition=GetVL53L0X_Range_Reading();
      } while (_lastPosition<_actPosition);
      brakeActuator();

      Serial.print("UpperPosition: ");
      Serial.println(_actPosition);

  }


    int16_t Lifter::getMaxLowerPositon(){
      int16_t _lastPosition = GetVL53L0X_Range_Reading();
      int16_t _actPosition = GetVL53L0X_Range_Reading();
      moveActuatorDown();
      do
      {
        delay(100);
        _actPosition=GetVL53L0X_Range_Reading();
      } while (_lastPosition>_actPosition);
      brakeActuator();
      Serial.print("LowerPosition: ");
      Serial.println(_actPosition);

  }


  void Lifter::autoCalibrate(){
      getMaxUpperPositon();
      getMaxLowerPositon();
  }