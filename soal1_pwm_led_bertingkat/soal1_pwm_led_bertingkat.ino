/*
 * UTS Praktikum IoT - Soal 1: Kontrol Kecerahan LED Bertingkat (PWM & GPIO)
 * LED         : GPIO 16 (via resistor 220 ohm ke GND)
 * Push Button : GPIO 4  (kaki lain ke GND, memakai INPUT_PULLUP internal)
 *
 * Urutan kecerahan tiap tombol ditekan:
 *   Padam (0%) -> Redup (30%) -> Sedang (70%) -> Terang (100%) -> Padam (0%) ...
 */
#include <Arduino.h>

// ---------- Pin ----------
const int PIN_LED    = 16;
const int PIN_TOMBOL = 4;

// ---------- Konfigurasi PWM ----------
const uint32_t PWM_FREKUENSI = 5000;   // 5 kHz (tidak terlihat berkedip)
const uint8_t  PWM_RESOLUSI  = 8;      // 8 bit -> duty 0..255

// Daftar tingkat kecerahan (persen) sesuai soal
const uint8_t TINGKAT_PERSEN[] = {0, 30, 70, 100};
const uint8_t JUMLAH_TINGKAT   = sizeof(TINGKAT_PERSEN) / sizeof(TINGKAT_PERSEN[0]);

// ---------- Debouncing ----------
const unsigned long WAKTU_DEBOUNCE_MS = 50;   // sinyal harus stabil minimal 50 ms

// ---------- Abstraksi API LEDC (beda antara Arduino-ESP32 core 2.x dan 3.x) ----------
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define PWM_SIAPKAN()      ledcAttach(PIN_LED, PWM_FREKUENSI, PWM_RESOLUSI)
  #define PWM_TULIS(duty)    ledcWrite(PIN_LED, duty)
#else
  const int KANAL_PWM = 0;
  #define PWM_SIAPKAN()      do { ledcSetup(KANAL_PWM, PWM_FREKUENSI, PWM_RESOLUSI); \
                                  ledcAttachPin(PIN_LED, KANAL_PWM); } while (0)
  #define PWM_TULIS(duty)    ledcWrite(KANAL_PWM, duty)
#endif

// ---------- Variabel status ----------
uint8_t tingkatSekarang = 0;              // indeks pada TINGKAT_PERSEN
int bacaanTerakhir      = HIGH;           // bacaan mentah sebelumnya
int statusStabil        = HIGH;           // status tombol setelah debounce
unsigned long waktuPerubahan = 0;         // kapan bacaan mentah terakhir berubah

// Ubah persen (0-100) menjadi duty cycle sesuai resolusi PWM, lalu keluarkan ke LED
void aturKecerahan(uint8_t persen) {
  uint32_t dutyMaks = (1UL << PWM_RESOLUSI) - 1;      // 255 untuk 8 bit
  uint32_t duty     = (persen * dutyMaks) / 100;      // 30% -> 76, 70% -> 178
  PWM_TULIS(duty);
  Serial.printf("Tingkat %d -> kecerahan %d%% (duty %lu/%lu)\n",
                tingkatSekarang, persen, duty, dutyMaks);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TOMBOL, INPUT_PULLUP);   // ditekan = LOW
  PWM_SIAPKAN();
  aturKecerahan(TINGKAT_PERSEN[tingkatSekarang]);   // awal: padam
}

void loop() {
  int bacaan = digitalRead(PIN_TOMBOL);

  // Setiap bacaan berubah, mulai hitung ulang waktu stabil
  if (bacaan != bacaanTerakhir) {
    waktuPerubahan = millis();
    bacaanTerakhir = bacaan;
  }

  // Bila sudah stabil lebih dari batas debounce dan berbeda dari status sebelumnya
  if ((millis() - waktuPerubahan) > WAKTU_DEBOUNCE_MS && bacaan != statusStabil) {
    statusStabil = bacaan;

    if (statusStabil == LOW) {                      // hanya saat tombol DITEKAN
      tingkatSekarang = (tingkatSekarang + 1) % JUMLAH_TINGKAT;
      aturKecerahan(TINGKAT_PERSEN[tingkatSekarang]);
    }
  }
}
