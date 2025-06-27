#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "ESP-CMD";
const char* password = "12345678";

ESP8266WebServer server(80);

void handleCommand() {
  String uri = server.uri();  // Contoh: /ESP/wifiMode/set
  Serial.println("URI: " + uri);

  // Pisahkan path
  int i1 = uri.indexOf('/', 1);
  int i2 = uri.indexOf('/', i1 + 1);
  int i3 = uri.indexOf('/', i2 + 1);

  String target1 = uri.substring(i1 + 1, i2);
  String target2 = uri.substring(i2 + 1, i3 > 0 ? i3 : uri.length());
  String command = (i3 > 0) ? uri.substring(i3 + 1) : "";

  // Ambil parameter
  String mode = server.hasArg("mode") ? server.arg("mode") : "";
  String timeStr = server.hasArg("time") ? server.arg("time") : "";
  String pinStr = server.hasArg("pin") ? server.arg("pin") : "";
  String state = server.hasArg("state") ? server.arg("state") : "";

  // Eksekusi command
  if (target1 == "ESP" && target2 == "wifiMode" && command == "set") {
    if (mode == "AP") {
      WiFi.mode(WIFI_AP);
      Serial.println("WiFi diubah ke AP mode");
      server.send(200, "text/plain", "WiFi mode: AP");
      return;
    } else if (mode == "STA") {
      WiFi.mode(WIFI_STA);
      Serial.println("WiFi diubah ke STA mode");
      server.send(200, "text/plain", "WiFi mode: STA");
      return;
    }
  }

  if (target1 == "ESP" && target2 == "led" && command == "set") {
    int pin = pinStr.toInt();
    if (pin > 0 && (state == "on" || state == "off")) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, state == "on" ? HIGH : LOW);
      server.send(200, "text/plain", "LED pin " + pinStr + " di-" + state);
      return;
    }
  }

  if (target1 == "ESP" && target2 == "info" && command == "get") {
    String info = "IP: " + WiFi.softAPIP().toString() + "\n";
    info += "SSID: " + String(ssid) + "\n";
    info += "Mode: " + String(WiFi.getMode() == WIFI_AP ? "AP" : "STA");
    server.send(200, "text/plain", info);
    return;
  }

  // Default jika tidak cocok
  server.send(404, "text/plain", "Perintah tidak ditemukan");
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  server.onNotFound(handleCommand);
  server.begin();
  Serial.println("Server siap menerima command!");
}

void loop() {
  server.handleClient();
}
