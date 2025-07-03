#include <Arduino.h>
#include <vector>

// format: target.command parameter1 parameter2 ...
// "led.builtin on 1000"

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

// Fungsi eksekusi perintah
void executeCommand() {
  Serial.println("=== EXECUTING COMMAND ===");
  Serial.println("Target : " + target);
  Serial.println("Command: " + command);
  for (int i = 0; i < params.size(); i++) {
    Serial.println("Param[" + String(i) + "]: " + params[i]);
  }

  // Contoh command: led.builtin on 1000
  if (target == "led" && command == "builtin" && params.size() >= 1) {
    String state = params[0];
    int delayTime = (params.size() >= 2) ? params[1].toInt() : 0;

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, (state == "on") ? LOW : HIGH);  // LOW = ON (active low LED)

    Serial.println("LED_BUILTIN: " + state);
    if (delayTime > 0) {
      delay(delayTime);
      digitalWrite(LED_BUILTIN, HIGH);  // Matikan setelah delay
      Serial.println("LED dimatikan setelah " + String(delayTime) + " ms");
    }
  } else {
    Serial.println("Perintah tidak dikenali.");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Siap menerima command Serial...");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    parseCommand(input);
    executeCommand();
  }
}
