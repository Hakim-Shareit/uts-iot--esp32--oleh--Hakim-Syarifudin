/*
 * UTS Praktikum IoT - Soal 5: Telemetri dan Kontrol Jarak Jauh via MQTT
 *
 * Publish   : data cahaya (LDR, GPIO 34) tiap 5 detik ke  unisda/informatika/[NIM]/cahaya
 * Subscribe : perintah relay (GPIO 19) dari topik         unisda/informatika/[NIM]/lampu
 *             pesan "ON" -> relay aktif, "OFF" -> relay mati
 *
 * Broker: broker.emqx.io (port 1883, publik). Library: PubSubClient
 * PENTING: isi NIM dengan NIM Anda sebelum dijalankan.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ---------- Identitas (WAJIB DIISI) ----------
const char* NIM = "ISI_NIM_DI_SINI";

// ---------- Wi-Fi (Wokwi) ----------
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const int   WIFI_KANAL    = 6;                     // mempercepat koneksi di Wokwi
const unsigned long TIMEOUT_WIFI_MS = 10000;

// ---------- MQTT ----------
const char* MQTT_BROKER = "broker.emqx.io";
const int   MQTT_PORT   = 1883;
String topikCahaya, topikLampu;                    // dibentuk di setup() dari NIM

// ---------- Pin dan konstanta ----------
const int PIN_LDR   = 34;
const int PIN_RELAY = 19;
const int ADC_MAKS  = 4095;
const int JUMLAH_SAMPEL = 16;
const bool ADC_NAIK_SAAT_GELAP = true;             // sama seperti Soal 2

// Logika relay (lihat penjelasan di Soal 3). false = Active HIGH (modul Wokwi default)
const bool RELAY_AKTIF_LOW = false;
const uint8_t LEVEL_RELAY_ON  = RELAY_AKTIF_LOW ? LOW  : HIGH;
const uint8_t LEVEL_RELAY_OFF = RELAY_AKTIF_LOW ? HIGH : LOW;

const unsigned long INTERVAL_PUBLISH_MS   = 5000;  // kirim data tiap 5 detik
const unsigned long INTERVAL_RECONNECT_MS = 5000;  // coba sambung ulang MQTT tiap 5 detik

WiFiClient   espClient;
PubSubClient mqtt(espClient);
unsigned long waktuPublish = 0, waktuReconnect = 0;

// ---------- Sensor ----------
int bacaAdcMentah() {
  long total = 0;
  for (int i = 0; i < JUMLAH_SAMPEL; i++) total += analogRead(PIN_LDR);
  return (int)(total / JUMLAH_SAMPEL);
}

float adcKePersenGelap(int adc) {
  float persen = (adc * 100.0f) / ADC_MAKS;
  if (!ADC_NAIK_SAAT_GELAP) persen = 100.0f - persen;
  return constrain(persen, 0.0f, 100.0f);
}

// ---------- Relay ----------
void setRelay(bool aktif) {
  digitalWrite(PIN_RELAY, aktif ? LEVEL_RELAY_ON : LEVEL_RELAY_OFF);
  Serial.println(aktif ? "[Relay] AKTIF (lampu menyala)" : "[Relay] MATI (lampu padam)");
}

// ---------- Callback: dipanggil saat ada pesan pada topik yang di-subscribe ----------
void saatPesanMasuk(char* topik, byte* isi, unsigned int panjang) {
  String pesan;
  for (unsigned int i = 0; i < panjang; i++) pesan += (char)isi[i];
  pesan.trim();
  pesan.toUpperCase();                              // "on", "On", "ON" diperlakukan sama

  Serial.printf("[MQTT] Pesan masuk di %s : %s\n", topik, pesan.c_str());

  if (pesan == "ON")       setRelay(true);
  else if (pesan == "OFF") setRelay(false);
  else                     Serial.println("[MQTT] Perintah tidak dikenal (gunakan ON atau OFF)");
}

// ---------- Koneksi ----------
void sambungkanWiFi() {
  Serial.print("Menyambung Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_KANAL);
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < TIMEOUT_WIFI_MS) {
    Serial.print(".");
    delay(250);                                     // hanya di setup(), sebelum loop utama
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[Wi-Fi Connected] IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[Wi-Fi] Gagal tersambung, akan dicoba lagi di loop.");
  }
}

// Coba sambung ke broker satu kali (dipanggil berkala dari loop, tidak memblokir)
void sambungkanMqtt() {
  String clientId = "esp32-unisda-" + String(NIM) + "-" + String((uint32_t)esp_random(), HEX);
  Serial.print("[MQTT] Menyambung ke broker... ");
  if (mqtt.connect(clientId.c_str())) {
    Serial.println("berhasil");
    mqtt.subscribe(topikLampu.c_str());
    Serial.println("[MQTT] Subscribe: " + topikLampu);
  } else {
    Serial.printf("gagal (rc=%d)\n", mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);

  digitalWrite(PIN_RELAY, LEVEL_RELAY_OFF);         // relay mati saat boot
  pinMode(PIN_RELAY, OUTPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_LDR, ADC_11db);

  topikCahaya = String("unisda/informatika/") + NIM + "/cahaya";
  topikLampu  = String("unisda/informatika/") + NIM + "/lampu";
  Serial.println("Topik publish  : " + topikCahaya);
  Serial.println("Topik subscribe: " + topikLampu);

  sambungkanWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(saatPesanMasuk);
}

void loop() {
  unsigned long sekarang = millis();

  // Pastikan Wi-Fi dan MQTT tetap terhubung (non-blocking)
  if (WiFi.status() != WL_CONNECTED) {
    if (sekarang - waktuReconnect >= INTERVAL_RECONNECT_MS) {
      waktuReconnect = sekarang;
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_KANAL);
    }
    return;
  }
  if (!mqtt.connected()) {
    if (sekarang - waktuReconnect >= INTERVAL_RECONNECT_MS) {
      waktuReconnect = sekarang;
      sambungkanMqtt();
    }
  } else {
    mqtt.loop();                                    // memproses pesan masuk (subscribe)
  }

  // Publish data cahaya tiap 5 detik
  if (mqtt.connected() && sekarang - waktuPublish >= INTERVAL_PUBLISH_MS) {
    waktuPublish = sekarang;

    int   adc   = bacaAdcMentah();
    float gelap = adcKePersenGelap(adc);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"adc\":%d,\"gelap_persen\":%.1f}", adc, gelap);

    bool ok = mqtt.publish(topikCahaya.c_str(), payload);
    Serial.printf("[MQTT] Publish %s -> %s (%s)\n", topikCahaya.c_str(), payload, ok ? "ok" : "gagal");
  }
}
