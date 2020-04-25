
/* ++++++++++ SMART BMS ++++++++++

  Original code by bress55
  23/09/2018
  Using Arduino Mega256
  https://github.com/bres55/Smart-BMS-arduino-Reader/blob/master/README.md

  20/04/2020
  Refactored and updated to use NodeMCU ESP8266.
  Moved to PlatformIO from Arduino IDE.
  https://github.com/CRaccani/smart-BMS-NodeMCU/blob/master/README.md
  
*/
#include <Arduino.h>

#define MySerial Serial  // Serial   - set this to the hardware serial port you wish to use... 
#define MyDebug Serial1  // Serial 1 - monitor output - Debug print()

// START //

// prints integer in binary format, nibbles, with leading zeros
void print_binary(int v, int num_places);
byte Bit_Reverse( byte x );
void flush();
void write_request_start();
void write_request_end();
void e_write_request_end();
uint16_t two_ints_into16(int highbyte, int lowbyte); // turns two bytes into a single long integer
void get_bms_feedback();  
uint8_t call_read_eprom();
void storeCellVoltageInfo();
void storeBasicInfo();
void call_Basic_info();
void call_get_cells_v();
void call_Hardware_info();
void control_mosfet();  //5A E1
void call_control_mosfet();  // the sequence required to control the mosfet
void change_cells_balance();  //5A E2
void call_change_cells_balance();
void eprom_read();   //BAR CODE
void eprom_end(); // no need at mo
String getcommand();
void getPOVP();
void getPUVP();
void getCOVP();
void getCUVP();
void getPOVPRelease();
void getPUVPRelease();
void getCOVPRelease();
void getCUVPRelease();
void getCHGOC();
void getDSGOC();
void getBatteryFunctions();
void writeEEPROM();
void controlMOSFET();
void otherFuntionBalanceControl();
