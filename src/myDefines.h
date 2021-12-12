#undef ULONG_MAX
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)


//IF Core is powered by USB/Battery
// #define USB_POWERED
// #define MYDEBUG
#define DIRECTFANCONTROL


// Feather M5Stack CORE2 I/O Pin declarations for connection to Motor driver board MDD3A
#define actuatorOutPin1 GPIO_NUM_26    // --> GPIO27 connected to pin M1A of the MDD3A Motor Driver board
#define actuatorOutPin2 GPIO_NUM_25    // --> GPIO19 connected to the M1B of the MDD3A Motor Driver board


#define I2C_SDA 32
#define I2C_SCL 33

#define BLE_SCAN_TIMEOUT 30

// #define RESET_BTN_PIN

// -------------------------- WARNING ------------------------------------------------------------
// The following VL6180X sensor values are a 100% construction specific and
// should be experimentally determined, when the Actuator AND the VL6180X sensor are mounted!
// ------>>>> Test manually and use example/test sketches that go with the VL6180X sensor! <<<<---
// Microswitches should limit physically/mechanically the upper and lower position of the Actuator!
// The microswitches are mechanically controlled, and NOT by the software --> should be fail safe!
// Notice that unrestricted movement at the boundaries can damage the Actuator and/or construction!
// The following values are respected by the software and will (in normal cases!) never be exceeded!
#define MINPOSITION 22 // VL6180X highest value top microswitch activated to mechanically stop operation
#define MAXPOSITION 299 // VL6180X lowest value bottom microswitch activated to mechanically stop operation
// -------------------------- WARNING ------------------------------------------------------------
// Operational boundaries of the VL6180X sensor are used/set in class Lifter after calling its "init".
// A safe measuring range of at least 30 cm of total movement is recommended for the VL6180X sensor setting!
//
// Bandwidth is used in the code to take measuring errors and a safe margin into account when reaching
// the above defined max or min positions of the construction! The software does painstakingly respect
// these and is independent of the appropriate working of the microswitches when reaching the boundaries!
// These microswitches are a SECOND line of defence against out of range and potentially damaging movement!
#define BANDWIDTH 5