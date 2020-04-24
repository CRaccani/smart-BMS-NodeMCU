
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "BMSUtils.h"


ESP8266WebServer server;
WebSocketsServer webSocket=WebSocketsServer(81);
uint8_t pin_led = 2;
String ssid = "reginald24";
String password = "45acklaneeastsk72be";

String Voltages[14] = {"3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", }; 

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
       // console.log("ED:"+event.data);
        const str = event.data;
        const arr = str.split(",");
        document.getElementById('cell'+arr[0]).innerHTML = 'Cell: '+arr[0]+' '+arr[1]+'V';
      }
    }
  </script>
<body onload="javascript:init()">
  <ul>
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

void setup()
{
  pinMode(pin_led, OUTPUT);
  WiFi.begin(ssid,password);
  Serial.begin(115200);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", webpage);
  });
  
  server.begin();
  webSocket.begin();
}

unsigned long messageTimestamp = 0;
void loop()
{
  webSocket.loop();
  server.handleClient();

  uint64_t now = millis();
  if(now - messageTimestamp > 2000) {
    messageTimestamp = now;
    for (int cell=0; cell<14; cell++)
    {
      webSocket.broadcastTXT(String(cell+1)+","+Voltages[cell]);
    
    }
  }
}
