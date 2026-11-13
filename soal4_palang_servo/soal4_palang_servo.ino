/*
 * UTS Praktikum IoT - Soal 4: Mekanisme Palang Otomatis Berbasis Motor Servo
 * Servo SG90 sinyal -> GPIO 18
 * HC-SR04 Trig -> GPIO 5, Echo -> GPIO 17
 *
 * Jarak objek < 10 cm  -> servo 90 derajat (palang TERBUKA)
 * Jarak objek >= 10 cm -> servo 0 derajat  (palang TERTUTUP)
 *
 * Library: ESP32Servo
 */
#include <Arduino.h>
#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_TRIG  = 5;
const int PIN_ECHO  = 17;

const float JARAK_BUKA_CM = 10.0;    // batas deteksi objek
const int   SUDUT_BUKA    = 90;
const int   SUDUT_TUTUP   = 0;

const unsigned long INTERVAL_UKUR_MS  = 100;    // ukur jarak 10 kali per detik
const unsigned long INTERVAL_CETAK_MS = 500;    // cetak log tiap 0,5 detik
const unsigned long TIMEOUT_ECHO_US   = 30000;  // 30 ms = sekitar 5 m (tanpa echo = di luar jangkauan)

Servo servoPalang;
bool palangTerbuka = false;
unsigned long waktuUkur = 0, waktuCetak = 0;

// Ukur jarak (cm). Mengembalikan -1 bila tidak ada pantulan dalam batas waktu.
float ukurJarakCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);          // pulsa pemicu 10 mikrodetik
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long durasi = pulseIn(PIN_ECHO, HIGH, TIMEOUT_ECHO_US);
  if (durasi == 0) return -1;
  // Kecepatan suara = 0,0343 cm/us; dibagi 2 karena gelombang pergi-pulang
  return durasi * 0.0343f / 2.0f;
}

// Gerakkan servo hanya bila status berubah (mengurangi jitter dan beban)
void aturPalang(bool buka) {
  if (buka == palangTerbuka) return;
  palangTerbuka = buka;
  servoPalang.write(buka ? SUDUT_BUKA : SUDUT_TUTUP);
  Serial.println(buka ? ">>> Objek terdeteksi: palang TERBUKA (90 derajat)"
                      : ">>> Objek menjauh: palang TERTUTUP (0 derajat)");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  servoPalang.setPeriodHertz(50);               // frekuensi standar servo 50 Hz
  servoPalang.attach(PIN_SERVO, 500, 2400);     // lebar pulsa 500-2400 us
  servoPalang.write(SUDUT_TUTUP);               // posisi awal tertutup
  Serial.println("--- Palang Otomatis Siap ---");
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - waktuUkur >= INTERVAL_UKUR_MS) {
    waktuUkur = sekarang;

    float jarak = ukurJarakCm();
    bool objekDekat = (jarak > 0 && jarak < JARAK_BUKA_CM);   // jarak -1 dianggap jauh
    aturPalang(objekDekat);

    if (sekarang - waktuCetak >= INTERVAL_CETAK_MS) {
      waktuCetak = sekarang;
      if (jarak < 0) Serial.println("Jarak: di luar jangkauan");
      else           Serial.printf("Jarak: %.1f cm | Palang: %s\n",
                                   jarak, palangTerbuka ? "TERBUKA" : "TERTUTUP");
    }
  }
}
