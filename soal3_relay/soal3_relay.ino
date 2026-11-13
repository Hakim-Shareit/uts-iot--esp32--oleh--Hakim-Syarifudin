/*
 * UTS Praktikum IoT - Soal 3: Pengendalian Sakelar Beban Daya Menggunakan Modul Relay
 * Relay IN -> GPIO 19. Beban (lampu) dipasang seri dengan sumber lewat terminal COM dan NO.
 * Pola: relay AKTIF 5 detik (lampu menyala), NONAKTIF 3 detik (lampu padam), berulang.
 */
#include <Arduino.h>

const int PIN_RELAY = 19;

// ---------- Logika pemicuan relay ----------
// Cara menentukan Active HIGH atau Active LOW:
//  1) Baca datasheet / label modul. Modul relay 1 kanal berwarna biru yang umum di pasaran
//     biasanya ACTIVE LOW (relay menarik bila IN = LOW).
//  2) Atau uji empiris: beri IN = HIGH lalu amati. Bila lampu menyala (COM-NO tersambung),
//     modulnya Active HIGH; bila padam dan baru menyala saat IN = LOW, modulnya Active LOW.
//  3) Pada simulator Wokwi: atribut "transistor" modul relay bernilai "npn" (default)
//     = Active HIGH, dan "pnp" = Active LOW.
// Setelan di bawah mengikuti modul Wokwi default (Active HIGH). Ubah ke true bila modul Anda
// Active LOW (atau bila lampu ternyata terbalik saat disimulasikan).
const bool RELAY_AKTIF_LOW = false;

const uint8_t LEVEL_RELAY_ON  = RELAY_AKTIF_LOW ? LOW  : HIGH;
const uint8_t LEVEL_RELAY_OFF = RELAY_AKTIF_LOW ? HIGH : LOW;

const unsigned long DURASI_ON_MS  = 5000;   // lampu menyala 5 detik
const unsigned long DURASI_OFF_MS = 3000;   // lampu padam 3 detik

bool relayAktif = false;
unsigned long waktuGanti = 0;

void setRelay(bool aktif) {
  relayAktif = aktif;
  digitalWrite(PIN_RELAY, aktif ? LEVEL_RELAY_ON : LEVEL_RELAY_OFF);
  Serial.println(aktif ? "Relay AKTIF  -> lampu MENYALA (5 detik)"
                       : "Relay MATI   -> lampu PADAM   (3 detik)");
}

void setup() {
  Serial.begin(115200);
  digitalWrite(PIN_RELAY, LEVEL_RELAY_OFF);   // tetapkan level mati SEBELUM pinMode
  pinMode(PIN_RELAY, OUTPUT);                 // agar relay tidak sempat "klik" saat boot
  setRelay(true);                             // mulai dengan relay aktif
  waktuGanti = millis();
}

void loop() {
  unsigned long durasi = relayAktif ? DURASI_ON_MS : DURASI_OFF_MS;
  if (millis() - waktuGanti >= durasi) {      // non-blocking: ganti status saat waktunya habis
    waktuGanti = millis();
    setRelay(!relayAktif);
  }
}
