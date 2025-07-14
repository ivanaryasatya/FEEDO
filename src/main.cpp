
/*

  Feedo - ESP8266 IoT Smart Feeder



  Feedo is an ESP8266-based smart feeder system that can be controlled via WiFi,
  Firebase, and a local webserver. This system is capable of automatic feeding
  using a servo, receiving commands from a mobile app (Firebase), and provides
  a web-based control panel for WiFi configuration and manual command execution.
  Additional features include sensor reading (potentiometer, tilt sensor, button),
  buzzer notifications, OTA updates, and configuration storage in EEPROM. Feedo is
  designed for flexible use both online (cloud) and offline (local AP/webserver).



  Sketch created by: Ivan Aryasatya
  Webserver IP: http://192.168.4.1/
  Site: https://feedo.fardhan.com/
  Version: 1.2.0

*/
#include <Arduino.h>
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <f:\ivan\Arduino\libraries\2820675-bbe995aa22826a8fbbb6b56ccd56513f9db6cb00\pitches.h>
#include <ArduinoOTA.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <ESP8266WebServer.h>


#define API_KEY "API_KEY_here" // API key is in firebase_secret.cpp
#define DATABASE_URL "DATABASE_URL_here" // Database URL is in firebase_secret.cpp

#define ledPin D3
#define servoPin D5
#define buttonPin D2
#define buzzerPin D1
#define tiltSensorPin D4
#define REST 0
#define EEPROM_SIZE 512
#define maxLength 30
char buffer[maxLength];

Servo myServo;
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
ESP8266WebServer server(80); // Port HTTP

byte
  potPin = A0,
  lastPos = 0,
  sampleSize = 30,
  currentBuzzerTone = 6,
  loopCount = 0,
  currentPos = 0,
  lastLastPos = 0,
  servoMAAddress = 0,          // addr 0 - 4   (5 byte)
  servoMIAAddress = 5,         // addr 5 - 10  (6 byte)
  servoODAddress = 11,         // addr 11 - 16 (6 byte)
  servoCDAddress = 17,         // addr 17 - 22 (6 byte)
  ssidAddress = 23,            // addr 23 - 54 (32 byte)
  passwordAddress = 55,        // addr 55 - 86 (32 byte)
  enableServoAddr = 87,        // addr 87 - 88 (2 byte)
  enableBuzzerAddr = 91,       // addr 89 - 90 (2 byte)
  enableLedAddr = 99,          // addr 91 - 92 (2 byte)
  enablePotAddr = 103,         // addr 93 - 94 (2 byte)
  enableButtonAddr = 107,      // addr 95 - 96 (2 byte)
  enableTiltSensorAddr = 111;  // addr 97 - 98 (2 byte)
;

int
  totalTime = 0,
  servoMaxAngle = 110,
  servoMinAngle = 0,
  servoOpenDelay = 300,
  servoCloseDelay = 800;

const unsigned int tones[] = { 300, 400, 500, 600, 700, 800, 900, 1000 };
byte eepromAddress[] = { servoMAAddress, servoMIAAddress, servoODAddress, servoCDAddress, ssidAddress, passwordAddress, enableServoAddr, enableBuzzerAddr, enableLedAddr, enablePotAddr, enableButtonAddr, enableTiltSensorAddr };
float averageHZ = 225;
bool
  systemHasStarted = false,
  signupOK = false,
  serialPrint = false,
  lastSensorState = false,
  fUpdateT1 = true,
  fUpdateT2 = true,
  fUpdateB1 = true,
  fUpdateB2 = true,
  fUpdateP1 = true,
  cooldownK = false,
  bCooldown = false,
  apIsActive = true,
  delayP = true,
  firebaseUpdate = true,
  buzzerAct = false,
  buzzerDelay = true,
  buzzerStart = true,
  buttonKatup = false,
  last = true,
  over = true,
  potChangeLed = true,
  potLedW = true,
  isWifiConnect = false,
  fbdConnected = false,
  lastStart = true,
  enableServo = true,
  enableBuzzer = true,
  enableLed = true,
  enablePot = true,
  enableButton = true,
  lastWifiStatus = false,
  otaIsActive = false,
  codeMarkerPrint = false,
  enableTiltSensor = true,
  webServerIsActive = false,
  wifiHasChanged = false,
  userActivatedWebServer = false;
unsigned long
  bCooldownStart = 0,
  sensorDTime = 0,
  cooldownStartK = 0,
  potPreviousMillis = 0,
  prevLoopRate = 0,
  previousTime = 0,
  prevUpMillis = 0,
  prevLoopRateM = 0,
  currentPosPrint = 0,
  lastReconnectAttempt = 0,
  startAttemptTime = 0,
  timeClientUpdateDelay = 0;
String
  mobileLastUp = "",
  mobileLatestUp = "",
  command = "",
  currentTime = "99:99",
  completeTime = "",
  currentMonthStr,
  monthDayStr,
  wifiSsid = "",
  wifiPassword = "",
  newWifiSsid = "",
  newWifiPassword = "",
  message = "";
const String APSsid = "FEEDO-ESP8266-AP";
const String APPassword = "ikansegar";
const String trueVal = "true";
const String falseVal = "false";
const String wifiHostName = "FEEDO-ESP8266";

String waktusBaru[24];
int katupsBaru[24];
struct CommandData {
  String cmd;
  String command;
  String values[10];  // Maksimal 10 value
  int valueCount;
};
CommandData cmd;

static const char main_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP8266 Control Panel</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f2f2f2;
      padding: 20px;
      max-width: 500px;
      margin: auto;
    }

    a.top-link {
      display: block;
      text-align: center;
      margin-bottom: 20px;
      font-size: 14px;
      text-decoration: none;
      color: #0066cc;
    }

    a.top-link:hover {
      text-decoration: underline;
    }

    h2 {
      color: #333;
      text-align: center;
    }

    .section {
      background: #fff;
      border-radius: 10px;
      padding: 15px 20px;
      margin-bottom: 20px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }

    label {
      display: block;
      margin-top: 10px;
      font-weight: bold;
    }

    input[type="text"],
    input[type="password"] {
      width: 100%;
      padding: 8px;
      margin-top: 5px;
      border: 1px solid #ccc;
      border-radius: 6px;
    }

    textarea {
      width: 100%;
      height: 80px;
      padding: 8px;
      margin-top: 5px;
      border: 1px solid #ccc;
      border-radius: 6px;
      resize: vertical;
    }

    .checkbox-group {
      margin-top: 10px;
    }

    .checkbox-group input {
      margin-right: 10px;
    }

    button {
      margin-top: 15px;
      width: 100%;
      padding: 10px;
      background-color: #4CAF50;
      border: none;
      color: white;
      font-size: 16px;
      border-radius: 6px;
      cursor: pointer;
    }

    button:hover {
      background-color: #45a049;
    }
  </style>
</head>
<body>
  <a class="top-link" href="feedo.fardhan.com" target="_blank">
    🔗 Go to main website (Feedo)
  </a>

  <h2>ESP8266 Control Panel</h2>

  <form action="/action_page" method="POST">
    <div class="section">
      <h3>Input / Output</h3>
      <label for="systemOutput">System Output:</label>
      <div style="display: flex; align-items: center;">
      <textarea id="systemOutput" name="systemOutput" readonly style="margin-right: auto;">Waiting for response...</textarea>
      </div>
      <label for="commandInput">Command Input:</label>
      <input type="text" id="commandInput" name="commandInput" placeholder="Enter command">
    </div>

    <div class="section">
      <h3>WiFi Settings</h3>
      <label for="ssid">WiFi SSID:</label>
      <input type="text" id="ssid" name="ssid" placeholder="My WiFi SSID">

      <label for="password">WiFi Password:</label>
      <input type="password" id="password" name="password" placeholder="My WiFi Password">

      <div class="checkbox-group">
      <input type="checkbox" id="disconnect" name="disconnect">
      <label for="disconnect" style="display: inline;">Disconnect webserver if new WiFi is connected</label>
      </div>
    </div>

    <button type="submit">Apply Settings</button>
  </form>
</body>
</html>
)=====";

// --------------------------------- functions ---------------------------------//

// Function to handle the root URL
// void handleRoot() {
//  String s = MAIN_page; //Read HTML contents
//  server.send(200, "text/html", s); //Send web page
// }

// void handleForm() {
//  String firstName = server.arg("firstname"); 
//  String lastName = server.arg("lastname"); 

//  Serial.print("First Name:");
//  Serial.println(firstName);

//  Serial.print("Last Name:");
//  Serial.println(lastName);
 
//  String s = "<a href='/'> Go Back </a>";
//  server.send(200, "text/html", s); //Send web page
// }
String target;
String command;
std::vector<String> params;


void parseCommand(String input) {
  input.trim();  // Buang spasi depan-belakang
  params.clear();

  // ambil target.command
  int spaceIdx = input.indexOf(' ');
  String head = input.substring(0, spaceIdx);
  String rest = input.substring(spaceIdx + 1);

  int dotIdx = head.indexOf('.');
  target = head.substring(0, dotIdx);
  command = head.substring(dotIdx + 1);

  // buat parameter
  int start = 0;
  while (true) {
    int sp = rest.indexOf(' ', start);
    if (sp == -1) {
      String param = rest.substring(start);
      if (param.length()) params.push_back(param);
      break;
    }
    params.push_back(rest.substring(start, sp));
    start = sp + 1;
  }
}

// Contoh command: led.builtin on 1000
void executeCommand() {
  const String wrongBoolValue = "error: wrong value, must be true or false";
  const String unknownCommand = "unknown input command, please check your command and try again";
  Serial.println("=== EXECUTING COMMAND ===");
  Serial.println("Target : " + target);
  Serial.println("Command: " + command);
  for (int i = 0; i < params.size(); i++) {
    Serial.println("Param[" + String(i) + "]: " + params[i]);
  }

  if (target == "wifi") {  // wifi
    if (command == "rssi") {
      long rssi = WiFi.RSSI();
      commandOutput(String(rssi));
    } else if (command == "localIp") {
      commandOutput(WiFi.localIP().toString());
    } else if (command == "macAddress") {
      commandOutput(WiFi.macAddress());
    } else if (command == "begin") {
      if (params[0].length() >= 30 || params[1].length() >= 30) {
      commandOutput("ssid atau password lebih dari 30 huruf");
      return;
      }
      newWifiSsid = params[0];
      newWifiPassword = params[1];
      wifiHasChanged = true;
      wait(500);
      wifiHandler.startConnect();
    }
  } else if (target == "esp") {  // ESP 8266
    if (command == "restart") {
      commandOutput("restarting board");
      firebaseHandler.setString("command/inputCode", "~");
      ESP.restart();
    } else if (command == "millis") {
      commandOutput(String(millis()));
    } else if (command == "getFreeHeap") {
      commandOutput(String(ESP.getFreeHeap()));
    } else if (command == "getHeapFragmentation") {
      commandOutput(String(ESP.getHeapFragmentation()));
    } else if (command == "getMaxFreeBlockSize") {
      commandOutput(String(ESP.getMaxFreeBlockSize()));
    } else if (command == "getFlashChipSize") {
      commandOutput(String(ESP.getFlashChipSize()));
    } else if (command == "getFlashChipRealSize") {
      commandOutput(String(ESP.getFlashChipRealSize()));
    } else if (command == "getFreeSketchSpace") {
      commandOutput(String(ESP.getFreeSketchSpace()));
    } else if (command == "getSketchSize") {
      commandOutput(String(ESP.getSketchSize()));
    } else if (command == "delay") {
      commandOutput("esp delay start");
      wait(params[0].toInt());
      commandOutput("esp delay ended");
    } 
    
  } else if (target == "servo") {  // Servo
    if (command == "setMinAngle") {
      if (params[0].toInt() <= 180) {
        servoMaxAngle = params[0].toInt();
        accessEEPROM(servoMAAddress, params[0].toInt());
        commandOutput("servo set max angle");
      } else {
        commandOutput("error: angle too large");
      }
    } else if (command == "setOpenDelay") {
      if (params[0].toInt() <= 10000) {
        servoOpenDelay = params[0].toInt();
        accessEEPROM(servoODAddress, servoOpenDelay);
        commandOutput("servo set open delay");
      } else {
        commandOutput("error: delay too long");
      }
    } else if (command == "setCloseDelay") {
      if (params[0].toInt() >= 80) {
        servoCloseDelay = params[0].toInt();
        accessEEPROM(servoCDAddress, params[0].toInt());
        commandOutput("servo set close delay");
      } else {
        commandOutput("error: delay too fast");
      } 
    } else if (command == "katup") {
      servoKatup(params[0].toInt());
      commandOutput("valve finished running");
    } else if (command == "setAngle") {
      myServo.attach(servoPin, 500, 2500);
      myServo.write(params[0].toInt());
      delay(500);
      myServo.detach();
      commandOutput("servo set to current angle");
    } else if (command == "enable") {
      bool enableServo2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableServoAddr, 1);
        enableServo = true;
      } else if (params[0] == trueVal) {
        accessEEPROM(enableServoAddr, 0);
        enableServo = false;
      } else {
        commandOutput("error: wrong value");
        enableServo2 = false;
      }
      if (enableServo2) {
        commandOutput("servo permission updated");
      }
    }
  } else if (target == "buzzer") {  // Buzzer
    if (command == "tone") {
      buzzerT(params[0].toInt());
      commandOutput("buzzer tone complete running")
    } else if (command == "enable") {
      bool enableBuzzer2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableBuzzerAddr, 1);
        enableBuzzer = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableBuzzerAddr, 0);
        enableBuzzer = false;
      } else {
        commandOutput("error: wrong value");
        enableBuzzer2 = false;
      }
      if (enableBuzzer2) {
        commandOutput("buzzer permission updated");
      }

    } 

  } else if (target == "eeprom") {  // Eeprom
    if (command == "get") {
      int intValues = params[0].toInt();
      if (intValues != ssidAddress && intValues != passwordAddress) {
        commandOutput(String(accessEEPROM(params[0].toInt(), -1)));
      } else {
        commandOutput(readStringFromEEPROM(params[0].toInt()));
      }
    } else if (command == "getAll") {
      String getAllResult = getAllEeprom();
      getAllResult.replace("\n", "\\n");
      Serial.println(getAllResult);
      commandOutput(getAllResult);
    } else if (command == "writeString") {
      if (params[0].toInt() == ssidAddress || params[0].toInt() == passwordAddress && params[1].length() <= 30) {
        saveStringToEEPROM(params[0].toInt(), params[1]);
        commandOutput("string data is saved");
      } else {
        commandOutput("error: destination address saves integer");
      }
    } else if (command == "writeInterger") {
      if (params[0].toInt() != ssidAddress && params[0].toInt() != passwordAddress) {
        if (params[1].toInt() <= 99999) {
          accessEEPROM(params[0].toInt(), params[1].toInt());
          commandOutput("interger data is saved");
        } else {
          commandOutput("error: number greater than 99999");
        }
      } else {
        commandOutput("error: destination address saves string");
      }
    } else if (command == "") {

    } else if (command == "") {

    }
  } else if (target == "led") {
    if (command == "state") {  // set led status
      if (params[0] == trueVal) {
        digitalWrite(ledPin, HIGH);
        commandOutput("led true");
      } else if (params[0] == trueVal) {
        digitalWrite(ledPin, LOW);
        commandOutput("led false");
      }
    } else if (command == "effect") { 
      if (params[0] == "fadeIn") {
        ledFadeIn();
        digitalWrite(ledPin, LOW);
        commandOutput("led fadeing in");
      } else if (params[0] == "fadeOut") {
        ledFadeOut();
        commandOutput("led fadeing out");
      }
    } else if (target == "led.enable") {
      bool enableLed2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableLedAddr, 1);
        enableLed = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableLedAddr, 0);
        enableLed = false;
      } else {
        commandOutput("error: wrong value");
        enableLed2 = false;
      }
      if (enableLed2) {
        commandOutput("led permission updated");
      }
    } 
  } else if (target == "pot") {
    if (target == "enable") {
      bool enablePot2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enablePotAddr, 1);
        enablePot = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enablePotAddr, 0);
        enablePot = false;
      } else {
        commandOutput("error: wrong value");
        enablePot2 = false;
      }
      if (enablePot2) {
        commandOutput("pot permission updated");
      }
    }
  } else if (target == "button") { // Button2
    if (target == "button.enable") {  // enable button
      bool enableButton2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableButtonAddr, 1);
        enableButton = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableButtonAddr, 0);
        enableButton = false;
      } else {
        commandOutput("error: wrong value");
        enableButton2 = false;
      }
      if (enableButton2) {
        commandOutput("button permission updated");
      }

    }
  } else if (target == "tiltSensor") {
    if (target == "enable") {  // enable tilt sensor
      bool enableTiltSensor2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableTiltSensorAddr, 1);
        enableTiltSensor = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableTiltSensorAddr, 0);
        enableTiltSensor = false;
      } else {
        commandOutput("error: wrong value");
        enableTiltSensor2 = false;
      }
      if (enableTiltSensor2) {
        commandOutput("servo permission updated");
      }
    }
  } else if (target == "ota") {
    if (target == "run") {
      if (params[0] == trueVal) {
        commandOutput("OTA start");
        otaHandler.start();
      } else if (params[0] == falseVal) {
        otaHandler.end();
        commandOutput("OTA ended");
      } else {
        commandOutput(wrongBoolValue);
      }
    }
    

  } else if (target == "webserver") {
    if (target == "run") {
      if (params[0] == trueVal) {
        webServerIsActive = true;
        commandOutput("webserver started");
      } else if (params[0] == falseVal) {
        webServerIsActive = false;
        commandOutput("webserver ended");
      } else {
        commandOutput(wrongBoolValue);
      }
    }
  } else {
    commandOutput(unknownCommand);
  } 
  firebaseHandler.setString("/command/inputCode", "~");
}


// example OOOOOOOOOO_codeMarker();
void OOOOOOOOOO_codeMarker(byte marker) {
  if (!codeMarkerPrint) {
    return;
  }
  const char *markerStr = "CM";
  const char *markerHighlight = "===";
  Serial.print(markerHighlight);
  Serial.print(markerStr);
  Serial.print(marker);
  Serial.println(markerHighlight);
}

String readStringFromEEPROM(int addr) {
  int i;
  for (i = 0; i < maxLength; i++) {
    buffer[i] = EEPROM.read(addr + i);
    if (buffer[i] == 0) break;
  }
  buffer[i] = '\0';
  Serial.print("Membaca EEPROM [");
  Serial.print(addr);
  Serial.print("] = ");
  Serial.println(String(buffer));
  return String(buffer);
}

// servo
void servoKatup(int perulanganKatup) {
  Serial.println("servo - katup" + perulanganKatup);
  if (perulanganKatup >= 5) {
    return;  
  }
  buzzerT(currentBuzzerTone);
  ifPrint("proses fade in led");  // led fade in
  ledFadeIn();
  if (enableLed) {
    digitalWrite(ledPin, HIGH);
  }

  ifPrintln("servo--katup");
  if (enableServo) {
    myServo.attach(servoPin, 500, 2500);
    for (int i = 0; i < perulanganKatup; i++) {
      ifPrintln("loop katup");
      myServo.write(servoMaxAngle);  // sebelum, 180
      delay(servoOpenDelay);         // sebelum, 400
      myServo.write(servoMinAngle);
      delay(servoCloseDelay);
    }
    delay(200);
    myServo.detach();
  }
  ifPrintln("servo detach");
  ifPrintln("fungsi servo berhenti");
  ifPrint("proses fade out led");
  ledFadeOut();
}

// buzzer
void buzzerT(int bTone) {
  if (!enableBuzzer) {
    return;
  }
  Serial.println("buzzer - indikator");
  if (bTone == 1) {  // pink panther
    ifPrintln("tone 8 (pink panther)");

    int pinkMelody[] = {
      REST, REST, REST, NOTE_DS4,
      NOTE_E4, REST, NOTE_FS4, NOTE_G4, REST, NOTE_DS4,
      NOTE_E4, NOTE_FS4, NOTE_G4, NOTE_C5, NOTE_B4, NOTE_E4, NOTE_G4, NOTE_B4,
      NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_E4, NOTE_D4,
      NOTE_E4, REST, REST, NOTE_DS4,

      NOTE_E4, REST, NOTE_FS4, NOTE_G4, REST, NOTE_DS4,
      NOTE_E4, NOTE_FS4, NOTE_G4, NOTE_C5, NOTE_B4, NOTE_G4, NOTE_B4, NOTE_E5,
      NOTE_DS5,
      NOTE_D5, REST, REST, NOTE_DS4,
      NOTE_E4, REST, NOTE_FS4, NOTE_G4, REST, NOTE_DS4,
      NOTE_E4, NOTE_FS4, NOTE_G4, NOTE_C5, NOTE_B4, NOTE_E4, NOTE_G4, NOTE_B4,

      NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_E4, NOTE_D4,
      NOTE_E4, REST,
      REST, NOTE_E5, NOTE_D5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_E4,
      NOTE_AS4, NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_AS4, NOTE_A4,
      NOTE_G4, NOTE_E4, NOTE_D4, NOTE_E4, NOTE_E4, NOTE_E4
    };
    int pinkDurations[] = {
      2, 4, 8, 8,
      4, 8, 8, 4, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8,
      2, 16, 16, 16, 16,
      2, 4, 8, 4,

      4, 8, 8, 4, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8,
      1,
      2, 4, 8, 8,
      4, 8, 8, 4, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8,

      2, 16, 16, 16, 16,
      4, 4,
      4, 8, 8, 8, 8, 8, 8,
      16, 8, 16, 8, 16, 8, 16, 8,
      16, 16, 16, 16, 16, 2
    };

    playMelody(pinkMelody, pinkDurations, sizeof(pinkMelody) / sizeof(pinkMelody[0]));

  } else if (bTone == 2) {  // star wars
    ifPrintln("tone 2 (start wars)");

    int starWarsMelody[] = {
      NOTE_AS4, NOTE_AS4, NOTE_AS4,
      NOTE_F5, NOTE_C6,
      NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6,
      NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6,
      NOTE_AS5, NOTE_A5, NOTE_AS5, NOTE_G5, NOTE_C5, NOTE_C5, NOTE_C5,
      NOTE_F5, NOTE_C6,
      NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6,

      NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6,
      NOTE_AS5, NOTE_A5, NOTE_AS5, NOTE_G5, NOTE_C5, NOTE_C5,
      NOTE_D5, NOTE_D5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F5,
      NOTE_F5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_D5, NOTE_E5, NOTE_C5, NOTE_C5,
      NOTE_D5, NOTE_D5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F5,

      NOTE_C6, NOTE_G5, NOTE_G5, REST, NOTE_C5,
      NOTE_D5, NOTE_D5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F5,
      NOTE_F5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_D5, NOTE_E5, NOTE_C6, NOTE_C6,
      NOTE_F6, NOTE_DS6, NOTE_CS6, NOTE_C6, NOTE_AS5, NOTE_GS5, NOTE_G5, NOTE_F5,
      NOTE_C6
    };
    int starWarsDurations[] = {
      8, 8, 8,
      2, 2,
      8, 8, 8, 2, 4,
      8, 8, 8, 2, 4,
      8, 8, 8, 2, 8, 8, 8,
      2, 2,
      8, 8, 8, 2, 4,

      8, 8, 8, 2, 4,
      8, 8, 8, 2, 8, 16,
      4, 8, 8, 8, 8, 8,
      8, 8, 8, 4, 8, 4, 8, 16,
      4, 8, 8, 8, 8, 8,

      8, 16, 2, 8, 8,
      4, 8, 8, 8, 8, 8,
      8, 8, 8, 4, 8, 4, 8, 16,
      4, 8, 4, 8, 4, 8, 4, 8,
      1
    };

    playMelody(starWarsMelody, starWarsDurations, sizeof(starWarsMelody) / sizeof(starWarsMelody[0]));
  } else if (bTone == 3) {  // subway surfers
    ifPrintln("tone 3 (subway surfers)");

    int subwayMelody[] = {
      NOTE_C4,
      REST,
      NOTE_G4,
      REST,
      NOTE_AS4,
      NOTE_C5,
      NOTE_AS4,
      REST,
      NOTE_F4,
      NOTE_DS4,
      REST,
      NOTE_C4,
      REST,
      NOTE_G4,
      REST,
      NOTE_AS4,
      NOTE_C5,
      NOTE_AS4,
      REST,
      NOTE_F4,
      NOTE_DS4,
      REST,
      NOTE_C4,
      REST,
      NOTE_G4,
      REST,
      NOTE_AS4,
      NOTE_C5,
      NOTE_AS4,
      REST,
      NOTE_F4,
      NOTE_DS4,
      REST,

      NOTE_C4,
      REST,
      NOTE_E4,
      REST,
      NOTE_G4,
      NOTE_A4,
      NOTE_AS4,
      NOTE_C5,
      REST,
      NOTE_C5,
      REST,
      NOTE_AS4,
      REST,
      NOTE_A4,
      REST,
      NOTE_AS4,
      REST,
      NOTE_AS4,
      NOTE_C5,
      REST,
      NOTE_AS4,
      NOTE_A4,
      REST,
      REST,
      NOTE_C5,
      REST,
      NOTE_AS4,
      REST,
      NOTE_A4,
      REST,
      NOTE_AS4,
      REST,
      NOTE_E5,
      REST,

      NOTE_C5,
      REST,
      NOTE_C5,
      REST,
      NOTE_AS4,
      REST,
      NOTE_A4,
      REST,
      NOTE_AS4,
      REST,
      NOTE_AS4,
      NOTE_C5,
      REST,
      NOTE_AS4,
      NOTE_A4,
      REST,
      REST,
      NOTE_C5,
      REST,
      NOTE_AS4,
      REST,
      NOTE_A4,
      REST,
      NOTE_AS4,
      REST,
      NOTE_E4,
      REST,
    };
    int subwayDurations[] = {
      4, 8, 4, 8, 4, 8, 8, 16, 8, 8, 16,
      4, 8, 4, 8, 4, 8, 8, 16, 8, 8, 16,
      4, 8, 4, 8, 4, 8, 8, 16, 8, 8, 16,

      4, 8, 4, 8, 4, 4, 4,
      8, 16, 8, 16, 8, 16, 8, 16,
      8, 16, 8, 8, 16, 8, 8, 16,
      4,
      8, 16, 8, 16, 8, 16, 8, 4, 8,
      4,

      8, 16, 8, 16, 8, 16, 8, 16,
      8, 16, 8, 8, 16, 8, 8, 16,
      4,
      8, 16, 8, 16, 8, 16, 8, 4, 8,
      1
    };

    playMelody(subwayMelody, subwayDurations, sizeof(subwayMelody) / sizeof(subwayMelody[0]));
  } else if (bTone == 4) {  // harry potter
    ifPrintln("tone 4 (harry potter)");

    int harryPotterMelody[] = {
      REST, NOTE_D4,
      NOTE_G4, NOTE_AS4, NOTE_A4,
      NOTE_G4, NOTE_D5,
      NOTE_C5,
      NOTE_A4,
      NOTE_G4, NOTE_AS4, NOTE_A4,
      NOTE_F4, NOTE_GS4,
      NOTE_D4,
      NOTE_D4,

      NOTE_G4, NOTE_AS4, NOTE_A4,
      NOTE_G4, NOTE_D5,
      NOTE_F5, NOTE_E5,
      NOTE_DS5, NOTE_B4,
      NOTE_DS5, NOTE_D5, NOTE_CS5,
      NOTE_CS4, NOTE_B4,
      NOTE_G4,
      NOTE_AS4,

      NOTE_D5, NOTE_AS4,
      NOTE_D5, NOTE_AS4,
      NOTE_DS5, NOTE_D5,
      NOTE_CS5, NOTE_A4,
      NOTE_AS4, NOTE_D5, NOTE_CS5,
      NOTE_CS4, NOTE_D4,
      NOTE_D5,
      REST, NOTE_AS4,

      NOTE_D5, NOTE_AS4,
      NOTE_D5, NOTE_AS4,
      NOTE_F5, NOTE_E5,
      NOTE_DS5, NOTE_B4,
      NOTE_DS5, NOTE_D5, NOTE_CS5,
      NOTE_CS4, NOTE_AS4,
      NOTE_G4
    };
    int harryPotterDurations[] = {
      2, 4,
      4, 8, 4,
      2, 4,
      2,
      2,
      4, 8, 4,
      2, 4,
      1,
      4,

      4, 8, 4,
      2, 4,
      2, 4,
      2, 4,
      4, 8, 4,
      2, 4,
      1,
      4,

      2, 4,
      2, 4,
      2, 4,
      2, 4,
      4, 8, 4,
      2, 4,
      1,
      4, 4,

      2, 4,
      2, 4,
      2, 4,
      2, 4,
      4, 8, 4,
      2, 4,
      1
    };

    playMelody(harryPotterMelody, harryPotterDurations, sizeof(harryPotterMelody) / sizeof(harryPotterMelody[0]));
  } else if (bTone == 5) {  // the small world
    ifPrintln("tone 5 (the small world)");

    int theMelody[] = {
      NOTE_E4, NOTE_F4, NOTE_G4, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_B4, NOTE_B4, NOTE_D4,
      NOTE_E4, NOTE_F4, NOTE_D5, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_E4, NOTE_F4,
      NOTE_G4, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_A4, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_E5,
      NOTE_D5, NOTE_G4, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_C5, REST, NOTE_C5, REST, NOTE_C5, NOTE_E5,
      NOTE_C5, NOTE_D5, REST, NOTE_D5, NOTE_D5, NOTE_D5, REST, NOTE_D5, NOTE_F5, NOTE_D5, NOTE_E5, REST, NOTE_E5,
      NOTE_E5, NOTE_E5, REST, NOTE_E5, NOTE_G5, NOTE_E5, NOTE_F5, REST, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_G4,
      NOTE_B4, NOTE_C5, NOTE_C5, REST
    };
    int theDurations[] = {
      8, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 4, 8, 8, 4, 8, 8, 4,
      8, 8, 4, 8, 8, 4, 4, 4, 4, 2, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 2, 4, 8, 8, 4, 4, 4, 8, 8,
      2, 4, 8, 8, 4, 4, 4, 8, 8, 4, 8, 8, 2, 2, 2, 4, 4
    };

    playMelody(theMelody, theDurations, sizeof(theMelody) / sizeof(theMelody[0]));
  } else if (bTone == 6) {  // pac man
    ifPrintln("tone 6 (pac man)");

    int pacManMelody[] = {
      NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5,
      NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5,
      NOTE_C6, NOTE_G6, NOTE_E6, NOTE_C6, NOTE_G6, NOTE_E6,

      NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5,
      NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5,
      NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5
    };
    int pacManDurations[] = {
      16, 16, 16, 16,
      32, 16, 8, 16,
      16, 16, 16, 32, 16, 8,

      16, 16, 16, 16, 32,
      16, 8, 32, 32, 32,
      32, 32, 32, 32, 32, 16, 8
    };

    playMelody(pacManMelody, pacManDurations, sizeof(pacManMelody) / sizeof(pacManMelody[0]));
  } else if (bTone == 7) {  // mulai sistem
    ifPrintln("tone 7 (memulai sistem)");
    tone(buzzerPin, 680);
    delay(200);
    tone(buzzerPin, 700);
    delay(200);
    tone(buzzerPin, 1000);
    delay(200);
    noTone(buzzerPin);
  } else if (bTone == 8) {  // peringatan
    ifPrintln("tone 8 (peringatan)");
    for (int i = 0; i < 5; i++) {
      tone(buzzerPin, 1000);
      delay(200);
      tone(buzzerPin, 800);
      delay(200);
      noTone(buzzerPin);
    }
  } else if (bTone == 9) {
  } else if (bTone == 0) {
  }

  buzzerAct = false;
  //(buzzerDelay) ? delay(1000) : delay(0);
  if (buzzerDelay) delay(1000);
  ifPrintln("fungsi buzzer berakhir");
}

// potentiometer
void potentiometer(String currentTime, String completeTime) {
  if (!enablePot) {
    return;
  }
  int potValue = analogRead(potPin);
  static unsigned long lastTime = 0;

  if (serialPrint) {
    ("potentiometer: " + String(potValue));
  }


  if (potValue < 170) {
    ifPrintln("pot value: 0");
    currentPos = 0;
  } else if (potValue < 340) {
    ifPrintln("pot value: 1");
    currentPos = 1;
  } else if (potValue < 510) {
    ifPrintln("pot value: 2");
    currentPos = 2;
  } else if (potValue < 680) {
    ifPrintln("pot value: 3");
    currentPos = 3;
  } else if (potValue < 850) {
    ifPrintln("pot value: 4");
    currentPos = 4;
  } else {
    ifPrintln("pot value: 5");
    currentPos = 5;
  }

  if (currentPos != lastPos) {  // posisi berubah
    lastPos = currentPos;
    lastTime = millis();
    potChangeLed = true;
    if (millis() - currentPosPrint >= 100) {
      Serial.print("current pos ");
      Serial.println(currentPos);
      currentPosPrint = millis();
    }

    if (currentPos != 0) {
      if (enableLed) {
        digitalWrite(ledPin, HIGH);
        delay(100);
      }
    }

    lastLastPos = currentPos;
    potChangeLed = false;

    fUpdateP1 = true;
    potLedW = true;
  } else {
    if (potLedW && enableLed) {
      digitalWrite(ledPin, LOW);
      potLedW = false;
    }
  }

  if (currentPos == 0) {
    over = true;
    lastPos = 0;
  }

  if (millis() - lastTime >= 5000) {
    ifPrint("pot pos: ");
    if (serialPrint) { Serial.println(lastPos); }
    if (fUpdateP1) {
      if (Firebase.RTDB.setInt(&fbdo, "/potValue", lastPos)) {
        ifPrintln("fb set pot value");
      } else {
        fbdoError();
      }
      fUpdateP1 = false;
    }

    if (over && lastPos != 0) {
      delayP = false;
      Serial.println("servo pot");
      servoKatup(lastPos);
      potPreviousMillis = millis();
      lastFood("pot", lastPos, completeTime);
      over = false;
    }
  }
}

// tilt sensor
void tiltSensor(String completeTime) {
  if (!enableTiltSensor) {
    return;
  }
  bool sensorState = digitalRead(tiltSensorPin) == LOW;
  if (sensorState && !lastSensorState) {
    sensorDTime = millis();
  }
  if (sensorState && (millis() - sensorDTime >= 5000)) {
    ifPrintln("tilt sensor true");

    if (fUpdateT1 && fbdConnected) {
      if (Firebase.RTDB.setBool(&fbdo, "/tiltSensor/isTiltSensor", true)) {
        ifPrintln("fb set tilt sensor true");
      } else {
        fbdoError();
      }
      if (Firebase.RTDB.setString(&fbdo, "/tiltSensor/waktu", completeTime)) {
        ifPrintln("fb set tiltSensor time");
      } else {
        fbdoError();
      }
      fUpdateT1 = false;
      fUpdateT2 = true;
    }
  } else if (!sensorState) {
    ifPrintln("tilt sensor false");

    if (fUpdateT2 && fbdConnected) {
      if (Firebase.RTDB.setBool(&fbdo, "/tiltSensor/isTiltSensor", false)) {
        ifPrintln("fb set tilt sensor false");
        fUpdateT1 = true;
        fUpdateT2 = false;
      } else {
        fbdoError();
      }
    }
  }
  lastSensorState = sensorState;
}

// utton
void button(String completeTime) {
  if (!enableButton) {
    return;
  }
  if (!bCooldown && digitalRead(buttonPin) == LOW) {
    bCooldown = true;
    bCooldownStart = millis();
    Serial.println("button true");

    if (fUpdateB1) {
      fUpdateB1 = false;
      fUpdateB2 = true;
      buttonKatup = true;
    }
  } else {
    if (fUpdateB2) {
      Serial.println("button false");
      fUpdateB1 = true;
      fUpdateB2 = false;
      if (buttonKatup) {
        Serial.println("servo button");
        servoKatup(1);
        lastFood("btn", 1, completeTime);
        buttonKatup = false;
      }
    }
  }
  if (bCooldown && (millis() - bCooldownStart >= 60000)) {
    bCooldown = false;
  }
}

// fadein led
void ledFadeIn() {
  if (!enableLed) {
    return;
  }
  ifPrint("proses fade in led");
  for (int brightness = 0; brightness <= 225; brightness += 5) {
    analogWrite(ledPin, brightness);
    ifPrint(".");
    delay(10);
  }
  ifPrintln(".");
}

// fadeout led
void ledFadeOut() {
  if (!enableLed) {
    return;
  }
  ifPrint("proses fade out led");
  for (int brightness = 225; brightness >= 0; brightness -= 5) {
    analogWrite(ledPin, brightness);
    ifPrint(".");
    delay(10);
  }
  ifPrintln(".");
  digitalWrite(ledPin, LOW);
}

/*
// loop rate
void loopRate() {
  unsigned long loopDuration = millis() - previousTime;
  previousTime = millis();
  totalTime += loopDuration;
  loopCount++;
  if (loopCount >= sampleSize) {
    float averageTime = (float)totalTime / sampleSize;
    if (serialPrint) {
      Serial.print("loop rate: ");
      Serial.print(averageTime);
      Serial.println(" ms");
    }
    if (millis() - prevLoopRateM >= 3000) {
      if (Firebase.RTDB.setInt(&fbdo, "/loopRate", averageTime)) {
        ifPrintln("fb set loop rate");
      } else {
        fbdoError();
      }
      prevLoopRateM = millis();
    }

    totalTime = 0;
    loopCount = 0;
  }
}
*/

// loop rate
float loopRate() {
  static unsigned long lastMillis = 0;
  static int loopCounter = 0;
  static int totalLoops = 0;
  float rataHz;
  byte intervalDetik = 5;
  loopCounter++;
  totalLoops++;

  if (millis() - lastMillis >= intervalDetik * 1000) {
    float rataHz = (float)totalLoops / intervalDetik;

    if (serialPrint) {
      Serial.print("Loop rate ");
      Serial.print(rataHz, 2);
    }
    lastMillis = millis();
    totalLoops = 0;
    averageHZ = rataHz;
  }
  return rataHz;
}

void playMelody(int melody[], int durations[], int size) {
  for (int note = 0; note < size; note++) {
    if (melody[note] != REST) {
      int duration = 1000 / durations[note];
      tone(buzzerPin, melody[note], duration);
      int pauseBetweenNotes = duration * 1.30;
      delay(pauseBetweenNotes);
      noTone(buzzerPin);
    } else {
      // Jika note adalah REST, hanya beri jeda
      int duration = 1000 / durations[note];
      delay(duration * 1.30);
    }
  }
}

// println
void ifPrintln(String textln) {
  if (serialPrint) {
    Serial.println(String(textln));
  }
}

// print
void ifPrint(String text) {
  if (serialPrint) {
    Serial.print(String(text));
  }
}

// print
void wait(unsigned long time) {
  unsigned long start = millis();
  while (millis() - start < time) {
    yield();
  }
}

// webserver handler
// HTTP example: http://192.168.4.1/ESP?command=<feedo.feeding.2>
// command: <target.command.parameter1,parameter2,...>
// command example: <ESP.moveServo.start=0,end=120>
// command example: <feedo.feeding.2>
struct WebServer {
  // true = memulai webserver, menayalakn ap mode, memulai akan otomastis hanya sekali dipanggil
  // false = menghentikan webserver, menonaktifkan ap mode, menghentikan akan otomastis hanya sekali dipanggil
  void handle(bool inF_webServerIsActive) {
    static bool hasStarted = false;
    static bool webServerHasStoped = true;
  
    if (inF_webServerIsActive && !hasStarted) {
      webServerHasStoped = false;
      webServerIsActive = true;
      WiFi.softAP(APSsid, APPassword);
      apIsActive = true;
      printInfo();
      server.on("/", handleRoot);        
      server.on("/action_page", handleForm);

      server.on("/pesan", []() {
        if (server.hasArg("command")) {
          String messages = server.arg("isi");
          Serial.println("Pesan diterima: " + messages);
          server.send(200, "text/plain", "Pesan diterima: " + messages);
        } else {
          server.send(400, "text/plain", "Parameter 'isi' tidak ditemukan");
        }
      });
      server.begin();
      hasStarted = true;
      
    } else if (!inF_webServerIsActive && !webServerHasStoped && isWifiConnect) {
      if (WiFi.softAPgetStationNum() <= 0) {
        server.stop();
        webServerHasStoped = true;
        Serial.println("Webserver ended");
        WiFi.softAPdisconnect(true);
        apIsActive = hasStarted = webServerIsActive = false;
      }
    }
    if (!isWifiConnect && !webServerIsActive) Serial.println("Wi-Fi disconnected! Enabling AP & Webserver..."), handle(true);

    if (isWifiConnect && webServerIsActive) {
      static unsigned long apStartTime = 0;
      static bool apTimerStarted = false;
      if (WiFi.softAPgetStationNum() > 0) {
        apTimerStarted = true;
        apStartTime = millis();
      } else {
        if (apTimerStarted && millis() - apStartTime >= 180000) {
          Serial.println("No AP clients for 3 minutes, disabling AP and webserver...");
          handle(false);
          apTimerStarted = false;
          apStartTime = millis();
        }
      }
    }
    if (webServerIsActive) server.handleClient();
  }

  bool sendStr (int code, String message) {
    handle(true);
    if (webServerIsActive) {
      server.send(code, "text/plain", message);
      return true;
    } else {
      Serial.println("failed webserver, server not active");
      return false;
    }
  }

  static void handleForm() {
    String newWifiSsid_debug = server.arg("lastname");
    String newWifiPassword_debug = server.arg("password");
    String command_debug = server.arg("commandInput");
    bool reconnectNewWifi_debug = server.hasArg("disconnect");
    Serial.println("New Wi-Fi SSID: " + newWifiSsid_debug);
    Serial.println("New Wi-Fi Password: " + newWifiPassword_debug);
    Serial.println("Command: " + command_debug);
    Serial.println("Reconnect to new Wi-Fi: " + String(reconnectNewWifi_debug ? trueVal : falseVal));
  }

  static void handleRoot() {
    String s = main_page;
    server.send(200, "text/html", s);
  }

  void printInfo() {
    Serial.print("AP status: ");
    Serial.println(apIsActive);
    Serial.print("SSID: ");
    Serial.println(APSsid);
    Serial.print("password: ");
    Serial.println(APPassword);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }
};
WebServer webServer = WebServer();
// wifi handler
struct WifiHandler {

  // mulai koneksi dengan begin()
  void startConnect() {
    static bool handlerWifiStatus = false;
    static bool wifiHasSetToApSta = false;
    unsigned long handleTimeout = millis();
    if (!wifiHasSetToApSta) {
      WiFi.mode(WIFI_AP_STA);
      wifiHasSetToApSta = true;
    }
    WiFi.hostname(wifiHostName);
    if (wifiHasChanged) {
      WiFi.begin(newWifiSsid, newWifiPassword);
    } else {
      WiFi.begin(wifiSsid, wifiPassword);
    }
    OOOOOOOOOO_codeMarker(19);
  
    while (WiFi.status() != WL_CONNECTED && millis() - handleTimeout < 15000) {
      OOOOOOOOOO_codeMarker(14);
      Serial.print(".");
      delay(200);
    }
    OOOOOOOOOO_codeMarker(15);
    Serial.println(" ");
    autoCheck();
    
  };

  // auto check connect ulang jika wifi terputus
  void autoCheck() {
    static bool onceWifiStatusTask = true;
    
    if (WiFi.status() == WL_CONNECTED) { // koneksi berhasil
      isWifiConnect = true;
      OOOOOOOOOO_codeMarker(16);
      if (onceWifiStatusTask) {
        digitalWrite(LED_BUILTIN, LOW);
        wait(100);
        digitalWrite(LED_BUILTIN, HIGH);
        printInfo(1);
        onceWifiStatusTask = false;
      }
      if (wifiHasChanged) {
        saveStringToEEPROM(ssidAddress, newWifiSsid);
        saveStringToEEPROM(passwordAddress, newWifiPassword);
        wifiSsid = newWifiSsid;
        wifiPassword = newWifiPassword;
        newWifiSsid = "";
        newWifiPassword = "";
        commandOutput("New wifi connected");
        wifiHasChanged = false;
      }
    } else { //---------------------------- koneksi gagal
      OOOOOOOOOO_codeMarker(17);
      isWifiConnect = false;
      onceWifiStatusTask = true;
      static unsigned long lastReconnect = 0;
      if (!otaIsActive && millis() - lastReconnect > 60000 && WiFi.status() != WL_CONNECTED) {
        if (WiFi.softAPgetStationNum() <= 0) {
          OOOOOOOOOO_codeMarker(18);
          Serial.println("Trying to reconnect to Wi-Fi...");
          lastReconnect = millis();
          startConnect();
        } else {
          Serial.println("Cannot attempt reconnect, there is a device connected in AP mode");
          lastReconnect = millis();
        }
        
      }
      if (wifiHasChanged) {
        commandOutput("failed to connect to new WiFi");
      }
    }
  }

  // ke AP mode
  void apMode(bool startAP) {
    
    if (startAP) {
      if (apIsActive) {
        Serial.println("AP mode is already active.");
        return;
      }
      WiFi.softAP(APSsid, APPassword);
      apIsActive = true;
      Serial.println("AP mode activated.");
    } else {
      WiFi.softAPdisconnect(true);
      apIsActive = false;
      Serial.println("AP mode deactivated.");
    }
  }
  // AP STA mode = 0
  // STA mode = 1
  // AP mode = 2
  void printInfo(byte mode) {
    if (mode == 1 || mode == 0) {
      Serial.print("wifi status: ");
      Serial.println(isWifiConnect);
      Serial.print("SSID: ");
      Serial.println(wifiSsid);
      Serial.print("password: ");
      Serial.println(wifiPassword);
      Serial.print("wifi IP: ");
      Serial.println(WiFi.localIP());
    }
    if (mode == 2 || mode == 0) {
      Serial.print("AP status: ");
      Serial.println(apIsActive);
      Serial.print("SSID: ");
      Serial.println(APSsid);
      Serial.print("password: ");
      Serial.println(APPassword);
      Serial.print("AP IP: ");
      Serial.println(WiFi.softAPIP());
    }
  }
    
};
WifiHandler wifiHandler;

/*
  if (WiFi.status() == WL_CONNECTED) {
    isWifiConnect = false;
    if (lastReconnectAttempt == 0 || millis() - lastReconnectAttempt >= 60000) {
      Serial.println("Mencoba menghubungkan ke WiFi...");
      lastReconnectAttempt = millis();
      WiFi.disconnect();
      WiFi.begin(wifi.c_str(), password.c_str());
      startAttemptTime = millis();

      Serial.println("\nconnecting to Wi-Fi");
      Serial.print("ssid: ");
      Serial.println(wifiSsid);
      Serial.print("password: ");
      Serial.println(wifiPassword);

      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        Serial.print(".");
        delay(500);
      }

      if (WiFi.status() != WL_CONNECTED) {
        Serial.print("connected with IP: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("failed to connect wifi, STA mode on");
        WiFi.mode(WIFI_STA);
      }
    }
  } else {
    isWifiConnect = true;
  }



// koneksi ke wifi
void wifiConnection(String ssid, String password) {
  int timeout = 20;
  Serial.println("\nconnecting to Wi-Fi");
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("password: ");
  Serial.println(password);
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    if (serialPrint) {
      Serial.print(".");
    }
    wait(500);
    timeout--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    
  } else {
    Serial.println("failed to connect wifi");
    
  }
}
*/

//pengolahan teks
CommandData parseCommand(String input) {

  CommandData result;
  result.valueCount = 0;
  int firstSpace = input.indexOf(' ');
  if (firstSpace == -1) {
    result.command = input;
    return result;
  }
  result.command = input.substring(0, firstSpace);
  String valuesString = input.substring(firstSpace + 1);
  int spaceIndex;
  while (result.valueCount < 10 && valuesString.length() > 0) {
    spaceIndex = valuesString.indexOf(' ');

    if (spaceIndex == -1) {
      result.values[result.valueCount] = valuesString;
      result.valueCount++;
      break;
    }

    result.values[result.valueCount] = valuesString.substring(0, spaceIndex);
    result.valueCount++;
    valuesString = valuesString.substring(spaceIndex + 1);
  }
  return result;
}

// akses ke eeprom interger
int accessEEPROM(int address, int value) {
  if (address < 0 || address >= EEPROM_SIZE) {
    Serial.println("Alamat di luar batas EEPROM!");
    return -1;
  }

  if (value == -1) {  // Mode membaca jika tidak ada nilai yang diberikan
    int storedValue;
    EEPROM.get(address, storedValue);
    Serial.print("Membaca EEPROM [");
    Serial.print(address);
    Serial.print("] = ");
    Serial.println(storedValue);
    return storedValue;
  } else {  // Mode menulis jika ada nilai yang diberikan
    EEPROM.put(address, value);
    EEPROM.commit();
    Serial.print("Menulis EEPROM [");
    Serial.print(address);
    Serial.print("] = ");
    Serial.println(value);
    return value;
  }
  delay(800);
}

// error firebase
void fbdoError() {
  static byte errorCount = 0;
  errorCount++;
  Serial.println(fbdo.errorReason());
  if (errorCount >= 150) {
    Serial.println("error, restart sistem");
    ESP.restart();
  }
}

// handler firebase
struct FirebaseHandler {
  FirebaseData* fbdo_s;
  
  bool start() { 
    static bool start = true;
    if (start == true && isWifiConnect) {
      OOOOOOOOOO_codeMarker(11);
      Serial.print("isWifiConnect: ");
      Serial.println(isWifiConnect);
      if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("firebase signup berhasil");
        OOOOOOOOOO_codeMarker(12);
        config.token_status_callback = tokenStatusCallback;
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        signupOK = true;
      } else {
        OOOOOOOOOO_codeMarker(13);
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
        signupOK = false;
      }
      start = false;
    }
    return signupOK;
  }
  bool setInt(const String& path, int value) {
    if (!isWifiConnect) return false;
    if (Firebase.RTDB.setInt(fbdo_s, path, value)) {
      return true;
    } else {
      return false;
      fbdoError();
    }
  }
  bool setFloat(const String& path, float value) {
    if (!isWifiConnect) return false;
    if (Firebase.RTDB.setFloat(fbdo_s, path, value)) {
      return true;
    } else {
      return false;
      fbdoError();
    }
  }
  bool setBool(const String& path, bool value) {
    if (!isWifiConnect) return false;
    if (Firebase.RTDB.setBool(fbdo_s, path, value)) {
      return true;
    } else {
      return false;
      fbdoError();
    }
  }
  bool setString(const String& path, const String& value) {
    if (!isWifiConnect) return false;
    if (Firebase.RTDB.setString(fbdo_s, path, value)) {
      return true;
    } else {
      return false;
      fbdoError();
    }
  }
  int getInt(const String& path) {
    if (!isWifiConnect) return -1;
    if (Firebase.RTDB.getInt(fbdo_s, path)) {
      return fbdo_s->intData();
    } else {
      fbdoError();
      return -1;
    }
  }
  float getFloat(const String& path) {
    if (!isWifiConnect) return 0.0;
    if (Firebase.RTDB.getFloat(fbdo_s, path)) {
      return fbdo_s->floatData();
    } else {
      fbdoError();
      return 0.0;
    }
  }
  bool getBool(const String& path) {
    if (!isWifiConnect) return false;
    if (Firebase.RTDB.getBool(fbdo_s, path)) {
      return fbdo_s->boolData();
    } else {
      fbdoError();
      return false;
    }
  }
  String getString(const String& path) {
    if (!isWifiConnect) return "";
    if (Firebase.RTDB.getString(fbdo_s, path)) {
      return fbdo_s->stringData();
    } else {
      fbdoError();
      return "";
    }
  }
};
FirebaseHandler firebaseHandler = { &fbdo };

// menyimpan data terakhir makanan
void lastFood(String source, int value, String currentTime) {
  // Example: source = pot / btn, value = 0-3
  // Example: |pot,3,2025-02-03 12:30:00
  const byte maxDataLimit = 50;
  const byte formatLength = 26;
  const String newData = "|" + source + "," + value + "," + currentTime;
  FirebaseJsonArray fja;
  String final = firebaseHandler.getString("/lastFood");
  final = final + newData;
  if (final.length() > (formatLength * maxDataLimit)) {
    final = final.substring(final.length() - (formatLength * maxDataLimit), final.length());
  }
  firebaseHandler.setString("/lastFood", final);

  /*if (Firebase.RTDB.getString(&fbdo, "/lastFood")) {
    const byte maxDataLimit = 50;
    const byte formatLength = 26;
    const String newData = "|" + source + "," + value + "," + currentTime;

    String lf = fbdo.stringData();
    String final = lf;

    final = final + newData;
    if (final.length() > (formatLength * maxDataLimit)) {
      final = final.substring(final.length() - (formatLength * maxDataLimit), final.length());
    }
    if (Firebase.RTDB.setString(&fbdo, "/lastFood", final)) {
    } else {
      fbdoError();
    }
  }*/
}

// command ouput firebase
void commandOutput(const String oCommand) {
  if (oCommand != "~") {
    if (firebaseHandler.setString("/command/output", oCommand)) {
      Serial.println("command output: " + oCommand);
    }
  }
  
}

void saveStringToEEPROM(int addr, String data) {
  int len = data.length();
  if (len > maxLength) len = maxLength;
  for (int i = 0; i < len; i++) {
    EEPROM.put(addr + i, data[i]);
  }
  EEPROM.write(addr + len, 0);
  EEPROM.commit();
}

// mendapatkan semua data EEPROM
String getAllEeprom() {

  int eepAddArrSize = sizeof(eepromAddress) / sizeof(eepromAddress[0]);
  String result = "";
  String eepReadvalue;
  int storedValue;
  for (int i = 0; i < eepAddArrSize; i++) {
    if (eepromAddress[i] != ssidAddress && eepromAddress[i] != passwordAddress) {
      eepReadvalue = String(accessEEPROM(eepromAddress[i], -1));
    } else {
      eepReadvalue = readStringFromEEPROM(eepromAddress[i]);
    }

    result += "Membaca EEPROM [";
    result += String(eepromAddress[i]);
    result += "] = ";
    result += eepReadvalue;
    result += "\n";
  }
  return result;
}

bool otaHasEnded = true;
bool otaInitialized = false;
bool ledState = false;
unsigned long lastBlink = 0;
// function handle OTA

struct OtaHandler {

  // auto handle saat dinyalakan atau dimatikan
  void autoHandle() {
    if (!otaIsActive) {
      return;
    }
    ArduinoOTA.handle();
    if (millis() - lastBlink >= 500) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
      lastBlink = millis();
    }
    
  }
  
  // start: mulai ota
  void start() {
    if (!otaInitialized) {
      otaHasEnded = false;
      wifiHandler.apMode(true);
      ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else {
          type = "filesystem";
        }
        Serial.println("Start updating " + type);
      });
      ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      });
      ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          Serial.println("End Failed");
        }
      });
      ArduinoOTA.begin();
      Serial.println("OTA is ready");
      wifiHandler.printInfo(2);
      Serial.println(WiFi.softAPIP());
      otaIsActive = otaInitialized = true;
      OOOOOOOOOO_codeMarker(2);
    }
    
  }
  
  // end: hentikan ota
  void end() {
    Serial.println("ended OTA");
    otaIsActive = otaInitialized = false;
    if (!otaHasEnded) {
      digitalWrite(LED_BUILTIN, HIGH);
      ArduinoOTA.end();
      if (!webServerIsActive) { wifiHandler.apMode(false); }
      otaHasEnded = true;
    }
  }

  
};
OtaHandler otaHandler;

struct TimeClientHandler {

  bool start() {
    timeClient.begin();
    timeClient.setTimeOffset(25200);  // Offset untuk Waktu Indonesia Barat (WIB)
    return true;
  }

  String currentTime() {
    timeClient.update();
    return timeClient.getFormattedTime().substring(0, 5);
  }

  String monthDay() {
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm* ptm = gmtime((time_t*)&epochTime); 
    byte monthDay = ptm->tm_mday;
    if (monthDay < 10) {
      monthDayStr = "0" + String(monthDay);
    } else {
      monthDayStr = String(monthDay);
    }
    return monthDayStr;
  }

  String currentMonth() {
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm* ptm = gmtime((time_t*)&epochTime); 
    byte currentMonth = ptm->tm_mon + 1;
    if (currentMonth < 10) {
      currentMonthStr = "0" + String(currentMonth);
    } else {
      currentMonthStr = String(currentMonth);
    }
    return currentMonthStr;
  }

  String currentYear() {
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm* ptm = gmtime((time_t*)&epochTime); 
    int currentYear = ptm->tm_year + 1900;
    return String(currentYear);
  }

  String completeTime() {
    return String(currentYear()) + "-" + currentMonth() + "-" + monthDay() + " " + timeClient.getFormattedTime();
  }
};
TimeClientHandler timeClientHandler;


/* commandRunner
void commandRunner(String CRcommand) {

  String command = fbdo.stringData();
  cmd = parseCommand(command);
  String parseCommand = cmd.command;
  String params[0] = cmd.values[0];
  String params[1] = cmd.values[1];


  for (int i = 0; i < cmd.valueCount; i++) {
    Serial.println("value " + String(i + 1) + ": " + cmd.values[i]);
  }

  if (command.indexOf('.') != -1) {
    Serial.println("command: " + cmd.command);

    if (command == "wifi.rssi") {  // Kekuatan sinyal WiFi
      long rssi = WiFi.RSSI();
      commandOutput(String(rssi));
    } else if (command == "wifi.localIp") {  // Alamat IP lokal
      commandOutput(WiFi.localIP().toString());
    } else if (command == "wifi.macAddress") {  // MAC Address
      commandOutput(WiFi.macAddress());
    } else if (command == "esp.restart") {  // restart
      commandOutput("restarting board");
      if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
      ESP.restart();
    } else if (command == "esp.millis") {  // millis
      commandOutput(String(millis()));
    } else if (command == "ntp.completeTime") {  // waktu NTP
      commandOutput(completeTime);
    } else if (command == "pot.value") {  // pot value
      commandOutput(String(analogRead(potPin)));
    } else if (command == "pot.currentPos") {  // pot current pos
      commandOutput(String(currentPos));
    } else if (command == "esp.getFreeHeap") {  // Free Heap
      commandOutput(String(ESP.getFreeHeap()));
    } else if (command == "esp.getHeapFragmentation") {  // Fragmentation
      commandOutput(String(ESP.getHeapFragmentation()));
    } else if (command == "esp.getMaxFreeBlockSize") {  // FreeBlockSize
      commandOutput(String(ESP.getMaxFreeBlockSize()));
    } else if (command == "esp.getFlashChipSize") {  // FlashChipSize
      commandOutput(String(ESP.getFlashChipSize()));
    } else if (command == "esp.getFlashChipRealSize") {  // FlashChippReal
      commandOutput(String(ESP.getFlashChipRealSize()));
    } else if (command == "esp.getFreeSketchSpace") {  // FreeSketchS
      commandOutput(String(ESP.getFreeSketchSpace()));
    } else if (command == "esp.getSketchSize") {  // SketchSize
      commandOutput(String(ESP.getSketchSize()));
    } else if (parseCommand == "wifi.begin") {  // ganti wifi
      if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
      if (params[0].length() >= 30 || params[1].length() >= 30) {
        commandOutput("ssid atau password lebih dari 30 huruf");
        return;
      }
      WiFi.disconnect();
      wait(1000);
      wifiHandler(params[0], params[1], true);

      if (WiFi.status() == WL_CONNECTED) {
        commandOutput("new wifi connected");
        saveStringToEEPROM(ssidAddress, params[0]);
        saveStringToEEPROM(passwordAddress, params[1]);
      } else {
        wifiHandler(wifiSsid, wifiPassword, true);
        commandOutput("new wifi not connected, connected to last wifi connection");
      }
    } else if (parseCommand == "esp.delay") {  // delay
      commandOutput("esp delay start");
      wait(params[0].toInt());
      commandOutput("esp delay ended");
    } else if (parseCommand == "servo.setMaxAngle") {  // servo max angle
      if (params[0].toInt() <= 180) {
        servoMaxAngle = params[0].toInt();
        accessEEPROM(servoMAAddress, params[0].toInt());
        commandOutput("servo set max angle");
      } else {
        commandOutput("error: angle too large");
      }
    } else if (parseCommand == "servo.setMinAngle") {  // servo min angle
      if (params[0].toInt() >= 0) {
        servoMinAngle = params[0].toInt();
        accessEEPROM(servoMIAAddress, params[0].toInt());
        delay(1000);
        commandOutput("servo set min angle");
      } else {
        commandOutput("error: angle too small");
      }

    } else if (parseCommand == "servo.setOpenDelay") {
      if (params[0].toInt() <= 10000) {
        servoOpenDelay = params[0].toInt();
        accessEEPROM(servoODAddress, servoOpenDelay);
        commandOutput("servo set open delay");
      } else {
        commandOutput("error: delay too long");
      }


    } else if (parseCommand == "servo.setCloseDelay") {
      if (params[0].toInt() >= 80) {
        servoCloseDelay = params[0].toInt();
        accessEEPROM(servoCDAddress, params[0].toInt());
        commandOutput("servo set close delay");
      } else {
        commandOutput("error: delay too fast");
      }
    } else if (parseCommand == "servo.katup") {  // katup
      servoKatup(params[0].toInt());
      commandOutput("valve finished running");
    } else if (parseCommand == "servo.setAngle") {  // set angle
      myServo.attach(servoPin, 500, 2500);
      myServo.write(params[0].toInt());
      delay(500);
      myServo.detach();
      commandOutput("servo set to current angle");
    } else if (parseCommand == "buzzer.tone") {  // buzzer tone
      buzzerT(params[0].toInt());
      commandOutput("buzzer tone complete running");
    } else if (parseCommand == "eeprom.get") {  // get EEPROM
      int intValues = params[0].toInt();
      if (intValues != ssidAddress && intValues != passwordAddress) {
        commandOutput(String(accessEEPROM(params[0].toInt(), -1)));
      } else {
        commandOutput(readStringFromEEPROM(params[0].toInt()));
      }

    } else if (parseCommand == "eeprom.getAll") {

      String getAllResult = getAllEeprom();
      getAllResult.replace("\n", "\\n");
      Serial.println(getAllResult);
      commandOutput(getAllResult);

    } else if (parseCommand == "led.state") {  // set led status
      if (params[0] == trueVal) {
        digitalWrite(ledPin, HIGH);
        commandOutput("led true");
      } else if (params[0] == falseVal) {
        digitalWrite(ledPin, LOW);
        commandOutput("led false");
      }
    } else if (parseCommand == "led.effect") {  // efel led
      if (params[0] == "fadeIn") {
        ledFadeIn();
        digitalWrite(ledPin, LOW);
        commandOutput("led fadeing in");
      } else if (params[0] == "fadeOut") {
        ledFadeOut();
        commandOutput("led fadeing out");
      }
    } else if (parseCommand == "eeprom.writeString") {  // write string to eeprom
      if (params[0].toInt() == ssidAddress || params[0].toInt() == passwordAddress && params[1].length() <= 30) {
        saveStringToEEPROM(params[0].toInt(), params[1]);
        commandOutput("string data is saved");
      } else {
        commandOutput("error: destination address saves int");
      }
    } else if (parseCommand == "eeprom.writeInterger") {  // write interger to eeprom
      if (params[0].toInt() != ssidAddress && params[0].toInt() != passwordAddress) {
        if (params[1].toInt() <= 99999) {
          accessEEPROM(params[0].toInt(), params[1].toInt());
          commandOutput("interger data is saved");
        } else {
          commandOutput("error: number greater than 99999");
        }
      } else {
        commandOutput("error: destination address saves string");
      }
    } else if (parseCommand == "servo.enable") {  // enable servo
      bool enableServo2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableServoAddr, 1);
        enableServo = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableServoAddr, 0);
        enableServo = false;
      } else {
        commandOutput("error: wrong value");
        enableServo2 = false;
      }
      if (enableServo2) {
        commandOutput("servo permission updated");
      }

    } else if (parseCommand == "buzzer.enable") {  // enable buzzer
      bool enableBuzzer2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableBuzzerAddr, 1);
        enableBuzzer = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableBuzzerAddr, 0);
        enableBuzzer = false;
      } else {
        commandOutput("error: wrong value");
        enableBuzzer2 = false;
      }
      if (enableBuzzer2) {
        commandOutput("buzzer permission updated");
      }

    } else if (parseCommand == "led.enable") {  // enable led
      bool enableLed2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableLedAddr, 1);
        enableLed = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableLedAddr, 0);
        enableLed = false;
      } else {
        commandOutput("error: wrong value");
        enableLed2 = false;
      }
      if (enableLed2) {
        commandOutput("led permission updated");
      }
    } else if (parseCommand == "pot.enable") {  // enable potentometer
      bool enablePot2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enablePotAddr, 1);
        enablePot = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enablePotAddr, 0);
        enablePot = false;
      } else {
        commandOutput("error: wrong value");
        enablePot2 = false;
      }
      if (enablePot2) {
        commandOutput("pot permission updated");
      }
    } else if (parseCommand == "button.enable") {  // enable button
      bool enableButton2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableButtonAddr, 1);
        enableButton = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableButtonAddr, 0);
        enableButton = false;
      } else {
        commandOutput("error: wrong value");
        enableButton2 = false;
      }
      if (enableButton2) {
        commandOutput("button permission updated");
      }

    } else if (parseCommand == "tiltSensor.enable") {  // enable tilt sensor
      bool enableTiltSensor2 = true;
      if (params[0] == trueVal) {
        accessEEPROM(enableTiltSensorAddr, 1);
        enableTiltSensor = true;
      } else if (params[0] == falseVal) {
        accessEEPROM(enableTiltSensorAddr, 0);
        enableTiltSensor = false;
      } else {
        commandOutput("error: wrong value");
        enableTiltSensor2 = false;
      }
      if (enableTiltSensor2) {
        commandOutput("servo permission updated");
      }

    } else {
      commandOutput(0);
    }
    if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
  } else {
    if (command != "~") {
      commandOutput(0);
    }
  }
}

*/


// --------------------------------- functions ---------------------------------//



void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println(" ");
  Serial.println("program dimulai");

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(16, WAKEUP_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(tiltSensorPin, INPUT_PULLUP);

  wifiSsid = readStringFromEEPROM(ssidAddress);
  wifiPassword = readStringFromEEPROM(passwordAddress);
  wifiHandler.startConnect();
  timeClientHandler.start();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  servoMaxAngle = accessEEPROM(servoMAAddress, -1);
  servoMinAngle = accessEEPROM(servoMIAAddress, -1);
  servoOpenDelay = accessEEPROM(servoODAddress, -1);
  servoCloseDelay = accessEEPROM(servoCDAddress, -1);

  enableServo = (accessEEPROM(enableServoAddr, -1) != 0);
  enableBuzzer = (accessEEPROM(enableBuzzerAddr, -1) != 0);
  enableLed = (accessEEPROM(enableLedAddr, -1) != 0);
  enablePot = (accessEEPROM(enablePotAddr, -1) != 0);
  enableButton = (accessEEPROM(enableButtonAddr, -1) != 0);
  enableTiltSensor = (accessEEPROM(enableTiltSensorAddr, -1) != 0);

  if (servoMaxAngle <= 0) {
    servoMaxAngle = 110;
    servoMinAngle = 0;
  }
}

void loop() {
  ifPrintln("loop");
  OOOOOOOOOO_codeMarker(3);

  // buzzer start
  if (buzzerStart) {
    buzzerT(7);
    buzzerStart = false;
  }

  wifiHandler.autoCheck();
  OOOOOOOOOO_codeMarker(20);

  // update waktu
  if (isWifiConnect &&  millis() - timeClientUpdateDelay >= 1000) {
    currentTime = timeClientHandler.currentTime();
    completeTime = timeClientHandler.completeTime();
    timeClientUpdateDelay = millis();
  }

  // timeClient update
  OOOOOOOOOO_codeMarker(4);
  /* timeClient.update();
  if (millis() - timeClientUpdateDelay >= 1000) {
    OOOOOOOOOO_codeMarker(5);

    time_t epochTime = timeClient.getEpochTime();
    struct tm* ptm = gmtime((time_t*)&epochTime); 

    int
      monthDay = ptm->tm_mday,            // tanggal
      currentMonth = ptm->tm_mon + 1,     // bulan
      currentYear = ptm->tm_year + 1900;  // tahun

    currentTime = timeClient.getFormattedTime().substring(0, 5);
    String formattedTime = timeClient.getFormattedTime();  // 00:00:00
    if (currentMonth < 10) {
      currentMonthStr = "0" + String(currentMonth);
    } else {
      currentMonthStr = String(currentMonth);
    }
    if (monthDay < 10) {
      monthDayStr = "0" + String(monthDay);
    } else {
      monthDayStr = String(monthDay);
    }
    completeTime = String(currentYear) + "-" + String(currentMonthStr) + "-" + String(monthDayStr) + " " + String(formattedTime);

    timeClientUpdateDelay = millis();
  } */

  // serial input
  if (Serial.available() > 0) {
    char serialCommand = Serial.read();
    if (serialCommand == '1') {
      serialPrint = true;
      Serial.println("enable all serial prints");
    } else if (serialCommand == '0') {
      serialPrint = false;
      Serial.println("disable all serial prints");
    } else if (serialCommand == 'a') {
      buzzerT(1);
    } else if (serialCommand == 'b') {
      buzzerT(2);
    } else if (serialCommand == 'c') {
      buzzerT(3);
    } else if (serialCommand == 'd') {
      buzzerT(4);
    } else if (serialCommand == 'e') {
      buzzerT(5);
    } else if (serialCommand == 'f') {
      buzzerT(6);
    } else if (serialCommand == 'g') {
      ledFadeIn();
      if (enableLed) {
        digitalWrite(ledPin, HIGH);
        delay(3000);
        ledFadeOut();
        digitalWrite(ledPin, LOW);
      }
    } else if (serialCommand == 'z') {
      codeMarkerPrint = !codeMarkerPrint;
    }
  }

  static unsigned long lastPrintMillis = 0;
  if (millis() - lastPrintMillis >= 3000) {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
      Serial.println("Access Point AKTIF!");
    } else {
      Serial.println("Access Point TIDAK aktif");
    }
    lastPrintMillis = millis();
  }



  OOOOOOOOOO_codeMarker(6);

  // runtime tasks
  loopRate();
  otaHandler.autoHandle();
  button(completeTime);
  tiltSensor(completeTime);
  webServer.handle(webServerIsActive);
  if (webServerIsActive) { server.handleClient(); }
  potentiometer(currentTime, completeTime);
  if (currentTime == "00:00") ESP.deepSleepMax(); // deepsleep
  if ((fbdConnected = firebaseHandler.start())) fbdConnected = Firebase.ready(); // cek apakah sudah signup firebase

  OOOOOOOOOO_codeMarker(7);

  
  
  // firebase database
  if (isWifiConnect && fbdConnected) {
    OOOOOOOOOO_codeMarker(8);

    // mobile app update
    if (millis() - prevUpMillis >= 3000) {
      mobileLatestUp = firebaseHandler.getString("/mobileLatestUp");
      if (mobileLatestUp != mobileLastUp) {
        mobileLastUp = mobileLatestUp;
        firebaseUpdate = true;
      }
      prevUpMillis = millis();
    }
    
    // last start
    if (lastStart) {
      firebaseHandler.setString("/lastStart", completeTime);
      lastStart = false;
    }
   
    // update loop rate
    if (millis() - prevLoopRateM >= 3000) {
      firebaseHandler.setFloat("/loopRate", averageHZ);
      prevLoopRateM = millis();
    }
   
    // update firebase
    if (firebaseUpdate) {
      Serial.println("firebase update");

      // update tone buzzer
      currentBuzzerTone = firebaseHandler.getInt("/buzzer/tone");

      // perintah aktivasi buzzer
      buzzerAct = firebaseHandler.getBool("/buzzer/isBuzzer");
      if (buzzerAct) {
        buzzerT(currentBuzzerTone);
        firebaseHandler.setBool("/buzzer/isBuzzer", false);
        ifPrintln("fb set isBuzzer false");
      }

      // restart
      bool restart = firebaseHandler.getBool("/espRestart");
      if (restart) {
        ifPrintln("ESP restart true");
        firebaseHandler.setBool("/espRestart", false);
        ifPrintln("fb set espRestart false\nrestarting ESP8266...");
        delay(1000);
        ESP.restart();
      }

      // ambil data dari array setting di firebase
      FirebaseJsonArray fja;
      FirebaseJsonData fjd;
      FirebaseJsonData fjdWaktu;
      FirebaseJsonData fjdKatup;
      if (Firebase.RTDB.getArray(&fbdo, "/setting")) {
        fja = fbdo.jsonArray();
      }

      // loop ambil value array sesuai panjang array
      for (int i = 0; i < fja.size(); i++) {
        // ambil isi objek array yaitu waktu & katup
        // lalu simpan ke variable
        ifPrintln("fb get array");
        if (fja.get(fjd, i)) {
          FirebaseJson fj;
          fjd.getJSON(fj);
          fj.get(fjdWaktu, "/waktu");
          fj.get(fjdKatup, "/katup");
          waktusBaru[i] = fjdWaktu.stringValue;
          katupsBaru[i] = fjdKatup.intValue;
          // tes print data yang didapat
          ifPrintln("");
          ifPrintln("data ke " + String(i) + ":");
          ifPrintln("waktu: " + waktusBaru[i]);
          ifPrintln("katup: " + String(katupsBaru[i]));
          wait(200);
        }
      }
      ifPrintln("");

      // feeding
      bool throwOut = firebaseHandler.getBool("/feeding/throwOut");
      int feedValue = firebaseHandler.getInt("/feeding/value");
      if (throwOut && feedValue != 0 && feedValue <= 4) {
        Serial.println("servo throwOut");
        servoKatup(feedValue);
        lastFood("fdg", feedValue, completeTime);
        firebaseHandler.setBool("/feeding/throwOut", false);
        ifPrintln("fb set throwOut false");
      }

      // command start line
      String command = firebaseHandler.getString("/command/inputCode");
      cmd = parseCommand(command);
      String parseCommand = cmd.command;
      String params[0] = cmd.values[0];
      String params[1] = cmd.values[1];
      const String wrongBoolValue = "error: wrong value, must be true or false";
      const String unknownCommand = "unknown input command, please check your command and try again";

      for (int i = 0; i < cmd.valueCount; i++) {
        Serial.println("value " + String(i + 1) + ": " + cmd.values[i]);
      }

      if (command.indexOf('.') != -1) {
        Serial.println("command: " + cmd.command);

        if (command == "wifi.rssi") {  // Kekuatan sinyal WiFi
          long rssi = WiFi.RSSI();
          commandOutput(String(rssi));
        } else if (command == "wifi.localIp") {  // Alamat IP lokal
          commandOutput(WiFi.localIP().toString());
        } else if (command == "wifi.macAddress") {  // MAC Address
          commandOutput(WiFi.macAddress());
        } else if (command == "esp.restart") {  // restart
          commandOutput("restarting board");
          if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
          ESP.restart();
        } else if (command == "esp.millis") {  // millis
          commandOutput(String(millis()));
        } else if (command == "ntp.completeTime") {  // waktu NTP
          commandOutput(completeTime);
        } else if (command == "pot.value") {  // pot value
          commandOutput(String(analogRead(potPin)));
        } else if (command == "pot.currentPos") {  // pot current pos
          commandOutput(String(currentPos));
        } else if (command == "esp.getFreeHeap") {  // Free Heap
          commandOutput(String(ESP.getFreeHeap()));
        } else if (command == "esp.getHeapFragmentation") {  // Fragmentation
          commandOutput(String(ESP.getHeapFragmentation()));
        } else if (command == "esp.getMaxFreeBlockSize") {  // FreeBlockSize
          commandOutput(String(ESP.getMaxFreeBlockSize()));
        } else if (command == "esp.getFlashChipSize") {  // FlashChipSize
          commandOutput(String(ESP.getFlashChipSize()));
        } else if (command == "esp.getFlashChipRealSize") {  // FlashChippReal
          commandOutput(String(ESP.getFlashChipRealSize()));
        } else if (command == "esp.getFreeSketchSpace") {  // FreeSketchS
          commandOutput(String(ESP.getFreeSketchSpace()));
        } else if (command == "esp.getSketchSize") {  // SketchSize
          commandOutput(String(ESP.getSketchSize()));
        } else if (parseCommand == "wifi.begin") {  // ganti wifi
          if (params[0].length() >= 30 || params[1].length() >= 30) {
            commandOutput("ssid atau password lebih dari 30 huruf");
            return;
          }
          newWifiSsid = params[0];
          newWifiPassword = params[1];
          wifiHasChanged = true;
          wait(500);
          wifiHandler.startConnect();

        } else if (parseCommand == "esp.delay") {  // delay
          commandOutput("esp delay start");
          wait(params[0].toInt());
          commandOutput("esp delay ended");
        } else if (parseCommand == "servo.setMaxAngle") {  // servo max angle
          if (params[0].toInt() <= 180) {
            servoMaxAngle = params[0].toInt();
            accessEEPROM(servoMAAddress, params[0].toInt());
            commandOutput("servo set max angle");
          } else {
            commandOutput("error: angle too large");
          }
        } else if (parseCommand == "servo.setMinAngle") {  // servo min angle
          if (params[0].toInt() >= 0) {
            servoMinAngle = params[0].toInt();
            accessEEPROM(servoMIAAddress, params[0].toInt());
            delay(1000);
            commandOutput("servo set min angle");
          } else {
            commandOutput("error: angle too small");
          }

        } else if (parseCommand == "servo.setOpenDelay") {
          if (params[0].toInt() <= 10000) {
            servoOpenDelay = params[0].toInt();
            accessEEPROM(servoODAddress, servoOpenDelay);
            commandOutput("servo set open delay");
          } else {
            commandOutput("error: delay too long");
          }


        } else if (parseCommand == "servo.setCloseDelay") {
          if (params[0].toInt() >= 80) {
            servoCloseDelay = params[0].toInt();
            accessEEPROM(servoCDAddress, params[0].toInt());
            commandOutput("servo set close delay");
          } else {
            commandOutput("error: delay too fast");
          }
        } else if (parseCommand == "servo.katup") {  // katup
          servoKatup(params[0].toInt());
          commandOutput("valve finished running");
        } else if (parseCommand == "servo.setAngle") {  // set angle
          myServo.attach(servoPin, 500, 2500);
          myServo.write(params[0].toInt());
          delay(500);
          myServo.detach();
          commandOutput("servo set to current angle");
        } else if (parseCommand == "buzzer.tone") {  // buzzer tone
          buzzerT(params[0].toInt());
          commandOutput("buzzer tone complete running");
        } else if (parseCommand == "eeprom.get") {  // get EEPROM
          int intValues = params[0].toInt();
          if (intValues != ssidAddress && intValues != passwordAddress) {
            commandOutput(String(accessEEPROM(params[0].toInt(), -1)));
          } else {
            commandOutput(readStringFromEEPROM(params[0].toInt()));
          }

        } else if (parseCommand == "eeprom.getAll") {

          String getAllResult = getAllEeprom();
          getAllResult.replace("\n", "\\n");
          Serial.println(getAllResult);
          commandOutput(getAllResult);

        } else if (parseCommand == "led.state") {  // set led status
          if (params[0] == trueVal) {
            digitalWrite(ledPin, HIGH);
            commandOutput("led true");
          } else if (params[0] == trueVal) {
            digitalWrite(ledPin, LOW);
            commandOutput("led false");
          }
        } else if (parseCommand == "led.effect") {  // efel led
          if (params[0] == "fadeIn") {
            ledFadeIn();
            digitalWrite(ledPin, LOW);
            commandOutput("led fadeing in");
          } else if (params[0] == "fadeOut") {
            ledFadeOut();
            commandOutput("led fadeing out");
          }
        } else if (parseCommand == "eeprom.writeString") {  // write string to eeprom
          if (params[0].toInt() == ssidAddress || params[0].toInt() == passwordAddress && params[1].length() <= 30) {
            saveStringToEEPROM(params[0].toInt(), params[1]);
            commandOutput("string data is saved");
          } else {
            commandOutput("error: destination address saves int");
          }
        } else if (parseCommand == "eeprom.writeInterger") {  // write interger to eeprom
          if (params[0].toInt() != ssidAddress && params[0].toInt() != passwordAddress) {
            if (params[1].toInt() <= 99999) {
              accessEEPROM(params[0].toInt(), params[1].toInt());
              commandOutput("interger data is saved");
            } else {
              commandOutput("error: number greater than 99999");
            }
          } else {
            commandOutput("error: destination address saves string");
          }
        } else if (parseCommand == "servo.enable") {  // enable servo
          bool enableServo2 = true;
          if (params[0] == trueVal) {
            accessEEPROM(enableServoAddr, 1);
            enableServo = true;
          } else if (params[0] == trueVal) {
            accessEEPROM(enableServoAddr, 0);
            enableServo = false;
          } else {
            commandOutput("error: wrong value");
            enableServo2 = false;
          }
          if (enableServo2) {
            commandOutput("servo permission updated");
          }

        } else if (parseCommand == "buzzer.enable") {  // enable buzzer
          bool enableBuzzer2 = true;
          if (params[0] == trueVal) {
            accessEEPROM(enableBuzzerAddr, 1);
            enableBuzzer = true;
          } else if (params[0] == falseVal) {
            accessEEPROM(enableBuzzerAddr, 0);
            enableBuzzer = false;
          } else {
            commandOutput("error: wrong value");
            enableBuzzer2 = false;
          }
          if (enableBuzzer2) {
            commandOutput("buzzer permission updated");
          }

        } else if (parseCommand == "led.enable") {  // enable led
          bool enableLed2 = true;
          if (params[0] == trueVal) {
            accessEEPROM(enableLedAddr, 1);
            enableLed = true;
          } else if (params[0] == falseVal) {
            accessEEPROM(enableLedAddr, 0);
            enableLed = false;
          } else {
            commandOutput("error: wrong value");
            enableLed2 = false;
          }
          if (enableLed2) {
            commandOutput("led permission updated");
          }
        } else if (parseCommand == "pot.enable") {  // enable potentometer
          bool enablePot2 = true;
          if (params[0] == trueVal) {
            accessEEPROM(enablePotAddr, 1);
            enablePot = true;
          } else if (params[0] == falseVal) {
            accessEEPROM(enablePotAddr, 0);
            enablePot = false;
          } else {
            commandOutput("error: wrong value");
            enablePot2 = false;
          }
          if (enablePot2) {
            commandOutput("pot permission updated");
          }
        } else if (parseCommand == "button.enable") {  // enable button
          bool enableButton2 = true;
          if (params[0] == trueVal) {
            accessEEPROM(enableButtonAddr, 1);
            enableButton = true;
          } else if (params[0] == falseVal) {
            accessEEPROM(enableButtonAddr, 0);
            enableButton = false;
          } else {
            commandOutput("error: wrong value");
            enableButton2 = false;
          }
          if (enableButton2) {
            commandOutput("button permission updated");
          }

        } else if (parseCommand == "tiltSensor.enable") {  // enable tilt sensor
          bool enableTiltSensor2 = true;
          if (params[0] == trueVal) {
            accessEEPROM(enableTiltSensorAddr, 1);
            enableTiltSensor = true;
          } else if (params[0] == falseVal) {
            accessEEPROM(enableTiltSensorAddr, 0);
            enableTiltSensor = false;
          } else {
            commandOutput("error: wrong value");
            enableTiltSensor2 = false;
          }
          if (enableTiltSensor2) {
            commandOutput("servo permission updated");
          }
        } else if (parseCommand == "wifi.ap") {
          if (parseCommand == trueVal) {
            WiFi.softAP(APSsid, APPassword);
            apIsActive = true;
            commandOutput("AP mode turned on");
          } else if (parseCommand == falseVal) {
            WiFi.softAPdisconnect(true);
            apIsActive = false;
            commandOutput("AP mode turned off");
          } else {
            commandOutput(wrongBoolValue);
          }
        } else if (parseCommand == "ota.run") {
          if (params[0] == trueVal) {
            commandOutput("OTA start");
            otaHandler.start();
          } else if (params[0] == falseVal) {
            otaHandler.end();
            commandOutput("OTA ended");
          } else {
            commandOutput(wrongBoolValue);
          }
        } else if (parseCommand == "webserver.run") {
          if (params[0] == trueVal) {
            webServerIsActive = true;
            commandOutput("webserver started");
          } else if (params[0] == falseVal) {
            webServerIsActive = false;
            commandOutput("webserver ended");
          } else {
            commandOutput(wrongBoolValue);
          }
        } else if (parseCommand == "webserver.sendStr") {
          if (webServer.sendStr(200, params[0])) {
            commandOutput("webserver send string success");
          } else {
            commandOutput("webserver send string failed");
          }
        } else {
          commandOutput(unknownCommand);
        }
        firebaseHandler.setString("/command/inputCode", "~");
      } else {
        commandOutput(unknownCommand);
      }
      // command end line

      firebaseUpdate = false;
      wait(100);
    }
  
  }

  // led dan servo sesuai waktu
  if (!cooldownK) {
    for (int i = 0; i < 24; i++) {
      if (currentTime == waktusBaru[i]) {
        int katup = katupsBaru[i];
        Serial.println("servo waktu");
        servoKatup(katup);
        lastFood("tmr", katup, completeTime);
        cooldownK = true;
        cooldownStartK = millis();
        break;
      }
    }
  } else if (cooldownK) {
    if (millis() - cooldownStartK >= 61000) {
      cooldownStartK = millis();
      cooldownK = false;
    }
  }
  yield();
  systemHasStarted = false;
  OOOOOOOOOO_codeMarker(10);

}