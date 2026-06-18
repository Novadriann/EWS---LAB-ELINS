#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// --- LORA PIN FOR LILYGO T-BEAM ---
#define LORA_SCK     5
#define LORA_MISO   19
#define LORA_MOSI   27
#define LORA_SS     18
#define LORA_RST    23
#define LORA_DIO0   26

// Frekuensi LoRa Indonesia
#define LORA_BAND 922E6

String ambilNilai(String data, String key) {
  int startIndex = data.indexOf(key);

  if (startIndex == -1) {
    return "";
  }

  startIndex += key.length();

  int endIndex = data.indexOf(",", startIndex);

  if (endIndex == -1) {
    endIndex = data.length();
  }

  return data.substring(startIndex, endIndex);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("======================================");
  Serial.println("TTGO T-Beam V1.1 - PENERIMA LORA");
  Serial.println("Menerima data Soil, Magnet, Tip, PWM, Tilt");
  Serial.println("======================================");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  Serial.println("Memulai LoRa...");

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("Gagal memulai LoRa!");
    Serial.println("Periksa pin LoRa, antena, dan frekuensi.");
    while (1);
  }

  Serial.println("LoRa berhasil aktif pada frekuensi 922 MHz");
  Serial.println("Menunggu data dari pengirim...");
  Serial.println();
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String dataMasuk = "";

    while (LoRa.available()) {
      dataMasuk += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    String soilStr   = ambilNilai(dataMasuk, "SOIL:");
    String magRawStr = ambilNilai(dataMasuk, "MAG_RAW:");
    String tipStr    = ambilNilai(dataMasuk, "TIP:");
    String pwmStr    = ambilNilai(dataMasuk, "PWM:");
    String tiltStr   = ambilNilai(dataMasuk, "TILT:");

    int kelembapanTanah = soilStr.toInt();
    int rawMagnet       = magRawStr.toInt();
    unsigned long totalTip = tipStr.toInt();
    int nilaiPWM        = pwmStr.toInt();
    float sudutTilt     = tiltStr.toFloat();

    Serial.println("========== DATA DITERIMA ==========");

    Serial.print("Raw Paket LoRa       : ");
    Serial.println(dataMasuk);

    Serial.print("Kelembapan Tanah     : ");
    Serial.print(kelembapanTanah);
    Serial.println(" %");

    Serial.print("Raw Sensor Magnet    : ");
    Serial.println(rawMagnet);

    Serial.print("Total Hitungan Tip   : ");
    Serial.println(totalTip);

    Serial.print("Nilai PWM            : ");
    Serial.println(nilaiPWM);

    Serial.print("Sudut Tilt ADXL345   : ");
    Serial.print(sudutTilt, 2);
    Serial.println(" derajat");

    Serial.print("RSSI                 : ");
    Serial.print(rssi);
    Serial.println(" dBm");

    Serial.print("SNR                  : ");
    Serial.print(snr);
    Serial.println(" dB");

    Serial.println("===================================");
    Serial.println();

    // Output untuk Serial Plotter
    Serial.print("Soil:");
    Serial.print(kelembapanTanah);
    Serial.print(",");

    Serial.print("Magnet:");
    Serial.print(rawMagnet);
    Serial.print(",");

    Serial.print("Tip:");
    Serial.print(totalTip);
    Serial.print(",");

    Serial.print("PWM:");
    Serial.print(nilaiPWM);
    Serial.print(",");

    Serial.print("Tilt:");
    Serial.print(sudutTilt);
    Serial.print(",");

    Serial.print("RSSI:");
    Serial.println(rssi);
  }
}