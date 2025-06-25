# FEEDO: IoT-Based Pet Feeder

ini update

**FEEDO** adalah proyek Internet of Things (IoT) yang dirancang untuk mengotomatiskan pemberian makanan hewan peliharaan menggunakan mikrokontroler **ESP8266**. Proyek ini mengintegrasikan berbagai komponen seperti motor servo untuk mendispensasi makanan, buzzer untuk peringatan, dan Firebase untuk manajemen database waktu nyata. Sistem ini dapat dikendalikan dan dipantau dari jarak jauh melalui aplikasi seluler.

## Fitur

- **Pemberian Makanan Otomatis**: Mendispensasi makanan pada waktu yang dijadwalkan.
- **Kontrol Jarak Jauh**: Mengendalikan pemberi makanan dari mana saja menggunakan aplikasi seluler.
- **Pemantauan Waktu Nyata**: Memantau status pemberian makanan dan kesehatan sistem melalui Firebase.
- **Peringatan Pengguna**: Peringatan untuk status pemberian makanan dan kesalahan sistem melalui notifikasi buzzer.
- **Sensor Tilt**: Mendeteksi jika pemberi makanan miring atau dipindahkan, memicu peringatan.
- **Kontrol Potensiometer**: Menyesuaikan jumlah makanan yang didispensasi berdasarkan input pengguna.

## Komponen yang Diperlukan

- **ESP8266**: Mikrokontroler untuk konektivitas Wi-Fi.
- **Motor Servo**: Untuk mendispensasi makanan hewan peliharaan.
- **Buzzer**: Untuk peringatan audio.
- **Sensor Tilt**: Untuk mendeteksi gerakan.
- **Potensiometer**: Untuk menyesuaikan pengaturan pemberian makanan.
- **LED**: Untuk umpan balik visual.
- **Tombol Tekan**: Untuk kontrol pemberian makanan manual.
- **Akun Firebase**: Untuk manajemen database waktu nyata.

## Konfigurasi Pin

| Komponen      | Nomor Pin |
|---------------|-----------|
| LED           | D3        |
| Motor Servo   | D5        |
| Tombol Tekan  | D2        |
| Buzzer        | D1        |
| Sensor Tilt   | D4        |
| Potensiometer | A0        |
| Pin Wakeup    | 16        |

## Persyaratan Perangkat Lunak

- **Arduino IDE**: Untuk pemrograman ESP8266.
- **Firebase Library untuk ESP8266**: Untuk interaksi database.
- **NTPClient Library**: Untuk sinkronisasi waktu.
- **Servo Library**: Untuk mengontrol motor servo.

## Langkah Instalasi

### 1. Siapkan Arduino IDE

- Instal **Arduino IDE** dari [situs resmi Arduino](https://www.arduino.cc/en/software).
- Tambahkan board ESP8266 ke Arduino IDE dengan pergi ke `File -> Preferences` dan menambahkan URL berikut ke "Additional Board Manager URLs":

    ```text
    http://arduino.esp8266.com/stable/package_esp8266com_index.json
    ```

- Instal board ESP8266 melalui `Tools -> Board -> Boards Manager`.

### 2. Instal Library yang Diperlukan

Instal library berikut melalui **Library Manager**:

- Firebase ESP Client
- NTPClient
- Servo

### 3. Kloning Repository

Kloning atau unduh repository yang berisi file proyek FEEDO.

### 4. Konfigurasi Kode

- Buka `FEEDO.ino` di Arduino IDE.
- Perbarui konstanta berikut dengan kredensial Wi-Fi dan Firebase Anda:

    ```cpp
    #define WIFI_SSID "SSID_WiFi_Anda"
    #define WIFI_PASSWORD "Kata_Sandi_WiFi_Anda"
    #define API_KEY "API_Key_Firebase_Anda"
    #define DATABASE_URL "URL_Database_Firebase_Anda"
    ```

### 5. Unggah Kode

- Hubungkan ESP8266 Anda ke komputer.
- Pilih board dan port yang benar di Arduino IDE.
- Unggah kode ke ESP8266.

## Penggunaan

1. **Nyalakan**: Hubungkan ESP8266 ke sumber daya.
2. **Koneksi ke Wi-Fi**: Perangkat akan mencoba terhubung ke jaringan Wi-Fi yang ditentukan.
3. **Aplikasi Seluler**: Gunakan aplikasi seluler untuk mengontrol pemberi makanan dan memantau statusnya.
4. **Pemberian Makanan Manual**: Tekan tombol untuk mendispensasi makanan secara manual.

## penyimpanan data EEPROM
- address 0 = servo max angle
- address 5 = servo min angle
- address 11 = servo open delay
- address 17 = servo close delay
- address 23 (23 - 54) = wifi ssid (panjang maksimal ssid 30 karakter + 1 null terminator)
- address 55 (55 - 86) = wifi password (panjang maksimal password 30 karakter + 1 null terminator)
- address 87 = servo enable
- address 88 = buzzer enable
- address 89 = led enable
- address 90 = potentiometer enable
- address 91 = button enable
- address 92 = tilt sensor enable

## Database Firebase
### 1. Struktur Database Firebase
Database Firebase diatur untuk mengelola jadwal pemberian makanan dan status sistem. Berikut adalah struktur database:
```txt
/isButton: boolean
/potValue: interger
/loopRate: interger
/espRestart: boolean
/password: interger
/command
  ├── config
  |       ├── 0
  |       |    ├── code: string
  |       |    ├── description: string
  |       |    ├── name: string
  |       ├── 1
  |       |    ├── code: string
  |       |    ├── description: string
  |       |    ├── name: string
  |       ├── 2
  |       |    ├── code: string
  |       |    ├── description: string
  |       |    ├── name: string
  |      ...
  ├── inputCode: string
  ├── output: string
/feeding
  ├── throwOut: boolean
  ├── value: integer
/mobileLatestUp: string
/buzzer
  ├── tone: integer
  ├── isBuzzer: boolean
/tiltSensor
  ├── isTiltSensor: boolean
  ├── waktu: string
/potValue: integer
/lastFood: string
/setting
  ├── 0
  |   ├── katup: int
  |   ├── waktu: string
  ├── 1
  |   ├── katup: int
  |   ├── waktu: string
  ├── 2
  |   ├── katup: int
  |   ├── waktu: string
 ...
```
  
### 2. Last Food
Last Food berfungsi untuk memberi indikator sumber, banyak , dan waktu makanan keluar.

#### 1. Format
Format data string: |source,value,currentTime
Example: |tmr,2,2025-01-02 12:07:03

#### 2. kode
- tmr = timer (dari firebase)
- fdg = feeding (dari firebase)
- pot = potentiometer (fisik)
- btn = button (fisik)

### Command Input and Output
command untuk memberi suatu perintah ke board ESP8266 berguna untuk maintenance dan pengetesan perangkat keras.
Format: /tujuan.perintah/permintaan value1, ...
Example: /wifi.begin IVAN titanasri

| Perintah                         | Fungsi                                  |
|----------------------------------|-----------------------------------------|
| `wifi.rssi`                      | Menampilkan kekuatan sinyal WiFi        |
| `wifi.localIp`                   | Menampilkan alamat IP lokal             |
| `wifi.macAdress`                 | Menampilkan alamat MAC                  |
| `wifi.begin [ssid] [password]`   | Mengganti jaringan WiFi                 |
| `esp.restart`                    | Me-restart ESP                          |
| `esp.delay [ms]`                 | Menghentikan looping program (ms)       |
| `esp.millis`                     | Waktu (microsecond) sejak ESP8266 menyala|
| `pot.value`                      | Mengecek posisi asli potentiometer           |
| `pot.currentPos`                 | mengecek posisi pilihan potetiometer         |
| `servo.setMaxAngle [angle]`      | Mengatur sudut maksimum servo           |
| `servo.setMinAngle [angle]`      | Mengatur sudut minimum servo            |
| `servo.setCurrentAngle [angle]`  | Mengatur sudut servo saat ini           |
| `servo.katup [value]`            | Menjalankan katup servo                 |
| `servo.state [true/false] `      | Untuk memberhentikan fungsi menggerakkan servo|
| `servo.setOpenDelay [ms]`        | Mengatur delay atau waktu servo terbuka |
| `servo.setCloseDelay [ms]`       | Mengatur delay atau waktu servo tertutup|
| `led.state [true/false]`         | Menyalakan LED                          |
| `led.effect [fedeIn/fadeOut]`    | menjalankan efek led                    |
| `buzzer.tone [tone]`             | Memainkan nada pada buzzer              |
| `ntp.completeTime`               | Waktu sekarang dari server NTP          |
| `eeprom.read [address]`          | Membaca data variable yang disimpan di EEPROM|
| `eeprom.writeInterger [address] [number]` | menulis angka ke eeprom        |
| `eeprom.writeString [address] [text]` | menulis teks ke eeprom             |
| `eeprom.get [address]`           | Membaca data variable yang disimpan di EEPROM
| `eeprom.writeInterger [address] [number]` | menulis angka ke eeprom        |
| `eeprom.writeString [address] [text]` |  menulis teks ke eeprom            |
| `esp.getFreeHeap`                | Melihat RAM (heap) tersisa (byte)       |
| `esp.getHeapFragmentation`       | Melihat tingat fregmentasi heap (persentase)|
| `esp.getMaxFreeBlockSize`        | Melihat blok terbsear dari heap yang tersisa|
| `esp.getFlashChipSize`           | Melihat ukuran total chip flash (byte)  |
| `esp.getFlashChipRealSize`       | Melihat ukuran aktual chip flash (byte) |
| `esp.getFreeSketchSpace`         | Melihat ruang kosong untuk kode baru (byte)|
| `esp.getSketchSize`              | Melihat ukran firmware yang sedang berjalan (byte)|
| `servo.enable [true/false]`      | Mengaktifkan atau menonaktifkan fungsi servo|
| `buzzer.enable [true/false]`     | Mengaktifkan atau menonaktifkan fungsi buzzer|
| `led.enable [true/false]`        | Mengaktifkan atau menonaktifkan fungsi led|
| `pot.enable [true/false]`        | Mengaktifkan atau menonaktifkan fungsi potentiometer|
| `button.enable [true/false]`     | Mengaktifkan atau menonaktifkan fungsi button|
| `tiltSensor.enable [true/false]` | Mengaktifkan atau menonaktifkan fungsi tilt sensor|

## Pemecahan Masalah

- **Masalah Koneksi Wi-Fi**: Pastikan kredensial Wi-Fi benar dan ESP8266 berada dalam jangkauan.
- **Kesalahan Firebase**: Periksa API key dan URL database Firebase untuk akurasi.
- **Motor Servo Tidak Berfungsi**: Verifikasi koneksi motor servo dan pasokan daya.

# Banyak Makanan Kucing Bersarkan kriteria

Berikut adalah daftar jumlah makanan kucing dalam sehari berdasarkan usia dan berat badan:
### 1. Usia 2–3 bulan (kitten baru disapih)
- Berat 500 g – 1 kg → 30–50 g per hari
- Berat 1–2 kg → 50–80 g per hari
- Frekuensi makan: 4–5 kali sehari
### 2. Usia 4–6 bulan
- Berat 2–3 kg → 80–100 g per hari
- Berat 3–4 kg → 100–120 g per hari
- Frekuensi makan: 3–4 kali sehari
### 3. Usia 7–12 bulan
- Berat 3–4 kg → 100–120 g per hari
- Berat 4–5 kg → 120–140 g per hari
- Frekuensi makan: 2–3 kali sehari
### 4. Kucing Dewasa (1 tahun ke atas)
- Berat 3–4 kg → 70–90 g per hari
- Berat 4–5 kg → 90–110 g per hari
- Frekuensi makan: 2 kali sehari
### Catatan:
- Makanan bisa berupa dry food (makanan kering) atau kombinasi dengan wet food (makanan basah).
- Pastikan selalu ada air minum bersih.
- Porsi bisa disesuaikan dengan aktivitas dan kondisi kucing.
