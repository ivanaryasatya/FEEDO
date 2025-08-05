# Changelog

Semua perubahan penting pada proyek ini akan didokumentasikan di sini

## [v1.2.5] - 02-08-2025
### Added
- Set mode ke ap sta lagi saat menyalakn mode ap
- tambahkan beberapa pemanggilan ap

### Fixed
- Perbaiki AP yang tidak dapat muncul sama sekali (masih hilang kadang kadang).

## [v1.2.6] - 03-08-2025
### Added
- command executeCommand untuk eksekusi perintah dengan perintah
- input serial "l" untuk menampilkan looprateHZ
- tambahkan pengecekan apakah rawCommand == format command
- const pada beberapa variabel logika parse command

### Changed
- ubah beberapa var ke const
- ubah int menjadi byte

## [v1.2.7] - 05-08-2025
### Added
- panggil webserver.handle() di loop dan sebagai runtime


### Changed
- hapus const untuk string di pemrosesan command
- ubah byte ke int seperti semula di pemrosesan command
- esp8266 control panel -> feedo control panel
- command playNote -> playTone
- hapus semua pemanggilan func untuk mematikan mode AP








<!-- TEMPLATE
## [v1.] - --2025
### Added



### Changed







