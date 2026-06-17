#include <Arduino.h>

// Pin analog sensor
#define SOIL_PIN 15

// Nilai kalibrasi awal
// Ubah sesuai hasil pengukuran sensor kamu
int nilaiKering = 3200;
int nilaiBasah  = 1300;

// Jumlah pembacaan untuk dirata-rata
const int jumlahSampling = 20;

int bacaSensorRataRata() {
  long total = 0;

  for (int i = 0; i < jumlahSampling; i++) {
    total += analogRead(SOIL_PIN);
    delay(10);
  }

  return total / jumlahSampling;
}

String statusKelembapan(int persen) {
  if (persen < 30) {
    return "Tanah Kering";
  } 
  else if (persen >= 30 && persen < 70) {
    return "Tanah Normal";
  } 
  else {
    return "Tanah Basah";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Mengatur resolusi ADC ESP32 menjadi 12-bit
  // Nilai pembacaan menjadi 0 sampai 4095
  analogReadResolution(12);

  // Mengatur range pembacaan ADC agar sesuai untuk tegangan 0 sampai sekitar 3.3V
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  Serial.println("====================================");
  Serial.println("Monitoring Kelembapan Tanah");
  Serial.println("ESP32 TTGO T-Beam V1.1");
  Serial.println("Capacitive Soil Moisture Sensor v2.0");
  Serial.println("====================================");
}

void loop() {
  int nilaiADC = bacaSensorRataRata();

  // Konversi nilai ADC menjadi persentase kelembapan
  // Tanah kering biasanya ADC besar
  // Tanah basah biasanya ADC kecil
  int kelembapan = map(nilaiADC, nilaiKering, nilaiBasah, 0, 100);

  // Batasi nilai agar tetap 0 sampai 100
  kelembapan = constrain(kelembapan, 0, 100);

  String kondisi = statusKelembapan(kelembapan);

  Serial.print("Nilai ADC: ");
  Serial.print(nilaiADC);

  Serial.print(" | Kelembapan: ");
  Serial.print(kelembapan);
  Serial.print(" %");

  Serial.print(" | Status: ");
  Serial.println(kondisi);

  delay(1000);
}