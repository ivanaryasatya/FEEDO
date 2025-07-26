#include <EEPROM.h>
#include <Arduino.h>

class EepromManager {
public:
  int address;  // alamat EEPROM untuk menyimpan data


  // constructor: tentukan alamat saat bikin objek
  EepromManager(int addr) {
    address = addr;
  }

  // tulis satu byte ke EEPROM
  void writeByte(byte value) {
    EEPROM.write(address, value);
    EEPROM.commit(); // untuk ESP8266/ESP32, untuk Arduino biasa ga masalah
  }

  // baca satu byte dari EEPROM
  byte readByte() {
    return EEPROM.read(address);
  }

  // bisa juga bikin fungsi lain, misalnya simpan int:
  void writeInt(int value) {
    EEPROM.put(address, value);
    EEPROM.commit();
  }

  int readInt() {
    int value;
    EEPROM.get(address, value);
    return value;
  }
};

EepromManager data1(0);    // objek untuk alamat 0
EepromManager data2(10);   // objek untuk alamat 10 (misal simpan int)

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512); // kalau pakai ESP8266/ESP32 perlu begin, Arduino Uno ga perlu

  // tulis data ke EEPROM
  data1.writeByte(123);
  data2.writeInt(2025);

  // baca data dari EEPROM
  byte val1 = data1.readByte();
  int val2 = data2.readInt();

  Serial.print("Data byte dari EEPROM: ");
  Serial.println(val1);

  Serial.print("Data int dari EEPROM: ");
  Serial.println(val2);
}

void loop() {
  // tidak ada yang dilakukan di loop
}
