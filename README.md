# smart-BMS-NodeMCU
The purpose of this project is to offer an alternative method of accesing features of this Generic Chinese smart BMS. In addition to the existing Bluetooth and USB methods.

BMS data is read by the NodeMCU directly via a short wired serial connection and then with ESPAsyncWebServer allow the BMS data to be viewed in a Browser anywhere there is a WiFi connection.

This method also has the potential to offer the fullest funtionality of the BMS. At the moment this is only offered by the PC application JBDTools V1.1 when connected via USB. Whereas the Bluetooth mobile apps offer far less functionality of the BMS.

The BMS will be part of a Powerwall installation where monitoring of the battery is very important. The potential is much greater with the NodeMCU as it will be possible to controll other devices, such as cut-off relays, using MQTT for example.

A lot of hard work by others have brought about the decoding of 99% of the protocol used by this BMS. This protocol is in fact used by many Generic Chinese BMSs'. As a result this project may benefit those who may have these other generic Chinese devices. 

There are many promising similar projects here on GitHub and these have offered many insights and are a mine of amazing information. This is a testament to the openness of the GitHub community.

Proof of concept/Testing program I have found most usefull is by Bress55:
Original code by bress55
23/09/2018
Using Arduino Mega256
https://github.com/bres55/Smart-BMS-arduino-Reader/blob/master/README.md

I have taken the code and refactored and updated it to use NodeMCU ESP8266.
Moved to PlatformIO from Arduino IDE.

The test program main.cpp, is the first stage of this project. It has proved to be successful.
The program will continuously request and read the BMS data and print to the console the decoded information.

Below are some photos of a few tangible items used in this project.



smart BMS by lithiumbatterypcb.com:

![smart BMS by lithiumbatterypcb.com.](/images/BMS.jpg)

Bluetooth Module top view:

![Bluetooth Module top view.](/images/BluetoothModuleTOP.jpg)

Bluetooth Module bottom view:

![Bluetooth Module bottom view.](/images/BluetoothModuleBOTTOM.jpg)

BMS conection, showing output pins:

![BMS conection, showing output pins.](/images/BMSPinOuts.jpg)

USB connection module CP2102:

![USB connection module CP2102.](/images/USBtoTTLModule.jpg)

NodeMCU wiring:

![NodeMCU wiring.](/images/NodeMCU.jpg)



JBDTools V1.1 Pack Info screen:



![JBDTools V1.1 Pack Info screen.](/images/PackInfo.png)



JBDTools V1.1 Settings screen:



![JBDTools V1.1 Settings screen.](/images/Settings.png)



JBDTools V1.1 Calibration screen:



![JBDTools V1.1 Calibration screen.](/images/Calibration.png)



JBDTools V1.1 OtherFunction screen:



![JBDTools V1.1 OtherFunction screen.](/images/OtherFunction.png)

