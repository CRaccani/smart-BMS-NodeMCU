
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "BMSUtils.h"
#include "html_1.h"
#include "css_1.h"
//#include "jquery.h"
//#include "logo.h"
#include "echarts_js.h"

#define MySerial Serial  // Serial   - set this to the hardware serial port you wish to use... 
#define MyDebug Serial1  // Serial 1 - monitor output - Debug print()


extern String inString;      // string to hold input
extern String inStringpc;    // string to hold PC input
extern int show;    // show all data flag
extern int incomingByte, BalanceCode, Length, highbyte, lowbyte;
extern byte Mosfet_control, mosfetnow, BatteryConfigH, BatteryConfigL, bcl, bcln, bch, Checksum, switche;
extern uint8_t BYTE1, BYTE2, BYTE3, BYTE4, BYTE5, BYTE6, BYTE7, BYTE8, BYTE9, BYTE10;
extern uint8_t inInts[40], data[9];   // an array to hold incoming data, not seen any longer than 34 bytes, or 9
extern uint16_t a16bitvar;
extern float  eresultf; //Cellv1, Cellv2, Cellv3, Cellv4, Cellv5, Cellv6, Cellv7, Cellv8,
extern float CellMin, CellMax, Cellsum, Celldiff;

// Broadcast cache
extern uint16_t voltages[14];
extern uint16_t voltagesMax[14];
extern uint16_t voltagesMin[14];

// Pack Voltage
extern float PackVoltagef;
// CURRENT
extern float PackCurrentf;
//RSOC remaining state of charge
extern int RSOC;

ESP8266WebServer server;
WebSocketsServer webSocket=WebSocketsServer(81);

uint8_t pin_led = 2;
String ssid = "reginald24";
String password = "45acklaneeastsk72be";
/*
char webpage[] PROGMEM = R"=====(
<html>
<head>
</head>
  <script>
    let Socket;
    function init()
    {
      Socket = new WebSocket('ws://'+window.location.hostname+':81/');
      Socket.onmessage = function(event)
      {
        console.log(event.data);
        const str = event.data;
        const arr = str.split(",");
        document.getElementById('cell'+arr[0]).innerHTML = arr[0]+'   '+arr[1]+'V';
      }
    }
  </script>
<body onload="javascript:init()">
  <ul>
    <li>Cell  Voltage(V)</li>
    <li id="cell1"></li>
    <li id="cell2"></li>
    <li id="cell3"></li>  
    <li id="cell4"></li>  
    <li id="cell5"></li>  
    <li id="cell6"></li>
    <li id="cell7"></li>
    <li id="cell8"></li>  
    <li id="cell9"></li>  
    <li id="cell10"></li>  
    <li id="cell11"></li>
    <li id="cell12"></li>
    <li id="cell13"></li>  
    <li id="cell14"></li>  
  </ul>
</body>
</html>
)=====";
*/
void setup()
{
  pinMode(pin_led, OUTPUT);
  WiFi.begin(ssid,password);

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

  //Serial.begin(115200);
  while(WiFi.status()!=WL_CONNECTED)
  {
    MyDebug.print(".");
    delay(500);
  }
  MyDebug.println("");
  MyDebug.print("IP Address: ");
  MyDebug.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", FILE_INDEX_HTML);
    //server.send_P(200, "text/html", webpage);
  });

  server.on("/style.css", []() {
    server.send_P(200, "text/css", FILE_STYLE_CSS);
  });
/*
  server.on("/jquery.js", []() {
    server.send_P(200, "text/javascript", FILE_JQUERY, FILE_JQUERY_SIZE_BYTES);
    //response->addHeader("Content-Encoding", "gzip");
  });
 
  server.on("/echarts.simple.min.js", []() {
    server.send_P(200, "text/javascript", FILE_ECHARTS, FILE_ECHARTS_SIZE_BYTES);
  });
  server.on("/logo.gif", HTTP_GET, []() {
    server.send_P(200, "image/gif", FILE_LOGO, FILE_LOGO_SIZE_BYTES);
  });
*/

  server.begin();
  webSocket.begin();
}

unsigned long messageTimestamp = 0;
void loop()
{
  webSocket.loop();
  server.handleClient();

  uint64_t now = millis();
  if(now - messageTimestamp > 2000) 
  {
    messageTimestamp = now;

    write_request_start(); 
    call_Basic_info();
    call_get_cells_v();
    write_request_end();

    storeCellVoltageInfo();

    // send packvoltage, packcurrent and remaining state of charge(SOC).
    webSocket.broadcastTXT(
      "info,"+
      String(PackVoltagef, 2)+","+
      String(PackCurrentf, 2)+","+
      String(RSOC)+","+
      String(Cellsum, 3)+","+
      String(CellMax, 3)+","+
      String(CellMin, 3)+","+
      String(Celldiff,0)
    );
    
    float thisCell, thisCellMax, thisCellMin;
    for (int cell=0; cell<14; cell++)
    {
      thisCell = voltages[cell] / 1000.0f; // convert to float
      thisCellMax = voltagesMax[cell] / 1000.0f;
      thisCellMin = voltagesMin[cell] / 1000.0f;
      webSocket.broadcastTXT("cell,"+
                              String(cell)+","+
                              String(thisCell, 2)+","+
                              String(thisCellMax, 2)+","+
                              String(thisCellMin, 2)
                            );
    
    }

    // tidy up
    Cellsum = 0;
    CellMin = 5;
    CellMax = 0;
    Length = 0;
    // new line, what ever happens
    MyDebug.println("");
    MyDebug.println("");
  }
}
