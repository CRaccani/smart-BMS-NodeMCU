
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

//#define CONSOLE_SHOW_VOLTAGES   (1)
//#define CONSOLE_SHOW_BASIC_INFO (1)

#define MySerial Serial  // Serial   - set this to the hardware serial port you wish to use... 
#define MyDebug Serial1  // Serial 1 - monitor output - Debug print()

uint16_t voltages[14];
uint16_t voltagesMax[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint16_t voltagesMin[14] = {5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000};
int cellBallance[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

String inString = "";      // string to hold input
String inStringpc = "";    // string to hold PC input
int show = 1;    // show all data flag
int incomingByte, Length, highbyte, lowbyte;
byte Mosfet_control, mosfetnow, BatteryConfigH, BatteryConfigL, bcl, bcln, bch, Checksum, switche;
uint8_t BYTE1, BYTE2, BYTE3, BYTE4, BYTE5, BYTE6, BYTE7, BYTE8, BYTE9, BYTE10;
uint8_t inInts[40], data[9], BalanceCode;   // an array to hold incoming data, not seen any longer than 34 bytes, or 9
uint16_t a16bitvar;
float  eresultf; //Cellv1, Cellv2, Cellv3, Cellv4, Cellv5, Cellv6, Cellv7, Cellv8,
float CellMin = 5, CellMax = 0, Cellsum = 0, Celldiff;

// Pack Voltage
float PackVoltagef;
// CURRENT
float PackCurrentf;
//RSOC remaining state of charge
int RSOC;

// START //
byte Bit_Reverse( byte x )
// http://www.nrtm.org/index.php/2013/07/25/reverse-bits-in-a-byte/
{
  //          01010101  |         10101010
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  //          00110011  |         11001100
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  //          00001111  |         11110000
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;
}

// prints integer in binary format, nibbles, with leading zeros
void print_binary(int v, int num_places) // prints integer in binary format, nibbles, with leading zeros
// altered a bit, but got from here,  https://phanderson.com/arduino/arduino_display.html
{
  MyDebug.println("");
  int mask = 0, n, places = num_places;
  for (n = 1; n <= places; n++)
  {
    mask = (mask << 1) | 0x0001;
  }
  //v= Bit_Reverse( v );
  v = v & mask;  // truncate v to specified number of places
  while (places)
  {
    if (v & (0x0001 << (places - 1)))
    {
       MyDebug.print("1 ");
      cellBallance[places-1] = -1;
    }
    else
    {
       MyDebug.print("0 ");
      cellBallance[places-1] = 0;
    }
    --places;
    if (((places % 4) == 0) && (places != 0))
    {
       MyDebug.print("");
    }
  }
 MyDebug.println();

}

void flush()
{ // FLUSH
  delay(100); // give it a mo to settle, seems to miss occasionally without this
  while (MySerial.available() > 0)
  { MySerial.read();
  }
  delay(50); // give it a mo to settle, seems to miss occasionally without this
}

void write_request_start()
{
  flush(); // flush first
  MyDebug.println("write_request_start(DD 5A 00  02 56  78 FF 30 77)");
  //   DD 5A 00  02 56  78  FF 30   77
  uint8_t data[9] = {221, 90, 0, 2, 86, 120, 255, 48, 119};
  MySerial.write(data, 9);
}

void write_request_end()
{
  flush(); // flush first
 MyDebug.println("write_request_end(DD 5A 01  02 00 00 FF FD 77)");
  //   DD 5A 01  02 00 00   FF  FD 77
  uint8_t data[9] = {221, 90, 1, 2, 0, 0, 255, 253, 119};
  MySerial.write(data, 9);
}

void e_write_request_end()
{
  flush(); // flush first
  delay(50);
  //   DD  5A  1 2 28  28   FF   AD   77
  //  221  90  1 2 40  40  255  173  119

  uint8_t data[9] = {221, 90, 1, 2, 40, 40, 255, 173, 119};
  MySerial.write(data, 9);
}

uint16_t two_ints_into16(int highbyte, int lowbyte) // turns two bytes into a single long integer
{
  a16bitvar = (highbyte);
  a16bitvar <<= 8; //Left shift 8 bits,
  a16bitvar = (a16bitvar | lowbyte); //OR operation, merge the two
  return a16bitvar;
}

void get_bms_feedback()  
// returns with up to date, inString= chars, inInts= numbers, chksum in last 2 bytes
//                          Length
//                          Data only, exclude first 3 bytes
{
  MyDebug.println("get_bms_feedback()");
  inString = ""; // clear instring for new incoming
  delay(100); // give it a mo to settle, seems to miss occasionally without this
  if (MySerial.available() > 0) {
    {
      for (int i = 0; i < 4; i++)               // just get first 4 bytes
      {
        incomingByte = MySerial.read();
        MyDebug.print(incomingByte, HEX);
        MyDebug.print(" ");
        if (i == 3)
        { // could look at 3rd byte, it's the ok signal
          Length = (incomingByte); 
          // The fourth byte holds the length of data, excluding last 3 bytes checksum etc
          // MyDebug.print(" inc ");
          //MyDebug.print(incomingByte);
        }
        if (Length == 0) {
          Length = 1; // in some responses, length=0, dont want that, so, make Length=1
        }
      }
      //  Length = Length + 2; // want to get the checksum too, for writing back, saves calculating it later
      for (int i = 0; i < Length + 2; i++) { // get the checksum in last two bytes, just in case need later
        incomingByte = MySerial.read(); // get the rest of the data, how long it might be.
        MyDebug.print(incomingByte, HEX);
        MyDebug.print(" ");
        inString += (char)incomingByte; // convert the incoming byte to a char and add it to the string
        inInts[i] = incomingByte;       // save incoming byte to array as int
      }
    }
  }
  MyDebug.println();
}

uint8_t call_read_eprom()
{
  flush(); // flush first
  // BYTES 3 and 6 need to be set first
  // DD A5 BYTE3 00 FF BYTE6 77
  uint8_t data1[7] = {221, 165, BYTE3, 0, 255, BYTE6, 119};
  MyDebug.println();
  MyDebug.println("call_read_eprom(DD A5 BYTE3 0 FF BYTE6 77)");
  MyDebug.print("BYTE3: "); MyDebug.println(BYTE3, HEX);
  MyDebug.print("BYTE6: "); MyDebug.println(BYTE6, HEX);
  
  MySerial.write(data1, 7);
  get_bms_feedback(); // get the data reply
  highbyte = (inInts[0]); // bytes 5 and 6, is where the actual data is
  lowbyte = (inInts[1]);
  uint16_t eresult = two_ints_into16(highbyte, lowbyte); // TURN THEM INTO ONE LONG INTEGER
  eresultf = eresult / 100.0f; // convert to float
  return eresultf;
}

void storeCellVoltageInfo()
{
  // and the values
  #ifdef CONSOLE_SHOW_VOLTAGES
    MyDebug.println ("Cell");
  #endif

  int cell = 2;
  for (int i = 0; i < Length; i = i + 2) {
    highbyte = (inInts[i]);
    lowbyte = (inInts[i + 1]);
    uint16_t Cellnow = two_ints_into16(highbyte, lowbyte);
    float Cellnowf = Cellnow / 1000.0f; // convert to float
    Cellsum = Cellsum + Cellnowf;

    // Cache the cell info for broadcast
    voltages[(cell/2)-1] = Cellnow; 
    if (Cellnow > voltagesMax[(cell/2)-1]) 
    {
      voltagesMax[(cell/2)-1]=Cellnow;
    }
    if (Cellnow < voltagesMin[(cell/2)-1]) 
    {
      voltagesMin[(cell/2)-1]=Cellnow;
    }

    // Averaged cell values for pack
    if (Cellnowf > CellMax) {   // get high and low
      CellMax = Cellnowf;
    }
    if (Cellnowf < CellMin) {
      CellMin = Cellnowf;
    }

    #ifdef CONSOLE_SHOW_VOLTAGES
      MyDebug.print (cell/2);
      MyDebug.print(" ");
      MyDebug.print(Cellnowf, 3); // 3 decimal places
      MyDebug.println();
    #endif

    cell+=2;
  }
  
  #ifdef CONSOLE_SHOW_VOLTAGES
    MyDebug.println();
    MyDebug.print("CellMax: "); // CellMax heading
    MyDebug.println(CellMax, 3); // 3 decimal places

    MyDebug.print("CellMin: "); // CellMin heading
    MyDebug.println(CellMin, 3); // 3 decimal places
  #endif

  Celldiff = (CellMax - CellMin)*1000.0; // difference between highest and lowest

  #ifdef CONSOLE_SHOW_VOLTAGES
    MyDebug.print("Diff: "); // diference heading
    MyDebug.println(Celldiff, 0); // 3 decimal places
  #endif

  Cellsum = Cellsum / (Length / 2); // Average of Cells
  
  #ifdef CONSOLE_SHOW_VOLTAGES
    MyDebug.print("Avg: "); // Average heading
    MyDebug.println(Cellsum, 3); // 3 decimal places
    MyDebug.println();
  #endif

}
  
void storeBasicInfo()
{
  #ifdef CONSOLE_SHOW_BASIC_INFO  
    MyDebug.println("Basic Info.");
  
    MyDebug.println();
    for (int i = 0; i < 40; i++) 
    {
      MyDebug.print(inInts[i], HEX);
      MyDebug.print(" ");
    }  
    MyDebug.println();
  #endif

  BalanceCode= inInts[12]; //  the 12th byte cells 16-9
 // BalanceCode = 34;
  //MyDebug.print("Balance code[12]: "); MyDebug.println(BalanceCode);
  // reverse the bits, so they are in same order as cells
 // BalanceCode = Bit_Reverse( BalanceCode ) ; 
  //MyDebug.println("Reversed code[13]: "); MyDebug.println(BalanceCode);
  print_binary(BalanceCode, 8);
  //move to hight cells to upper part of cell array
  for (int i = 0; i < 8; i++) 
  {
    cellBallance[i+8]=cellBallance[i];
  }  
  //MyDebug.println(BalanceCode, BIN); 
    // works, but, loses leading zeros and get confusing on screen
    // print balance state as binary, cell 1 on the right, cell 8 on left
    // Reversed this. 1 on left, 8 on right
  
  
  BalanceCode= inInts[13]; //  the 13th byte cells 8-1
  //BalanceCode = 34;
  //MyDebug.print("Balance code[13]: "); MyDebug.println(BalanceCode);
 // BalanceCode = Bit_Reverse( BalanceCode );
  //MyDebug.println("Reversed code[14]: "); MyDebug.print(BalanceCode);
  //MyDebug.println(BalanceCode, BIN); 
  print_binary(BalanceCode, 8);
    
    // PACK VOLTAGE,, bytes 0 and 1, its 16 bit, high and low
    highbyte = (inInts[0]); // bytes 0 and 1
    lowbyte = (inInts[1]);
    uint16_t PackVoltage = two_ints_into16(highbyte, lowbyte);
    PackVoltagef = PackVoltage / 100.0f; // convert to float and leave at 2 dec places

    #ifdef CONSOLE_SHOW_BASIC_INFO
      MyDebug.print("Pack Voltage = ");
      MyDebug.print(PackVoltagef);
    #endif

    // CURRENT can be +ve and -ve value so int16_t (signed)
    highbyte = (inInts[2]); // bytes 2 and 3
    lowbyte = (inInts[3]);
    int16_t PackCurrent = two_ints_into16(highbyte, lowbyte);

    PackCurrentf = PackCurrent / 100.0f; // convert to float and leave at 2 dec places
    #ifdef CONSOLE_SHOW_BASIC_INFO
      MyDebug.print("   Current = ");
      MyDebug.print(PackCurrentf);
    #endif

     //REMAINING CAPACITY
    highbyte = (inInts[4]);
    lowbyte = (inInts[5]);
    uint16_t RemainCapacity = two_ints_into16(highbyte, lowbyte);
    float RemainCapacityf = RemainCapacity / 100.0f; // convert to float and leave at 2 dec places
    
    #ifdef CONSOLE_SHOW_BASIC_INFO
      MyDebug.print("   Remaining Capacity = ");
      MyDebug.print(RemainCapacityf);
      MyDebug.print("Ah");
    #endif

    //RSOC
    RSOC = (inInts[19]);
    
    #ifdef CONSOLE_SHOW_BASIC_INFO
      MyDebug.print("   RSOC = ");
      MyDebug.print(RSOC);
      MyDebug.print("%");
    #endif

    //Temp probe 1
    highbyte = (inInts[23]);
    lowbyte = (inInts[24]);
    float Temp_probe_1 = two_ints_into16(highbyte, lowbyte);
    float Temp_probe_1f = (Temp_probe_1 - 2731) / 10.00f; // convert to float and leave at 2 dec places
    
    #ifdef CONSOLE_SHOW_BASIC_INFO
      MyDebug.println("");
      MyDebug.print("Temp probe 1 = ");
      MyDebug.print(Temp_probe_1f);
      MyDebug.print(" ");
    #endif

    //Temp probe 2
    highbyte = (inInts[25]);
    lowbyte = (inInts[26]);
    float Temp_probe_2 = two_ints_into16(highbyte, lowbyte);
    float Temp_probe_2f = (Temp_probe_2 - 2731) / 10.00f; // convert to float and leave at 2 dec places
    
    #ifdef CONSOLE_SHOW_BASIC_INFO
      MyDebug.print("   Temp probe 2 = ");
      MyDebug.print(Temp_probe_2f);
      MyDebug.println(" ");
    #endif
} 

void call_Basic_info()
// total voltage, current, Residual capacity, Balanced state, MOSFET control status
{
  flush(); // flush first
  MyDebug.println("call_Basic_info(DD A5 03 00 FF FD 77)");
  //  DD  A5 03 00  FF  FD  77
  // 221 165  3  0 255 253 119
  uint8_t data[7] = {221, 165, 3, 0, 255, 253, 119};
  MySerial.write(data, 7);

  // get that data, used to get 
  // BALANCE STATE byte 17 less 4, decimal=byte 13
  get_bms_feedback();

  storeBasicInfo();
}

void call_get_cells_v()
{
  flush(); // flush first
  MyDebug.println("call_get_cells(DD A5 04 00 FF FC 77)");
  // DD  A5  4 0 FF  FC  77
  // 221 165 4 0 255 252 119
  uint8_t data[7] = {221, 165, 4, 0, 255, 252, 119};
  MySerial.write(data, 7);

 // returns with up to date, inString= chars, inInts[]= numbers,
 // chksum in last 2 bytes
  get_bms_feedback();
}

void call_Hardware_info()
{
  flush(); // flush first
  MyDebug.println();
  MyDebug.println("call_Hardware_info(DD A5 05 00 FF FB 77)");
  //  DD  A5 05 00  FF  FB  77
  // 221 165  5  0 255 251 119
  uint8_t data[7] = {221, 165, 5, 0, 255, 251, 119};
  // uint8_t data[7] = {DD, A5, 05, 00, FF, FB, 77};
  MySerial.write(data, 7);

  get_bms_feedback();   // get that data
  MyDebug.println("BMS Name= " + inString.substring(0, 18)); // no need for now
}

void control_mosfet()  //5A E1
// a unique sequence for controlling the mosfets, control by BYTE 6 and BYTE 8
// all the other BYTEs are the same.
{
  flush(); // flush first
  //delay(50);
  //   DD 5A  E1 02 00  BYTE6  FF BYTE8  77
  //  221 90 225  2  0  BYTE6 255 BYTE8 119
  uint8_t data[9] = {221, 90, 225, 2, 0, BYTE6, 255, BYTE8, 119};
  MySerial.write(data, 9);
  // Serial.write(data, 9);
}

void call_control_mosfet()  // the sequence required to control the mosfet
{
  write_request_start();
  control_mosfet();
  write_request_end();
}

void change_cells_balance()  //5A E2
// a unique sequence for controlling 
//the balance control, to change 
//the balance, odd, even, close, exit
// control by BYTE 6 and BYTE 8
// all the other BYTEs are the same.
{
  flush(); // flush first
  delay(50);
  //   DD 5A E2  2  0  BYTE6  FF BYTE8  77
  //  221 90 226 2  0  BYTE6  255 BYTE8 119
  uint8_t data[9] = {221, 90, 226, 2, 0, BYTE6, 255, BYTE8, 119};
  MySerial.write(data, 9);
}

//the sequence required to change 
//the balance, odd, even, close, exit
void call_change_cells_balance()
{
  write_request_start();
  change_cells_balance();
}

void eprom_read()   //BAR CODE
{
  flush(); // flush first
  //delay(5);
  // SENT CODE depends on WHAT IS REQD???
  //   DD  A5  A2 0  FF 5E  77...BAR CODE
  //  221 165 162 0 255 94 119
  // uint8_t data[7] = {221, 165, 162, 0, 255, 94, 119};
  uint8_t data[7] = {221, 165, 32, 0, 255, 224, 119};
  MySerial.write(data, 7);
}

void eprom_end() // no need at mo
{
  flush(); // flush first
  // delay(5);
  //DD  A5  AA  0 FF  56  77
  //221 165 170 0 255 86  119
  // from eprom read
  uint8_t data[7] = {221, 165, 170, 0, 255, 86, 119};
  MySerial.write(data, 7);
}

// gets input from serial monitor, and returns with inStringpc, holding it
String getcommand()
{
  // inStringpc = "";  // clear instringpc for new incoming
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte != '\n') {
      inStringpc += (char)incomingByte;
    }
  }

  return inStringpc;
}

void getPOVP()
{
      // POVP,
      //       HEX  20           E0
      //       DEC  32          224
      //  DD  A5    20  0   FF   E0    77
      // 221  165   32  0  255  224  119
      // ONLY BYTES 3 AND 6 CHANGE
      BYTE3 = 32;
      BYTE6 = 224;
      call_read_eprom(); // having called this eresultf, will hold the float value
      float POVP =  eresultf;
      MyDebug.print(" POVP = ");
      MyDebug.print(POVP);
      MyDebug.println();
}
    
void getPUVP()
{
  // PUVP
  //   22  DE
  //   34  222
  // ONLY BYTES 3 AND 6 CHANGE
  BYTE3 = 34;
  BYTE6 = 222;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float PUVP =  eresultf;
  MyDebug.print(" PUVP = ");
  MyDebug.print(PUVP);
  MyDebug.println();
}

void getCOVP()
{
  // COVP
  // 36   24 
  // 220  DC
  // ONLY BYTES 3 AND 6 CHANGE
  BYTE3 = 36;
  BYTE6 = 220;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float COVP =  eresultf / 10.0f;
  MyDebug.print(" COVP = ");
  MyDebug.print(COVP);
  MyDebug.println();
}

void getCUVP()
{
  // CUVP
  // 38   26
  // 218  DA
  BYTE3 = 38;
  BYTE6 = 218;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float CUVP =  eresultf / 10.0f;
  MyDebug.print(" CUVP = ");
  MyDebug.print(CUVP);
  MyDebug.println();
}

void getPOVPRelease()
{
  // POVPRelease,
  //   HEX  21      DF
  //   DEC  33     223
  // ONLY BYTES 3 AND 6 CHANGE
  BYTE3 = 33;
  BYTE6 = 223;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float POVPRelease =  eresultf;
  MyDebug.print(" POVPRelease = ");
  MyDebug.print(POVPRelease);
  MyDebug.println();
}

void getPUVPRelease()
{
  // PUVPRelease
  //   35   221
  //   23   DD  
  // ONLY BYTES 3 AND 6 CHANGE
  BYTE3 = 35;
  BYTE6 = 221;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float PUVPRelease =  eresultf;
  MyDebug.print(" PUVPRelease = ");
  MyDebug.print(PUVPRelease);
  MyDebug.println();
}

void getCOVPRelease()
{
  // COVPRelease
  //   37   219
  //   25   DB
  // ONLY BYTES 3 AND 6 CHANGE
  BYTE3 = 37;
  BYTE6 = 219;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float COVPRelease =  eresultf / 10.0f;
  MyDebug.print(" COVPRelease = ");
  MyDebug.print(COVPRelease);
  MyDebug.println();
}

void getCUVPRelease()
{
  // CUVPRelease
  //   39   217
  //   27   D9
  BYTE3 = 39;
  BYTE6 = 217;
  call_read_eprom(); // having called this eresultf, will hold the float value
  float CUVPRelease =  eresultf / 10.0f;
  MyDebug.print(" CUVPRelease = ");
  MyDebug.print(CUVPRelease);
  MyDebug.println();
}

void getCHGOC()
{
  // CHGOC
  //   40   216
  //   28   D8
  BYTE3 = 40;
  BYTE6 = 216;
  call_read_eprom(); // having called this eresultf, will hold the float value
  uint16_t CHGOC =  eresultf / 1;
  MyDebug.print(" CHGOC = ");
  MyDebug.print(CHGOC);
  MyDebug.println();
}

void getDSGOC()
{
  // DSGOC
  //   41   215
  //   29   D7
  BYTE3 = 41;
  BYTE6 = 215;
  call_read_eprom(); // having called this eresultf, will hold the float value
  uint16_t DSGOC =  (eresultf + 5) / 10; // round it up
  MyDebug.print(" DSGOC = ");
  MyDebug.print(DSGOC);
  MyDebug.println();
}

void getBatteryFunctions()
{
  //  BatteryConfig, BALANCE ENABLE and CHARGE BALANCE CONTROL
  // long hand as different needs
  //   45   211
  //   2D   D3
  BYTE3 = 45;
  BYTE6 = 211;
  flush(); // flush first
  delay (100);
  uint8_t data1[7] = {221, 165, BYTE3, 0, 255, BYTE6, 119};
  MyDebug.println();
  MyDebug.println("getBatteryFunctions(DD A5 BYTE3 0 FF BYTE6 77)");
  MyDebug.print("BYTE3: "); MyDebug.println(BYTE3, HEX);
  MyDebug.print("BYTE6: "); MyDebug.println(BYTE6, HEX);
  
  MySerial.write(data1, 7);
  get_bms_feedback(); // get the data reply
  // highbyte = (inInts[0]); // bytes 5 and 6, is where the actual data is
  BatteryConfigH = (inInts[0]);
  BatteryConfigL = (inInts[1]);
  /*
      MyDebug.print("  BatteryConfigH = ");
      MyDebug.print( BatteryConfigH);
      MyDebug.print("   ");
      MyDebug.print("  BatteryConfigL = ");
      MyDebug.print( BatteryConfigL);
      MyDebug.print("   ");
      delay (100);
  */
  bch = BatteryConfigH; // this relies on showoff not being used right away, to get a real value for
  //                         BatteryconfigH and L
  //                         and if a change is made here, then the temp balance switches are reset
  bcl = BatteryConfigL;
  bcln = bcl;      // temp var to work on, of balance control

 //  Show the state of BALANCE control // bits 2 and 3 and 0..switch en.
  MyDebug.print(F("Balance Enable = ")); // (F) saves memory apparently
  switche = bcl;
  bcl = bcl >> 2; //>> (bitshift right), move bit 2 to bit 0
  bcl = bcl & 1; //& (bitwise and) just want bit 0
  MyDebug.println(bcl);

  MyDebug.print(F("Charge Balance = "));
  bcln = bcln >> 3; //>> (bitshift right), move bit 3 to bit 0
  bcln = bcln & 1; //& (bitwise and) just want bit 0 again
  MyDebug.println(bcln);
  
  //  Show the state of SWITCH
  MyDebug.print(F("Switch State = "));
  //>> switch already in bit 0
  switche = switche & 1; //& (bitwise and) just want bit 0
  MyDebug.println(switche);
  MyDebug.println();
  
}

void writeEEPROM()
{
  //-------------------- EPROM WRITES START ----------------------------------

  // vvvvvvvvvvvvvvvvvvvvvvvvv or move end of show on/off bracket to vvvvvvvvvvvv below
  // do we want to change Balance control??
  // beoff=balance off, beon=balance on
  // cboff=charge balance off, cbon=charge balance on
  // swoff=switch off, swon=switch enabled
  // if changes made with showoff, then some values will not be updated, so turn showon
  // to allow variables to be updated.

  if (inStringpc.equalsIgnoreCase("beoff")) { // Balance off
    bitClear(bcln, 2);                  //  If we are wanting it OFF, need to set the bit 2 low/clear, -4
  }
  if (inStringpc.equalsIgnoreCase("beon")) { // Balance on
    bitSet(bcln, 2);                 //  If we are wanting it ON, need to set the bit 2 high/set, +4
  }
  if (inStringpc.equalsIgnoreCase("cboff")) { // Charge Balance off
    bitClear(bcln, 3);                    //  If we are wanting it OFF, need to set the bit 3 low/clear, -8
  }
  if (inStringpc.equalsIgnoreCase("cbon")) {  // Charge Balance on
    bitSet(bcln, 3);                     //  If we are wanting it ON, need to set the bit 3 high/set, +8
  }
  if (inStringpc.equalsIgnoreCase("swoff")) { // switch off
    bitClear(bcln, 0);
  }
  if (inStringpc.equalsIgnoreCase("swon")) {  // switch on
    bitSet(bcln, 0);
  }
  // Checksum calculate
  // (data + length + command code) checksum, then Complement, then add 1, high bit first, low bit last
  if (bcln != bcl) {  
    // if they are not equal, we are trying to make a change
    //MyDebug.print(bcl);
    //MyDebug.print("  bcls"  );
    //  MyDebug.print(bcln);
    bcl = bcln;
    Checksum = bcln + 47; // 45+2+(BYTE5+BYTE6)=47+0+bcln
    Checksum = Checksum ^ B11111111; // complement it, by XOR
    Checksum = Checksum + 1;

    write_request_start();

    BYTE5 = bch, BYTE6 = bcln, BYTE8 = Checksum; // Change BYTES 5,6 and 8 with reqd data
    // DD  5A  2D  2 BYTE5 BYTE6 FF  BYTE8 77
    // 221 90  45  2 BYTE5 BYTE6 255 BYTE8 119

    uint8_t data[9] = {221, 90, 45, 2, BYTE5, BYTE6, 255, BYTE8, 119};
    MySerial.write(data, 9);

    // e_write_request_end(); // not needed
    write_request_end(); // finished eprom reads, stick with this one
  }
  //  MyDebug.println("   beoff = balance off,    cboff = charge balance off,  swoff = switch off");
  //  MyDebug.println("   beon  = balance on,     cbon  = charge balance on,   swon  = switch enable");
  //-------------------- EPROM WRITES END ----------------------------------
}

void controlMOSFET()
{
  //MOSFET FLAG.... still BASIC INFO
  //MyDebug.println("");
  // MyDebug.print("   mosfet flag= ");
  // MyDebug.print(inInts[20]); //  the 20th byte
  //Mosfet_control = (inInts[20]);

  //  END          USING BASIC INFO 03
  // ----------------------------------------------------------------------------------------------

  // MMMMMMMMMMMMMMMMMMMMMM    Mosfet_control       MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  // do we want to change it??,, Has to be done after MOSFET FLAG
  // mdoff=discharge off, mdon=discharge on
  // mcoff=charge off, mcon=charge on
  //            Remember, BMS conditions need to be considered too
  //            Cannot turn on Discharge if batteries are low
  //            Cannot turn on Charge if batteries are already high V.


  mosfetnow = Mosfet_control; // temp var to work on, of mosfet control
  //                               Set required bits, without touching the other bits
  if (inStringpc.equalsIgnoreCase("mdoff")) { //Discharge Mosfet, not case sensitive.
    bitClear(mosfetnow, 1);                  //  If we are wanting it OFF, need to set the bit 1 low/clear
  }
  if (inStringpc.equalsIgnoreCase("mdon")) {
    bitSet(mosfetnow, 1);                  //  If we are wanting it ON, need to set the bit 1 high/set
  }
  if (inStringpc.equalsIgnoreCase("mcoff")) { // Charge Mosfet.
    bitClear(mosfetnow, 0);                  //  If we are wanting it OFF, need to set the bit 0 low/clear
  }
  if (inStringpc.equalsIgnoreCase("mcon")) {
    bitSet(mosfetnow, 0);                  //  If we are wanting it ON, need to set the bit 0 high/set
  }
  // this method particular to mosfet control,  write_request_start(); control_mosfet(); write_request_end
  if (mosfetnow != Mosfet_control) {  // if they are not equal, we are trying to make a change
    Mosfet_control = mosfetnow;       // so mosfet control can equal the new setting we want

    if (Mosfet_control == 0) {        // Change BYTES 6 and 8 with reqd data
      BYTE6 = 3, BYTE8 = 26;
    }
    if (Mosfet_control == 1) {
      BYTE6 = 2, BYTE8 = 27;
    }
    if (Mosfet_control == 2) {
      BYTE6 = 1, BYTE8 = 28;
    }
    if (Mosfet_control == 3) {
      BYTE6 = 0, BYTE8 = 29;
    }
    call_control_mosfet(); // writing in this sequence is how JBDTools write to mosfet control
  }

  // Show the state of MOSFET control
  MyDebug.println("");
  MyDebug.print(F("Mosfet Charge = ")); // (F) saves memory apparently
  Mosfet_control = Mosfet_control & 1; //& (bitwise and) just want bit 0
  MyDebug.print(Mosfet_control);
  MyDebug.print(F("  Mosfet DisCharge = "));
  mosfetnow = mosfetnow >> 1; //>> (bitshift right) use variabe mosfetnow, move bit 1 to bit 0
  Mosfet_control = mosfetnow & 1; //& (bitwise and) just want bit 0 again
  MyDebug.println(Mosfet_control);
  MyDebug.println("");
  MyDebug.println("   mcooff = charge off,    mdoff = discharge off");
  MyDebug.println("   mcon   = charge on,     mdon  = discharge on");
}

void otherFuntionBalanceControl()
{
  // BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB TEMP BALANCE CONTROL, ODD/EVEN Short time  BBBBBBBBBBBBBBBBBB
  // having got this working, because of limitations, not much use....
  // And if eprom reads in action, balancing is reset before it even starts!!
  // hence, need to turn showoff, to see working. but honestly not much use.
  
     // Because I am now reading the eprom in loop, any change here gets reset,
     // left here for legacy, may come back to it, if I find it might be useful.
     // if you want to see it working, need to have showoff, and rem out the vvvvvvvvvvvv  to vvvvvvvvv bits
    if (inStringpc.equalsIgnoreCase("exit")) {   // EXIT, same as EXIT on JBDTools
     //                                  // BUT, if any mosfet controls are actioned, it effectively runs EXIT
     //                                            JBDTools has same issue.
     write_request_start();
     write_request_end() ;  // same as exit mode codes
    }
    else if (inStringpc.equalsIgnoreCase("close")) { // Close, same as CLOSE on JBDTools
     // Effectively, stops balancing, even if balancing in settings is on
     BYTE6 = 3, BYTE8 = 25; // Change BYTES 6 and 8 with reqd data
     call_change_cells_balance();
    }
    else if (inStringpc.equalsIgnoreCase("bodd")) { //balance ODD cells
     BYTE6 = 1, BYTE8 = 27; // Change BYTES 6 and 8 with reqd data
     call_change_cells_balance();
    }
    else if (inStringpc.equalsIgnoreCase("beven")) { //balance EVEN cells
     BYTE6 = 2, BYTE8 = 26; // Change BYTES 6 and 8 with reqd data
     call_change_cells_balance();
    }
  
  //  bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb TEMP balance control end  bbbbbbbbbbbbbbbbbbbbbbb  
}

/*
void setup() 
{
  // Serial for comms to BMS
  Serial.begin(9600);

  //Use alternative GPIO pins of D7/D8
  //D7 = GPIO13 = RECEIVE SERIAL
  //D8 = GPIO15 = TRANSMIT SERIAL
  Serial.swap();

  //Debug serial output
  MyDebug.begin(115200);
  MyDebug.setDebugOutput(true);
  MyDebug.println("Starting up ...");
}
*/

/*
void loop()
{
  
//  write_request_start();// Found this helps timing issue, by saying hello, hello.
//  write_request_end() ; // Or maybe it flushes out any rogue data.
//  write_request_start();// Any way it works,
//  write_request_end() ; // And accomodates long delays if you want them at the end.

  //-------------------- EPROM READS START ----------------------------------
  // get  POVP, POVPRelease, PUVP, PUVPRelease, 
  // COVP, COVPRelease, CUVP, CUVPRelease, CHGOC, DSGOC
  // from, bms eprom read, 10 in all

  // the start of read eprom
  write_request_start(); 

  // dont really need this
  // HARDWARE INF 05
  // get hardware info 05, the name of it, not really useful
  
  call_Hardware_info(); // requests model number etc
  getPOVP();
  getPUVP();
  getCOVP();
  getCUVP();
  getPOVPRelease();
  getPUVPRelease();
  getCOVPRelease();
  getCUVPRelease();
  getCHGOC();
  getDSGOC();
  getBatteryFunctions();

  // requests cells voltage
  // store in memory structure
  call_get_cells_v();

  // USING BASIC INFO 03 get
  // CELL BALANCE... info
  call_Basic_info();      // requests basic info.

  // finished eprom reads
  write_request_end();

  //delay(100);
  // -------------------      EPROM READS END    -----------------------


  
  // tidy up
  Cellsum = 0;
  CellMin = 5;
  CellMax = 0;
  Length = 0;
  // new line, what ever happens
  MyDebug.println("");
  MyDebug.println("");

  //write_request_end();

}
// eeeeeeeeeeeeeeeeeeeeennnnnnnnnnnnnnnnnnnnnnndddddddddddddddddddddd
//     END
// eeeeeeeeeeeeeeeeeeeeennnnnnnnnnnnnnnnnnnnnnndddddddddddddddddddddd
*/
