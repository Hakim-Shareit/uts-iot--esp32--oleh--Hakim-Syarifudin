/*
 * UTS Praktikum IoT - Soal 2: Pembacaan Sensor Analog LDR dan Pengolahan Data ADC
 * Modul LDR Wokwi (berisi LDR yang diseri resistor 10 kOhm = pembagi tegangan)
 *   VCC -> 3V3 | GND -> GND | AO -> GPIO 34 (ADC1)
 *
 * Keluaran Serial Monitor tiap 1 detik: nilai ADC mentah (0-4095) dan persen kegelapan.
 */
#include <Arduino.h>

const int PIN_LDR = 34;                    // ADC1 (aman dipakai bersama Wi-Fi)

const int  ADC_MAKS          = 4095;       // resolusi 12 bit
const int  JUMLAH_SAMPEL     = 16;         // rata-rata untuk meredam noise ADC
const unsigned long INTERVAL_MS = 1000;    // tampilkan data tiap 1 detik

// Arah sensor:
//  true  -> ADC NAIK saat GELAP (LDR di sisi atas, 10k ke GND; seperti modul LDR Wokwi)
//  false -> ADC TURUN saat gelap (LDR di sisi bawah/ke GND, 10k ke 3V3)
// Uji di Wokwi: geser lux kecil; bila ADC membesar biarkan true, bila mengecil ganti false.
const bool ADC_NAIK_SAAT_GELAP = true;

unsigned long waktuSebelumnya = 0;

// Baca ADC mentah dengan rata-rata beberapa sampel
int bacaAdcMentah() {
  long total = 0;
  for (int i = 0; i < JUMLAH_SAMPEL; i++) {
    total += analogRead(PIN_LDR);
  }
  return (int)(total / JUMLAH_SAMPEL);
}

// Konversi ADC (0-4095) menjadi persentase kegelapan (0-100 %)
float adcKePersenGelap(int adc) {
  float persen = (adc * 100.0f) / ADC_MAKS;       // 0 % .. 100 % terhadap skala penuh
  if (!ADC_NAIK_SAAT_GELAP) persen = 100.0f - persen;
  return constrain(persen, 0.0f, 100.0f);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);                          // 0..4095
  analogSetPinAttenuation(PIN_LDR, ADC_11db);        // jangkauan hingga sekitar 3,3 V
  Serial.println("--- Pembacaan LDR (ADC 12-bit) ---");
}

void loop() {
  unsigned long sekarang = millis();
  if (sekarang - waktuSebelumnya >= INTERVAL_MS) {   // non-blocking, tanpa delay()
    waktuSebelumnya = sekarang;

    int   adc      = bacaAdcMentah();
    float tegangan = adc * 3.3f / ADC_MAKS;
    float gelap    = adcKePersenGelap(adc);

    Serial.printf("ADC mentah: %4d | Tegangan: %.2f V | Kegelapan: %5.1f %%\n",
                  adc, tegangan, gelap);
  }
}
