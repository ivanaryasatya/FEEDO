
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
  Version: 1.2.5

*/
#include <Arduino.h>
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <f:\Arduino\libraries\2820675-bbe995aa22826a8fbbb6b56ccd56513f9db6cb00\pitches.h>
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
#include <firebase_secret.h>

#define API_KEY "your_api_key_here"
#define DATABASE_URL "your_database_url_here"

#define ledPin D3
#define servoPin D5
#define buttonPin D2
#define buzzerPin D1
#define tiltSensorPin D4
#define REST 0
#define EEPROM_SIZE 512
#define maxLength 30
#define defaultInt 0
#define defaultStr ""
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
  lastPos = defaultInt,
  sampleSize = 30,
  currentBuzzerTone = 6,
  loopCount = defaultInt,
  currentPos = defaultInt,
  lastLastPos = defaultInt
;

int
  totalTime = defaultInt;

const unsigned int tones[] = { 300, 400, 500, 600, 700, 800, 900, 1000 };
float loopRateHz = 225;
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
  lastWifiStatus = false,
  otaIsActive = false,
  codeMarkerPrint = false,
  enableTiltSensor = true,
  webServerIsActive = false,
  wifiHasChanged = false,
  userActivatedWebServer = false,
  isSerialCommand = false,
  tiltSensorState = false
;
unsigned long
  bCooldownStart = defaultInt,
  sensorDTime = defaultInt,
  cooldownStartK = defaultInt,
  potPreviousMillis = defaultInt,
  prevLoopRate = defaultInt,
  previousTime = defaultInt,
  prevUpMillis = defaultInt,
  prevLoopRateM = defaultInt,
  currentPosPrint = defaultInt,
  lastReconnectAttempt = defaultInt,
  startAttemptTime = defaultInt,
  timeClientUpdateDelay = defaultInt
;
String
  mobileLastUp = defaultStr,
  mobileLatestUp = defaultStr,
  currentTime = "99:99",
  completeTime = defaultStr,
  currentMonthStr,
  monthDayStr,
  newWifiSsid = defaultStr,
  newWifiPassword = defaultStr,
  serialMessage = defaultStr,
  message = defaultStr;
const String
  APSsid = "FEEDO-ESP8266-AP",
  APPassword = "ikansegar",
  trueVal = "true",
  falseVal = "false",
  wifiHostName = "FEEDO-ESP8266"
;

const byte
  servoMaxAngleAddr = 0,
  servoMinAngleAddr = 5,
  servoOpenDelayAddr = 11,
  servoCloseDelayAddr = 17,
  wifiSsidAddr = 23,
  wifiPasswordAddr = 55,
  enableServoAddr = 87,
  enableBuzzerAddr = 91,
  enableLedAddr = 99,
  enablePotAddr = 103,
  enableButtonAddr = 107,
  enableTiltSensorAddr = 111
;
unsigned int
  servoMaxAngle = defaultInt,
  servoMinAngle = defaultInt,
  servoOpenDelay = defaultInt,
  servoCloseDelay = defaultInt
;
bool
  enableServo = true,
  enableBuzzer = true,
  enableLed = true,
  enablePot = true,
  enableButton = true,
  enableTiltSenso = true
;
String
  wifiSsid = defaultStr,
  wifiPassword = defaultStr
;
byte eepromAddress[] = { servoMaxAngleAddr, servoMinAngleAddr, servoOpenDelayAddr, servoCloseDelayAddr, wifiSsidAddr, wifiPasswordAddr, enableServoAddr, enableBuzzerAddr, enableLedAddr, enablePotAddr, enableButtonAddr, enableTiltSensorAddr };
String waktusBaru[24];
int katupsBaru[24];

IPAddress local_ip(192, 168, 4, 1);       // IP untuk ESP8266 (default: 192.168.4.1)
IPAddress gateway(192, 168, 4, 1);        // Gateway
IPAddress subnet(255, 255, 255, 0);       // Subnet mask

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

String target;
String command;
String rawCommand = "~";
std::vector<String> params;

// struct command
const String wrongBoolValue = "error: wrong value, must be true or false";
const String unknownCommand = "unknown command, please check your command and try again";
const String unknownTarget = "unknown target in command, please check your target and try again";
const String enable = "enable";
const String zeroStr = "0";
const String oneStr = "1";

/* class Message {
  public:
    class Info {
      public:

        // wifi
        const char* connected = "wifi connected";
        const char* disconnected = "wifi disconnected";
        const char* connectionFailed = "wifi connection failed, please check your wifi connection";
        const char* ssidChanged = "wifi ssid changed, please reconnect to the new wifi";
        const char* passwordChanged = "wifi password changed, please reconnect to the new wifi";

        // firebase
        const char* connected = "firebase connected";
        const char* disconnected = "firebase disconnected";
        const char* connectionFailed = "firebase connection failed, please check your firebase connection";
        const char* dataUpdated = "firebase data updated";
        const char* dataNotUpdated = "firebase data not updated";




        // const char* wifiConnected = "wifi connected";
        // const char* firebaseConnected = "firebase connected";
        // const char* webServerStarted = "webserver started";
        // const char* webServerEnded = "webserver ended";
        // const char* otaStarted = "ota started";
        // const char* otaEnded = "ota ended";
        // const char* commandReceived = "command received";
        // const char* commandExecuted = "command executed";
        // const char* commandNotFound = "command not found";
        // const char* commandError = "command error, please check your command and try again";
    };

    class Error {
      public:

      class Wifi {
        public:

      };

      class Firebase {
        public:

      };

      class WebServer {
        public:
      };

      class OTA {
        public:
      };

      class Command {
        public:
      };
        // const char* noWifi = "error: no wifi connection";
        // const char* noFirebase = "error: firebase not connected";
        // const char* noWebServer = "error: webserver not running";
        // const char* noCommand = "error: command not found";
        // const char* wrongCommand = "error: wrong command, please check your command and try again";
        // const char* wrongValue = "error: wrong value, please check your value and try again";
    };

    class Warning {
      public:
        class Warning_Wifi {
          public:
            const char* excessLetters = ;
            const char* noConnection = "warning: no wifi connection, please check your wifi connection";
            const char* noCredentials = "warning: no wifi credentials, please set your wifi credentials";
            const char* noSsid = "warning: no wifi ssid, please set your wifi ssid";
            const char* noPassword = "warning: no wifi password, please set your wifi password";
        };

        class Firebase {
          public:

        };

        class WebServer {
          public:
        };

        class OTA {
          public:
        };

        class Command {
          public:
        };

      Warning_Wifi wifi;
      Firebase firebase;
      WebServer webServer;
      OTA ota;
      Command command;
        // const char* noWifi = "warning: no wifi connection, please check your wifi connection";
        // const char* noFirebase = "warning: firebase not connected, please check your firebase connection";
        // const char* noWebServer = "warning: webserver not running, please check your webserver connection";
        // const char* noCommand = "warning: command not found, please check your command and try again";
    };

    // class FirebasePaths {
    //   public:
    //     const char* mobileLatestUp = "/mobileLatestUp";
    //     const char* lastStart = "/lastStart";
    //     const char* loopRate = "/loopRate";
    // };

    // class command {
    //   public:
    //   const char* wifi = "wifi";
    //   const char* esp = "esp";
    //   const char* servo = "servo";
    //   const char* buzzer = "buzzer";
    //   const char* eeprom = "eeprom";
    //   const char* led = "led";
    //   const char* pot = "pot";
    //   const char* button = "button";
    //   const char* tiltSensor = "tiltSensor";
    //   const char* ota = "ota";
    //   const char* webserver = "webserver";
    //   const char* serial = "serial";
    //   const char* feedo = "feedo";
    // };

    Info info;
    Error error;
    Warning warning;
    FirebasePaths firebasePath;
};
Message savesMessages;
 */

class TimeClientHandler {
  public:
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

// String readStringFromEEPROM(int addr) {
//   int i;
//   for (i = 0; i < maxLength; i++) {
//     buffer[i] = EEPROM.read(addr + i);
//     if (buffer[i] == 0) break;
//   }
//   buffer[i] = '\0';
//   Serial.print("Membaca EEPROM [");
//   Serial.print(addr);
//   Serial.print("] = ");
//   Serial.println(String(buffer));
//   return String(buffer);
// }

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
void buzzerT(byte bTone) {
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
  }

  buzzerAct = false;
  //(buzzerDelay) ? delay(1000) : delay(0);
  if (buzzerDelay) delay(1000);
  ifPrintln("fungsi buzzer berakhir");
}

// potentiometer
void potentiometer() {
  if (!enablePot) {
    return;
  }
  unsigned int potValue = analogRead(potPin);
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
      lastFood("pot", lastPos, timeClientHandler.completeTime());
      over = false;
    }
  }
}

// button
void button() {
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
        lastFood("btn", 1, timeClientHandler.completeTime());
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
    loopRateHz = rataHz;
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

class EepromManager {
  public:
    // read = 0
    // write = 1
    void printInfo(const byte mode, const unsigned int address, const unsigned int intValue, const String strValue) {
      if (mode == 0) {
        Serial.print("Membaca EEPROM [");
      } else if (mode == 1) {
        Serial.print("Menulis EEPROM [");
      }
      Serial.print(address);
      Serial.print("] = ");
      if (strValue != defaultStr) {
        Serial.println(strValue);
      } else {
        Serial.println(intValue);
      }
    }

    // menulis byte
    void writeByte(const byte address, const byte value) {
      EEPROM.write(address, value);
      EEPROM.commit();
      printInfo(1, address, value, defaultStr);
    }

    // mebaca byte
    byte readByte(const unsigned int address) {
      unsigned int value = EEPROM.read(address);
      printInfo(0, address, value, defaultStr);
      return value;
    }

    // menulis data
    void putData(const unsigned int address, const int value) {
      EEPROM.put(address, value);
      EEPROM.commit();
      printInfo(1, address, value, defaultStr);
    }

    // membaca data
    int getData(const unsigned int address, const bool enablePrintInfo) {
      unsigned int storedValue;
      EEPROM.get(address, storedValue);
      if (enablePrintInfo) printInfo(0, address, storedValue, defaultStr);
      return storedValue;
    }

    // menulis string
    void writeString(const unsigned int address, const String value) {
      int len = value.length();
      if (len > maxLength) len = maxLength;
      for (int i = 0; i < len; i++) {
        EEPROM.put(address + i, value[i]);
      }
      EEPROM.write(address + len, 0);
      EEPROM.commit();
    }

    // membaca string
    String readString(const unsigned int address, const bool enablePrintInfo) {
      int i;
      for (i = 0; i < maxLength; i++) {
        buffer[i] = EEPROM.read(address + i);
        if (buffer[i] == 0) break;
      }
      buffer[i] = '\0';
      String value = String(buffer);
      if (enablePrintInfo) printInfo(0, address, 0, String(buffer));
      return String(buffer);
    }

    String getAll(const bool enablePrintInfo) {
      int eepAddArrSize = sizeof(eepromAddress) / sizeof(eepromAddress[0]);
      String result = defaultStr;
      String eepReadvalue;
      for (int i = 0; i < eepAddArrSize; i++) {
        if (eepromAddress[i] != wifiSsidAddr && eepromAddress[i] != wifiPasswordAddr) {
          eepReadvalue = String(getData(eepromAddress[i], false) );
        } else {
          eepReadvalue = readString(eepromAddress[i], false);
        }

        if (enablePrintInfo)
        result += "Membaca EEPROM [";
        result += String(eepromAddress[i]);
        result += "] = ";
        result += eepReadvalue;
        result += "\n";
      }
      return result;
    }
};
EepromManager eepromManager;

// wifiAp.begin(); akan mengaktifkan AP mode
class WifiAp {
  public:
    bool apHasStarted = false;
    bool status = false;

    void begin() {
      if (!status && !apHasStarted) {
        Serial.println("turning on AP mode");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(APSsid, APPassword);
        wait(500);
        status = true;
        apHasStarted = true;

        Serial.print("AP status: ");
        Serial.println(status);
        Serial.print("SSID: ");
        Serial.println(APSsid);
        Serial.print("password: ");
        Serial.println(APPassword);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());

      }
    }
    void end() {
      if (status) {
        Serial.println("turning off AP mode");
        WiFi.softAPdisconnect(true);
        status = false;
        apHasStarted = false;
      }
    }
    bool hasConnectedDevice() {
      return WiFi.softAPgetStationNum() > 0;
    }
};
WifiAp wifiAp;

bool webserverHasStarted = false;
bool webserverEndRequest = false;
// webserver handler
// HTTP example: http://192.168.4.1/ESP?command=<feedo.feeding.2>
class WebServer {
  public:
    // true = memulai webserver, menayalakn ap mode, memulai akan otomastis hanya sekali dipanggil
    // false = menghentikan webserver, menonaktifkan ap mode, menghentikan akan otomastis hanya sekali dipanggil
    void begin() {
      static bool webServerHasStoped = true;

      if (!webserverHasStarted) {
        wifiAp.begin();
        Serial.println("Starting webserver...");
        webserverEndRequest = webServerHasStoped = false;
        webServerIsActive = true;
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
        webserverHasStarted = true;
      }
    }

    void handle() {
      if (webServerIsActive) server.handleClient();
      if (isWifiConnect && webServerIsActive) {
        static unsigned long apStartTime = 0;
        static bool apTimerStarted = false;
        if (wifiAp.hasConnectedDevice()) {
          apTimerStarted = true;
          apStartTime = millis();
        } else {
          if (apTimerStarted && millis() - apStartTime >= 180000) {
            Serial.println("No AP clients for 3 minutes, disabling AP and webserver...");
            end();
            apTimerStarted = false;
            apStartTime = millis();
          }
        }
      }
      if (webserverEndRequest) end();
    }

    void end() {
      webserverEndRequest = true;
      if (webserverEndRequest && isWifiConnect && !wifiAp.hasConnectedDevice() && webServerIsActive) {
        webserverEndRequest = false;
        webserverHasStarted = false;
        webServerIsActive = false;
        server.stop();
        wifiAp.end();
        Serial.println("Webserver stopped");
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

};
WebServer webserver;
// wifi handler
class WifiHandler {

  public:
    // mulai koneksi dengan begin()
    void startConnect() {
      static bool handlerWifiStatus = false;
      static bool wifiHasSetToApSta = false;

      WiFi.softAPConfig(local_ip, gateway, subnet);
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

    }

    // auto check connect ulang jika wifi terputus
    void autoCheck() {
      static bool onceWifiStatusTask = true;

      if (WiFi.status() == WL_CONNECTED) { // koneksi berhasil
        isWifiConnect = true;
        OOOOOOOOOO_codeMarker(16);
        webserver.end();
        wifiAp.end();
        if (onceWifiStatusTask) {
          digitalWrite(LED_BUILTIN, LOW);
          delay(200);
          digitalWrite(LED_BUILTIN, HIGH);
          printInfo();
          onceWifiStatusTask = false;
        }
        if (wifiHasChanged) {
          eepromManager.writeString(wifiSsidAddr, newWifiSsid);
          eepromManager.writeString(wifiPasswordAddr, newWifiPassword);
          wifiSsid = newWifiSsid;
          wifiPassword = newWifiPassword;
          newWifiSsid = defaultStr;
          newWifiPassword = defaultStr;
          Firebase.RTDB.setString(&fbdo, "/command/output", "New wifi connected");
          wifiHasChanged = false;
        }

      } else { //---------------------------- koneksi gagal
        OOOOOOOOOO_codeMarker(17);
        isWifiConnect = false;
        onceWifiStatusTask = true;
        webserver.begin();
        static unsigned long lastReconnect = 0;
        if (!otaIsActive && millis() - lastReconnect > 60000 && WiFi.status() != WL_CONNECTED) {
          if (!wifiAp.hasConnectedDevice()) {
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
          Firebase.RTDB.setString(&fbdo, "/command/output", "failed to connect to new WiFi");
        }
      }
    }
    // AP STA mode = 0
    // STA mode = 1
    // AP mode = 2
    void printInfo() {
      Serial.print("wifi status: ");
      Serial.println(isWifiConnect);
      Serial.print("SSID: ");
      Serial.println(wifiSsid);
      Serial.print("password: ");
      Serial.println(wifiPassword);
      Serial.print("wifi IP: ");
      Serial.println(WiFi.localIP());

      // if (mode == 2 || mode == 0) {
      //   Serial.print("AP status: ");
      //   Serial.println(wifiAp.status);
      //   Serial.print("SSID: ");
      //   Serial.println(APSsid);
      //   Serial.print("password: ");
      //   Serial.println(APPassword);
      //   Serial.print("AP IP: ");
      //   Serial.println(WiFi.softAPIP());
      // }
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

// akses ke eeprom interger
// int accessEEPROM(int address, int value) {
//   if (address < 0 || address >= EEPROM_SIZE) {
//     Serial.println("Alamat di luar batas EEPROM!");
//     return -1;
//   }

//   if (value == -1) {  // Mode membaca jika tidak ada nilai yang diberikan
//     int storedValue;
//     EEPROM.get(address, storedValue);
//     Serial.print("Membaca EEPROM [");
//     Serial.print(address);
//     Serial.print("] = ");
//     Serial.println(storedValue);
//     return storedValue;
//   } else {  // Mode menulis jika ada nilai yang diberikan
//     EEPROM.put(address, value);
//     EEPROM.commit();
//     Serial.print("Menulis EEPROM [");
//     Serial.print(address);
//     Serial.print("] = ");
//     Serial.println(value);
//     return value;
//   }
//   delay(800);
// }

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
      if (Firebase.signUp(&config, &auth, defaultStr, defaultStr)) {
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
    if (!isWifiConnect) return defaultStr;
    if (Firebase.RTDB.getString(fbdo_s, path)) {
      return fbdo_s->stringData();
    } else {
      fbdoError();
      return defaultStr;
    }
  }
};
FirebaseHandler firebaseHandler = { &fbdo };

// tilt sensor
void tiltSensor() {
  if (!enableTiltSensor) {
    return;
  }
  bool tiltSensorState = digitalRead(tiltSensorPin) == LOW;
  if (tiltSensorState && !lastSensorState) {
    sensorDTime = millis();
  }
  if (tiltSensorState && (millis() - sensorDTime >= 5000)) {
    ifPrintln("tilt sensor true");

    if (fUpdateT1 && fbdConnected) {
      firebaseHandler.setBool("/tiltSensor/isTiltSensor", true);
      firebaseHandler.setString("/tiltSensor/waktu", completeTime);
      fUpdateT1 = false;
      fUpdateT2 = true;
    }
  } else if (!tiltSensorState) {
    ifPrintln("tilt sensor false");

    if (fUpdateT2 && fbdConnected) {
      firebaseHandler.setBool("/tiltSensor/isTiltSensor", false);
      fUpdateT1 = true;
      fUpdateT2 = false;
    }
  }
  lastSensorState = tiltSensorState;
}

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
}

/* void saveStringToEEPROM(int addr, String data) {
  int len = data.length();
  if (len > maxLength) len = maxLength;
  for (int i = 0; i < len; i++) {
    EEPROM.put(addr + i, data[i]);
  }
  EEPROM.write(addr + len, 0);
  EEPROM.commit();
}

mendapatkan semua data EEPROM
String getAllEeprom() {

  int eepAddArrSize = sizeof(eepromAddress) / sizeof(eepromAddress[0]);
  String result = defaultStr;
  String eepReadvalue;
  int storedValue;
  for (int i = 0; i < eepAddArrSize; i++) {
    if (eepromAddress[i] != wifiSsidAddr && eepromAddress[i] != wifiPasswordAddr) {
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
*/


// handle OTA
class OtaHandler {
  private:
    unsigned long lastBlink = 0;
    bool ledState = false;
    bool otaHasEnded = true;
    bool otaInitialized = false;

  public:

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
        // wifiAp.begin();
        // ganti ap manual
        wifiAp.begin();

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
        Serial.println(WiFi.softAPIP());
        otaIsActive = otaInitialized = true;
        OOOOOOOOOO_codeMarker(2);
      }

    }

    // end: hentikan ota
    void end() {
      Serial.println("ending OTA");
      otaIsActive = otaInitialized = false;
      if (!otaHasEnded) {
        digitalWrite(LED_BUILTIN, HIGH);
        ArduinoOTA.end();
        if (!webServerIsActive) {
          wifiAp.end();
          otaHasEnded = true;
        }
      }
    }


};
OtaHandler otaHandler;

class CommandHandler {
  public:
    // Contoh command: led.builtin on 1000
    void parse(String input) {
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

    void output(const String oCommand, const bool serialCommand) {
      if (oCommand != "~") {
        if (serialCommand) {
          Serial.println("command output: " + oCommand);
        } else {
          firebaseHandler.setString("/command/output", oCommand);
        }
      }
    }

  // target.command  param1 param2 p...
  bool execute(String INF_execute_rawCommand) {
    parse(INF_execute_rawCommand);
    Serial.println("=== EXECUTING COMMAND ===");
    Serial.println("Target : " + target);
    Serial.println("Command: " + command);
    for (int i = 0; i < params.size(); i++) {
      Serial.println("Param[" + String(i) + "]: " + params[i]);
    }

    if (target == "wifi") {  // wifi
      if (command == "getRssi") {
        long rssi = WiFi.RSSI();
        output(String(rssi), isSerialCommand);
      } else if (command == "getLocalIp") {
        output(WiFi.localIP().toString(), isSerialCommand);
      } else if (command == "getMacAddress") {
        output(WiFi.macAddress(), isSerialCommand);
      } else if (command == "begin") {
        if (params[0].length() >= 30 || params[1].length() >= 30) {
        output("SSID atau password lebih dari 30 karakter", isSerialCommand);
        return false;
        }
        newWifiSsid = params[0];
        newWifiPassword = params[1];
        wifiHasChanged = true;
        wait(500);
        wifiHandler.startConnect();
      } else if (command == "getApStationNum") {
        output(String(WiFi.softAPgetStationNum()), isSerialCommand);
      } else if (command == "getStatus") {
        output(String(WiFi.status()), isSerialCommand);
      } else if (command == "getMode") {
        output(String(WiFi.getMode()), isSerialCommand);
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "esp") {  // ESP 8266
      if (command == "restart") {
        output("restarting ESP8266" , isSerialCommand);
        firebaseHandler.setString("command/inputCode", "~");
        ESP.restart();
      } else if (command == "getMillis") {
        output(String(millis()),isSerialCommand);
      } else if (command == "getFreeHeap") {
        output(String(ESP.getFreeHeap()), isSerialCommand);
      } else if (command == "getHeapFragmentation") {
        output(String(ESP.getHeapFragmentation()), isSerialCommand);
      } else if (command == "getMaxFreeBlockSize") {
        output(String(ESP.getMaxFreeBlockSize()), isSerialCommand);
      } else if (command == "getFlashChipSize") {
        output(String(ESP.getFlashChipSize()), isSerialCommand);
      } else if (command == "getFlashChipRealSize") {
        output(String(ESP.getFlashChipRealSize()), isSerialCommand);
      } else if (command == "getFreeSketchSpace") {
        output(String(ESP.getFreeSketchSpace()), isSerialCommand);
      } else if (command == "getSketchSize") {
        output(String(ESP.getSketchSize()), isSerialCommand);
      } else if (command == "setDelay") {
        output("esp delay start", isSerialCommand);
        unsigned long espDelayStartTime = millis();
        wait(params[0].toInt());
        output("esp delay ended", isSerialCommand);
      } else {
        output(unknownCommand, isSerialCommand);
      }

    } else if (target == "servo") {  // Servo
      if (command == "setMinAngle") {
        if (params[0].toInt() <= 180) {
          servoMaxAngle = params[0].toInt();
          eepromManager.putData(servoMaxAngleAddr, params[0].toInt());
          output("servo set max angle", isSerialCommand);
        } else {
          output("error: angle too large",  isSerialCommand);
        }
      } else if (command == "setOpenDelay") {
        if (params[0].toInt() <= 10000) {
          servoOpenDelay = params[0].toInt();
          eepromManager.putData(servoOpenDelayAddr, servoOpenDelay);
          output("servo set open delay", isSerialCommand);
        } else {
          output("error: delay too long", isSerialCommand);
        }
      } else if (command == "setCloseDelay") {
        if (params[0].toInt() >= 80) {
          servoCloseDelay = params[0].toInt();
          eepromManager.putData(servoCloseDelayAddr, servoCloseDelay);
          output("servo set close delay", isSerialCommand);
        } else {
          output("error: delay too fast", isSerialCommand);
        }
      } else if (command == "setAngle") {
        myServo.attach(servoPin, 500, 2500);
        myServo.write(params[0].toInt());
        delay(500);
        myServo.detach();
        output("servo set to current angle", isSerialCommand);
      } else if (command == enable) {
        bool enableServo2 = true;
        if (params[0] == trueVal) {
          eepromManager.putData(enableServoAddr, 1);
          enableServo = true;
        } else if (params[0] == trueVal) {
          eepromManager.putData(enableServoAddr, 0);
          enableServo = false;
        } else {
          output(wrongBoolValue, isSerialCommand);
          enableServo2 = false;
        }
        if (enableServo2) {
          output("servo permission updated", isSerialCommand);
        }
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "buzzer") {  // Buzzer
      if (command == "playNote") {
        int playNote = params[0].toInt();
        tone(buzzerPin, playNote);
        delay(params[1].toInt());
        noTone(buzzerPin);
        output("buzzer note complete running", isSerialCommand);
      } else if (command == enable) {
        bool enableBuzzer2 = true;
        if (params[0] == trueVal) {
          eepromManager.putData(enableBuzzerAddr, 1);
          enableBuzzer = true;
        } else if (params[0] == falseVal) {
          eepromManager.putData(enableBuzzerAddr, 0);
          enableBuzzer = false;
        } else {
          output("error: wrong value", isSerialCommand);
          enableBuzzer2 = false;
        }
        if (enableBuzzer2) {
          output("buzzer permission updated", isSerialCommand);
        }

      } else if (command == "playMelody") {
        if (params[0] == "pink_panther") {
          buzzerT(1);
        } else if (params[0] == "star_wars") {
          buzzerT(2);
        } else if (params[0] == "subway_surfers") {
          buzzerT(3);
        } else if (params[0] == "harry_potter") {
          buzzerT(4);
        } else if (params[0] == "the_small_world") {
          buzzerT(5);
        } else if (params[0] == "pac_man") {
          buzzerT(6);
        } else if (params[0] == "mulai_sistem") {
          buzzerT(7);
        } else if (params[0] == "peringatan") {
          buzzerT(8);
        }
        output("buzzer melody complete running", isSerialCommand);
      } else {
        output(unknownCommand, isSerialCommand);
      }

    } else if (target == "eeprom") {  // Eeprom
      if (command == "get") {
        int intValues = params[0].toInt();
        if (intValues != wifiSsidAddr && intValues != wifiPasswordAddr) {
          output(String(eepromManager.getData(params[0].toInt(), false)), isSerialCommand);
        } else {
          output(eepromManager.readString(params[0].toInt(), false), isSerialCommand);
        }
      } else if (command == "getAll") {
        String getAllResult = eepromManager.getAll(true);
        getAllResult.replace("\n", "\\n");
        Serial.println(getAllResult);
        output(getAllResult, isSerialCommand);
      } else if (command == "writeString") {
        if (params[0].toInt() == wifiSsidAddr || params[0].toInt() == wifiPasswordAddr && params[1].length() <= 30) {
          eepromManager.writeString(params[0].toInt(), params[1]);
          output("string data is saved", isSerialCommand);
        } else {
          output("error: destination address saves integer", isSerialCommand);
        }
      } else if (command == "writeInterger") {
        if (params[0].toInt() != wifiSsidAddr && params[0].toInt() != wifiPasswordAddr) {
          if (params[1].toInt() <= 99999) {
            eepromManager.putData(params[0].toInt(), params[1].toInt());
            output("interger data is saved", isSerialCommand);
          } else {
            output("error: number greater than 99999",  isSerialCommand);
          }
        } else {
          output("error: destination address saves string", isSerialCommand);
        }
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "led") {
      if (command == "state") {  // set led status
        if (params[0] == trueVal) {
          digitalWrite(ledPin, HIGH);
          output("led true", isSerialCommand);
        } else if (params[0] == trueVal) {
          digitalWrite(ledPin, LOW);
          output("led false",   isSerialCommand);
        }
      } else if (command == "effect") {
        if (params[0] == "fadeIn") {
          ledFadeIn();
          digitalWrite(ledPin, LOW);
          output("led fadeing in", isSerialCommand);
        } else if (params[0] == "fadeOut") {
          ledFadeOut();
          output("led fadeing out", isSerialCommand);
        }
      } else if (target == "led.enable") {
        bool enableLed2 = true;
        if (params[0] == trueVal) {
          eepromManager.putData(enableLedAddr, 1);
          enableLed = true;
        } else if (params[0] == falseVal) {
          eepromManager.putData(enableLedAddr, 0);
          enableLed = false;
        } else {
          output("error: wrong value", isSerialCommand);
          enableLed2 = false;
        }
        if (enableLed2) {
          output("led permission updated", isSerialCommand);
        }
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "pot") {
      if (command == enable) {
        bool enablePot2 = true;
        if (params[0] == trueVal) {
          eepromManager.putData(enablePotAddr, 1);
          enablePot = true;
        } else if (params[0] == falseVal) {
          eepromManager.putData(enablePotAddr, 0);
          enablePot = false;
        } else {
          output(wrongBoolValue, isSerialCommand);
          enablePot2 = false;
        }
        if (enablePot2) {
          output("pot permission updated", isSerialCommand);
        }
      } else if (command == "getPosition") {
        output(String(currentPos), isSerialCommand);
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "button") { // Button2
      if (command == "button.enable") {  // enable button
        bool enableButton2 = true;
        if (params[0] == trueVal) {
          eepromManager.putData(enableButtonAddr, 1);
          enableButton = true;
        } else if (params[0] == falseVal) {
          eepromManager.putData(enableButtonAddr, 0);
          enableButton = false;
        } else {
          output("error: wrong value", isSerialCommand);
          enableButton2 = false;
        }
        if (enableButton2) {
          output("button permission updated", isSerialCommand);
        }

      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "tiltSensor") {
      if (command == enable) {  // enable tilt sensor
        bool enableTiltSensor2 = true;
        if (params[0] == trueVal) {
          eepromManager.putData(enableTiltSensorAddr, 1);
          enableTiltSensor = true;
        } else if (params[0] == falseVal) {
          eepromManager.putData(enableTiltSensorAddr, 0);
          enableTiltSensor = false;
        } else {
          output("error: wrong value", isSerialCommand);
          enableTiltSensor2 = false;
        }
        if (enableTiltSensor2) {
          output("servo permission updated", isSerialCommand);
        }
      } else if (command == "getState") {
        if (tiltSensorState) {
          output(trueVal, isSerialCommand);
        } else {
          output(falseVal, isSerialCommand);
        }
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "ota") {
      if (command == "run") {
        if (params[0] == trueVal) {
          output("OTA start", isSerialCommand);
          otaHandler.start();
        } else if (params[0] == falseVal) {
          otaHandler.end();
          output("OTA ended", isSerialCommand);
        } else {
          output(wrongBoolValue, isSerialCommand);
        }
      } else {
        output(unknownCommand, isSerialCommand);
      }


    } else if (target == "webserver") {
      if (command == "run") {
        if (params[0] == trueVal) {
          webServerIsActive = true;
          output("webserver started", isSerialCommand);
        } else if (params[0] == falseVal) {
          webServerIsActive = false;
          output("webserver ended", isSerialCommand);
        } else {
          output(wrongBoolValue, isSerialCommand);
        }
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "serial") {
      if (command == "enablePrint") {
        codeMarkerPrint = (params[0] == oneStr);
      } else {
        output(unknownCommand, isSerialCommand);
      }
    } else if (target == "feedo") {
      if (command == "appLock") {
        bool parameter;
        if (params[0] == "true") {
          parameter = true;
        } else if (params[0] == "false") {
          parameter = false;
        }
        firebaseHandler.setBool("password/isActive", params[0] == "true");
      } else if (command == "setSchedule") {
        waktusBaru[params[0].toInt() + 1] = params[1];
      } else if (command == "setValveLoop") {
        katupsBaru[params[0].toInt() + 1] = params[1].toInt();
      } else if (command == "getLoopRate") {
        output(String(loopRateHz), isSerialCommand);
      } else if (command == "getLatestUpdate") {
        output(mobileLatestUp, isSerialCommand);
      } else if (command == "feeding") {
        servoKatup(params[0].toInt());
        output("valve finished running", isSerialCommand);
      } else if (command == "setOutput") {
        output(params[1], (params[0] != "app"));
      } else if (command == "getPing") {
        unsigned long pingStartTime = millis();
        String pingCode = String(random(0, 999));
        firebaseHandler.setString("/command/output", pingCode);
        if (firebaseHandler.getString("/command/output") == pingCode) {
          double ping = millis() - pingStartTime;
          output(String(ping), isSerialCommand);
        } else {
          output("error: ping failed", isSerialCommand);
        }

      }else {
        output(unknownCommand, isSerialCommand);
      }
    } else {
      output(unknownTarget, isSerialCommand);
    }
    firebaseHandler.setString("/command/inputCode", "~");
    return true;
  }
};
CommandHandler commandHandler;



// --------------------------------- functions ---------------------------------//



// EepromManager
//   servoMaxAngle(0),        // addr 0 - 4   (5 byte)
//   servoMinAngle(5),        // addr 5 - 10  (6 byte)
//   servoOpenDelay(11),      // addr 11 - 16 (6 byte)
//   servoCloseDelay(17),     // addr 17 - 22 (6 byte)
//   savesWifiSsid(23),       // addr 23 - 54 (32 byte)
//   savesWifiPassword(55),   // addr 55 - 86 (32 byte)
//   enableServo(87),         // addr 87 - 88 (2 byte)
//   enableBuzzer(91),        // addr 89 - 90 (2 byte)
//   enableLed(99),           // addr 91 - 92 (2 byte)
//   enablePot(103),          // addr 93 - 94 (2 byte)
//   enableButton(107),       // addr 95 - 96 (2 byte)
//   enableTiltSensor(111)    // addr 97 - 98 (2 byte)
// ;

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

  wifiSsid = eepromManager.readString(wifiSsidAddr, true);
  wifiPassword = eepromManager.readString(wifiPasswordAddr, true);
  wifiHandler.startConnect();
  timeClientHandler.start();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;


  //servoMaxAngle = accessEEPROM(servoMAAddress, -1);
  servoMinAngle = eepromManager.getData(servoMinAngleAddr, true);
  servoOpenDelay = eepromManager.getData(servoOpenDelayAddr, true);
  servoCloseDelay = eepromManager.getData(servoCloseDelayAddr, true);

  enableServo = (eepromManager.getData(enableServoAddr, true) != 0);
  enableBuzzer = (eepromManager.getData(enableBuzzerAddr, true) != 0);
  enableLed = (eepromManager.getData(enableLedAddr, true) != 0);
  enablePot = (eepromManager.getData(enablePotAddr, true) != 0);
  enableButton = (eepromManager.getData(enableButtonAddr, true) != 0);
  enableTiltSensor = (eepromManager.getData(enableTiltSensorAddr, true) != 0);

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
    serialMessage = Serial.readStringUntil('\n');
    if (serialMessage == "p") {
      serialPrint = !serialPrint;
    } else {
      isSerialCommand = true;
      commandHandler.execute(serialMessage);
    }
  }

  OOOOOOOOOO_codeMarker(6);

  // runtime tasks
  loopRate();
  otaHandler.autoHandle();
  button();
  tiltSensor();
  potentiometer();
  if (webServerIsActive) server.handleClient();
  if (currentTime == "00:00") ESP.deepSleepMax(); // deepsleep
  if ((fbdConnected = firebaseHandler.start() ) ) fbdConnected = Firebase.ready(); // cek apakah sudah signup firebase

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
      firebaseHandler.setFloat("/loopRate", loopRateHz);
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
          ifPrintln(defaultStr);
          ifPrintln("data ke " + String(i) + ":");
          ifPrintln("waktu: " + waktusBaru[i]);
          ifPrintln("katup: " + String(katupsBaru[i]));
          wait(200);
        }
      }
      ifPrintln(defaultStr);

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

      rawCommand = firebaseHandler.getString("/command/inputCode");
      if (rawCommand.length() > 0 && rawCommand != "~") {
        isSerialCommand = false;
        commandHandler.execute(rawCommand);
      }

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
  wifiHandler.autoCheck();
}