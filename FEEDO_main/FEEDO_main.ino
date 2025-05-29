#include <Arduino.h>
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <f:\ivan\Arduino\libraries\2820675-bbe995aa22826a8fbbb6b56ccd56513f9db6cb00\pitches.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

String wifiSsid = "";
String wifiPassword = "";
#define API_KEY "AIzaSyALzv1N1Kdh84U_lhwgb3jXlGSy-9EWMyo"
#define DATABASE_URL "https://feedo-39725-default-rtdb.firebaseio.com/"

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

int
  potPin = A0,
  lastPos = 0,
  sampleSize = 30,
  currentBuzzerTone = 6,
  totalTime = 0,
  loopCount = 0,
  lastLastPos = 0,
  currentPos = 0,
  servoMaxAngle = 110,
  servoMinAngle = 0,
  servoOpenDelay = 300,
  servoCloseDelay = 800,

  servoMAAddress = 0,         // addr 0 - 4   (5 byte)
  servoMIAAddress = 5,        // addr 5 - 10  (6 byte)
  servoODAddress = 11,        // addr 11 - 16 (6 byte)
  servoCDAddress = 17,        // addr 17 - 22 (6 byte)
  ssidAddress = 23,           // addr 23 - 54 (32 byte)
  passwordAddress = 55,       // addr 55 - 86 (32 byte)
  enableServoAddr = 87,       // addr 87 - 88 (2 byte)
  enableBuzzerAddr = 91,      // addr 89 - 90 (2 byte)
  enableLedAddr = 99,         // addr 91 - 92 (2 byte)
  enablePotAddr = 103,         // addr 93 - 94 (2 byte)
  enableButtonAddr = 107,      // addr 95 - 96 (2 byte)
  enableTiltSensorAddr = 111;  // addr 97 - 98 (2 byte)

int tones[] = { 300, 400, 500, 600, 700, 800, 900, 1000 };
int eepromAddress[] = { servoMAAddress, servoMIAAddress, servoODAddress, servoCDAddress, ssidAddress, passwordAddress, enableServoAddr, enableBuzzerAddr, enableLedAddr, enablePotAddr, enableButtonAddr, enableTiltSensorAddr };

bool
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
  enableServo = true,
  enableBuzzer = true,
  enableLed = true,
  enablePot = true,
  enableButton = true,
  enableTiltSensor = true;
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
  currentTimeM = 0;
String
  mobileLastUp = "",
  mobileLatestUp = "",
  command = "",
  currentTime = "99:99",
  completeTime = "",
  currentMonthStr,
  monthDayStr;

String waktusBaru[24];
int katupsBaru[24];
struct CommandData {
  String cmd;
  String command;
  String values[10];  // Maksimal 10 value
  int valueCount;
};
CommandData cmd;

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
  wifiConnection(wifiSsid, wifiPassword);
  if (!isWifiConnect) {
    wifiSsid = "ADAN";
    wifiPassword = "titanasri";
    wifiConnection(wifiSsid, wifiPassword);
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (isWifiConnect) {
    if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("firebase signup berhasil");
      signupOK = true;
    } else {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    timeClient.begin();
    timeClient.setTimeOffset(25200);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // perbaiki, harusnya 2 address untuk enable karen include 0 karena pakai eeprom.put();
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

  // buzzer start
  if (buzzerStart) {
    buzzerT(7);
    buzzerStart = false;
  }

  // timeClient update
  timeClient.update();
  if (millis() - currentTimeM >= 1000) {

    time_t epochTime = timeClient.getEpochTime();  // Get epoch time
    struct tm* ptm = gmtime((time_t*)&epochTime);  // Convert epoch time to struct tm

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

    currentTimeM = millis();
  }

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
    }
  }

  // ESP deep sleep
  if (currentTime == "00:00") {
    ESP.deepSleepMax();
  }

  button(completeTime);
  tiltSensor(completeTime);
  potentiometer(currentTime, completeTime);

  // cek apakah firebase siap
  if (Firebase.ready() && signupOK) {
    fbdConnected = true;
  } else {
    fbdConnected = false;
  }

  // firebase database
  if (isWifiConnect && fbdConnected) {
    loopRate();

    // mobile app update
    if (millis() - prevUpMillis >= 3000) {
      if (Firebase.RTDB.getBool(&fbdo, "/mobileLatestUp")) {
        mobileLatestUp = fbdo.stringData();
      }
      if (mobileLatestUp != mobileLastUp) {
        mobileLastUp = mobileLatestUp;
        firebaseUpdate = true;
      }
      prevUpMillis = millis();
    }

    // last start
    if (lastStart) {
      if (Firebase.RTDB.setString(&fbdo, "/lastStart", completeTime)) {
        lastStart = false;
      }
    }

    if (firebaseUpdate) {
      Serial.println("firebase update");

      // update tone buzzer
      if (Firebase.RTDB.getInt(&fbdo, "/buzzer/tone")) {
        currentBuzzerTone = fbdo.intData();
        ifPrintln("buzzer fb get int");
      } else {
        fbdoError();
      }

      // perintah aktivasi buzzer
      if (Firebase.RTDB.getBool(&fbdo, "/buzzer/isBuzzer")) {
        buzzerAct = fbdo.boolData();
        ifPrintln("buzzer fb get bool");
        if (buzzerAct) {
          buzzerT(currentBuzzerTone);
          if (Firebase.RTDB.setBool(&fbdo, "/buzzer/isBuzzer", false)) {
            ifPrintln("fb set isBuzzer false");
          } else {
            fbdoError();
          }
        }
      } else {
        fbdoError();
      }

      // restart
      if (Firebase.RTDB.getBool(&fbdo, "/espRestart")) {
        bool restart = fbdo.boolData();
        if (restart) {
          ifPrintln("ESP restart true");
          if (Firebase.RTDB.setBool(&fbdo, "/espRestart", false)) {
            ifPrintln("fb set espRestart false\nrestarting ESP8266...");
          } else {
            fbdoError();
          }
          delay(1000);
          ESP.restart();
        }
      } else {
        fbdoError();
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
      if (Firebase.RTDB.getBool(&fbdo, "/feeding/throwOut")) {
        bool throwOut = fbdo.boolData();
        if (Firebase.RTDB.getInt(&fbdo, "/feeding/value")) {
          int feedValue = fbdo.intData();
          if (throwOut && feedValue != 0 && feedValue <= 5) {
            Serial.println("servo throwOut");
            servoKatup(feedValue);
            lastFood("fdg", feedValue, completeTime);
            if (Firebase.RTDB.setBool(&fbdo, "/feeding/throwOut", false)) {
              ifPrintln("fb set throwOut false");
            } else {
              fbdoError();
            }
          }
        } else {
          fbdoError();
        }
      } else {
        fbdoError();
      }

      // command
      if (Firebase.RTDB.getString(&fbdo, "/command/inputCode")) {
        String command = fbdo.stringData();
        cmd = parseCommand(command);
        String parseCommand = cmd.command;
        String cmdValue1 = cmd.values[0];
        String cmdValue2 = cmd.values[1];


        for (int i = 0; i < cmd.valueCount; i++) {
          Serial.println("value " + String(i + 1) + ": " + cmd.values[i]);
        }

        if (command.indexOf('.') != -1) {
          Serial.println("command: " + cmd.command);

          if (command == "wifi.rssi") {  // Kekuatan sinyal WiFi
            long rssi = WiFi.RSSI();
            fbdCommandOutput(String(rssi));
          } else if (command == "wifi.localIp") {  // Alamat IP lokal
            fbdCommandOutput(WiFi.localIP().toString());
          } else if (command == "wifi.macAddress") {  // MAC Address
            fbdCommandOutput(WiFi.macAddress());
          } else if (command == "esp.restart") {  // restart
            fbdCommandOutput("restarting board");
            if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
            ESP.restart();
          } else if (command == "esp.millis") {  // millis
            fbdCommandOutput(String(millis()));
          } else if (command == "ntp.completeTime") {  // waktu NTP
            fbdCommandOutput(completeTime);
          } else if (command == "pot.value") {  // pot value
            fbdCommandOutput(String(analogRead(potPin)));
          } else if (command == "pot.currentPos") {  // pot current pos
            fbdCommandOutput(String(currentPos));
          } else if (command == "esp.getFreeHeap") {  // Free Heap
            fbdCommandOutput(String(ESP.getFreeHeap()));
          } else if (command == "esp.getHeapFragmentation") {  // Fragmentation
            fbdCommandOutput(String(ESP.getHeapFragmentation()));
          } else if (command == "esp.getMaxFreeBlockSize") {  // FreeBlockSize
            fbdCommandOutput(String(ESP.getMaxFreeBlockSize()));
          } else if (command == "esp.getFlashChipSize") {  // FlashChipSize
            fbdCommandOutput(String(ESP.getFlashChipSize()));
          } else if (command == "esp.getFlashChipRealSize") {  // FlashChippReal
            fbdCommandOutput(String(ESP.getFlashChipRealSize()));
          } else if (command == "esp.getFreeSketchSpace") {  // FreeSketchS
            fbdCommandOutput(String(ESP.getFreeSketchSpace()));
          } else if (command == "esp.getSketchSize") {  // SketchSize
            fbdCommandOutput(String(ESP.getSketchSize()));
          } else if (parseCommand == "wifi.begin") {  // ganti wifi
            if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
            if (cmdValue1.length() >= 30 || cmdValue2.length() >= 30) {
              fbdCommandOutput("ssid atau password lebih dari 30 huruf");
              return;
            }
            WiFi.disconnect();
            wait(1000);
            wifiConnection(cmdValue1, cmdValue2);

            if (WiFi.status() == WL_CONNECTED) {
              fbdCommandOutput("new wifi connected");
              saveStringToEEPROM(ssidAddress, cmdValue1);
              saveStringToEEPROM(passwordAddress, cmdValue2);
            } else {
              wifiConnection(wifiSsid, wifiPassword);
              fbdCommandOutput("new wifi not connected, connected to last wifi connection");
            }
          } else if (parseCommand == "esp.delay") {  // delay
            fbdCommandOutput("esp delay start");
            wait(cmdValue1.toInt());
            fbdCommandOutput("esp delay ended");
          } else if (parseCommand == "servo.setMaxAngle") {  // servo max angle
            if (cmdValue1.toInt() <= 180) {
              servoMaxAngle = cmdValue1.toInt();
              accessEEPROM(servoMAAddress, cmdValue1.toInt());
              fbdCommandOutput("servo set max angle");
            } else {
              fbdCommandOutput("error: angle too large");
            }
          } else if (parseCommand == "servo.setMinAngle") {  // servo min angle
            if (cmdValue1.toInt() >= 0) {
              servoMinAngle = cmdValue1.toInt();
              accessEEPROM(servoMIAAddress, cmdValue1.toInt());
              delay(1000);
              fbdCommandOutput("servo set min angle");
            } else {
              fbdCommandOutput("error: angle too small");
            }

          } else if (parseCommand == "servo.setOpenDelay") {
            if (cmdValue1.toInt() <= 10000) {
              servoOpenDelay = cmdValue1.toInt();
              accessEEPROM(servoODAddress, servoOpenDelay);
              fbdCommandOutput("servo set open delay");
            } else {
              fbdCommandOutput("error: delay too long");
            }


          } else if (parseCommand == "servo.setCloseDelay") {
            if (cmdValue1.toInt() >= 80) {
              servoCloseDelay = cmdValue1.toInt();
              accessEEPROM(servoCDAddress, cmdValue1.toInt());
              fbdCommandOutput("servo set close delay");
            } else {
              fbdCommandOutput("error: delay too fast");
            }
          } else if (parseCommand == "servo.katup") {  // katup
            servoKatup(cmdValue1.toInt());
            fbdCommandOutput("valve finished running");
          } else if (parseCommand == "servo.setAngle") {  // set angle
            myServo.attach(servoPin, 500, 2500);
            myServo.write(cmdValue1.toInt());
            delay(500);
            myServo.detach();
            fbdCommandOutput("servo set to current angle");
          } else if (parseCommand == "buzzer.tone") {  // buzzer tone
            buzzerT(cmdValue1.toInt());
            fbdCommandOutput("buzzer tone complete running");
          } else if (parseCommand == "eeprom.get") {  // get EEPROM
            int intValues = cmdValue1.toInt();
            if (intValues != ssidAddress && intValues != passwordAddress) {
              fbdCommandOutput(String(accessEEPROM(cmdValue1.toInt(), -1)));
            } else {
              fbdCommandOutput(readStringFromEEPROM(cmdValue1.toInt()));
            }

          } else if (parseCommand == "eeprom.getAll") {

            String getAllResult = getAllEeprom();
            getAllResult.replace("\n", "\\n");
            Serial.println(getAllResult);
            fbdCommandOutput(getAllResult);

          } else if (parseCommand == "led.state") {  // set led status
            if (cmdValue1 == "true") {
              digitalWrite(ledPin, HIGH);
              fbdCommandOutput("led true");
            } else if (cmdValue1 == "false") {
              digitalWrite(ledPin, LOW);
              fbdCommandOutput("led false");
            }
          } else if (parseCommand == "led.effect") {  // efel led
            if (cmdValue1 == "fadeIn") {
              ledFadeIn();
              digitalWrite(ledPin, LOW);
              fbdCommandOutput("led fadeing in");
            } else if (cmdValue1 == "fadeOut") {
              ledFadeOut();
              fbdCommandOutput("led fadeing out");
            }
          } else if (parseCommand == "eeprom.writeString") {  // write string to eeprom
            if (cmdValue1.toInt() == ssidAddress || cmdValue1.toInt() == passwordAddress && cmdValue2.length() <= 30) {
              saveStringToEEPROM(cmdValue1.toInt(), cmdValue2);
              fbdCommandOutput("string data is saved");
            } else {
              fbdCommandOutput("error: destination address saves int");
            }
          } else if (parseCommand == "eeprom.writeInterger") {  // write interger to eeprom
            if (cmdValue1.toInt() != ssidAddress && cmdValue1.toInt() != passwordAddress) {
              if (cmdValue2.toInt() <= 99999) {
                accessEEPROM(cmdValue1.toInt(), cmdValue2.toInt());
                fbdCommandOutput("interger data is saved");
              } else {
                fbdCommandOutput("error: number greater than 99999");
              }
            } else {
              fbdCommandOutput("error: destination address saves string");
            }
          } else if (parseCommand == "servo.enable") {  // enable servo
            bool enableServo2 = true;
            if (cmdValue1 == "true") {
              accessEEPROM(enableServoAddr, 1);
              enableServo = true;
            } else if (cmdValue1 == "false") {
              accessEEPROM(enableServoAddr, 0);
              enableServo = false;
            } else {
              fbdCommandOutput("error: wrong value");
              enableServo2 = false;
            }
            if (enableServo2) {
              fbdCommandOutput("servo permission updated");
            }

          } else if (parseCommand == "buzzer.enable") {  // enable buzzer
            bool enableBuzzer2 = true;
            if (cmdValue1 == "true") {
              accessEEPROM(enableBuzzerAddr, 1);
              enableBuzzer = true;
            } else if (cmdValue1 == "false") {
              accessEEPROM(enableBuzzerAddr, 0);
              enableBuzzer = false;
            } else {
              fbdCommandOutput("error: wrong value");
              enableBuzzer2 = false;
            }
            if (enableBuzzer2) {
              fbdCommandOutput("buzzer permission updated");
            }

          } else if (parseCommand == "led.enable") {  // enable led
            bool enableLed2 = true;
            if (cmdValue1 == "true") {
              accessEEPROM(enableLedAddr, 1);
              enableLed = true;
            } else if (cmdValue1 == "false") {
              accessEEPROM(enableLedAddr, 0);
              enableLed = false;
            } else {
              fbdCommandOutput("error: wrong value");
              enableLed2 = false;
            }
            if (enableLed2) {
              fbdCommandOutput("led permission updated");
            }
          } else if (parseCommand == "pot.enable") {  // enable potentometer
            bool enablePot2 = true;
            if (cmdValue1 == "true") {
              accessEEPROM(enablePotAddr, 1);
              enablePot = true;
            } else if (cmdValue1 == "false") {
              accessEEPROM(enablePotAddr, 0);
              enablePot = false;
            } else {
              fbdCommandOutput("error: wrong value");
              enablePot2 = false;
            }
            if (enablePot2) {
              fbdCommandOutput("pot permission updated");
            }
          } else if (parseCommand == "button.enable") {  // enable button
            bool enableButton2 = true;
            if (cmdValue1 == "true") {
              accessEEPROM(enableButtonAddr, 1);
              enableButton = true;
            } else if (cmdValue1 == "false") {
              accessEEPROM(enableButtonAddr, 0);
              enableButton = false;
            } else {
              fbdCommandOutput("error: wrong value");
              enableButton2 = false;
            }
            if (enableButton2) {
              fbdCommandOutput("button permission updated");
            }

          } else if (parseCommand == "tiltSensor.enable") {  // enable tilt sensor
            bool enableTiltSensor2 = true;
            if (cmdValue1 == "true") {
              accessEEPROM(enableTiltSensorAddr, 1);
              enableTiltSensor = true;
            } else if (cmdValue1 == "false") {
              accessEEPROM(enableTiltSensorAddr, 0);
              enableTiltSensor = false;
            } else {
              fbdCommandOutput("error: wrong value");
              enableTiltSensor2 = false;
            }
            if (enableTiltSensor2) {
              fbdCommandOutput("servo permission updated");
            }

          } else {
            fbdCommandOutput("unknown input code, check the command and try again.");
          }
          if (Firebase.RTDB.setString(&fbdo, "/command/inputCode", "~")) {}
        } else {
          if (command != "~") {
            fbdCommandOutput("unknown input code, check the command and try again.");
          }
        }
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

  handleWiFi(wifiSsid, wifiPassword);
  yield();
}

// Example: source = pot / btn, value = 0-3
// Example: |pot,3,2025-02-03 12:30:00
void lastFood(String source, int value, String currentTime) {
  if (isWifiConnect == false && fbdConnected == false) {
    return;
  }
  FirebaseJsonArray fja;
  if (Firebase.RTDB.getString(&fbdo, "/lastFood")) {
    const int maxDataLimit = 50;
    const int formatLength = 26;
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
  }
}

// servo
void servoKatup(int perulanganKatup) {
  Serial.println("servo - katup" + perulanganKatup);
  if (perulanganKatup <= 5) {
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

  /*
  Serial.print("currentp ");
  Serial.println(currentPos);
  Serial.print("over ");
  Serial.println(over);
  */
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

// handle wifi otomatis
void handleWiFi(String wifi, String password) {
  if (WiFi.status() != WL_CONNECTED) {
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

      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("connected with IP: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("failed to connect wifi");
      }
    }
  } else {
    isWifiConnect = true;
  }
}

// koneksi ke wifi
void wifiConnection(String ssid, String password) {
  int timeout = 20;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
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
    Serial.print("connected with IP: ");
    Serial.println(WiFi.localIP());
    isWifiConnect = true;
  } else {
    Serial.println("failed to connect wifi");
    isWifiConnect = false;
  }
}

//pengolahan teks
CommandData parseCommand(String input) {
  CommandData result;
  result.valueCount = 0;

  // Mencari posisi pertama dari spasi
  int firstSpace = input.indexOf(' ');

  // Jika tidak ada spasi, berarti hanya perintah tanpa value
  if (firstSpace == -1) {
    result.command = input;
    return result;
  }

  // Ambil perintah
  result.command = input.substring(0, firstSpace);

  // Ambil sisanya sebagai value string
  String valuesString = input.substring(firstSpace + 1);

  // Pisahkan value berdasarkan spasi
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
  Serial.println("firebase error" + fbdo.errorReason());
}

// command ouput firebase
void fbdCommandOutput(String oCommand) {
  if (Firebase.RTDB.setString(&fbdo, "/command/output", oCommand)) {
    Serial.println("fb set command output: " + oCommand);
  } else {
    fbdoError();
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
























// kosong