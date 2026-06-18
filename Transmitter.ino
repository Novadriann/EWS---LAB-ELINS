#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>               // Library untuk komunikasi I2C ADXL345
#include <LoRa.h>              // Library LoRa Sandeep Mistry
#include <Adafruit_Sensor.h>    // Library sensor pendukung Adafruit
#include <Adafruit_ADXL345_U.h> // Library ADXL345

// --- PIN CONFIGURATION ---
#define SOIL_PIN 15
#define HALL_PIN 13

// --- PIN CONFIGURATION I2C (UNTUK SENSOR ADXL345) ---
#define I2C_SDA 14  
#define I2C_SCL 4  

// --- LORA PIN FOR LILYGO T-BEAM ---
#define LORA_SCK     5
#define LORA_MISO   19
#define LORA_MOSI   27
#define LORA_SS     18
#define LORA_RST    23
#define LORA_DIO0   26

// Frekuensi LoRa Indonesia (922 MHz)
#define LORA_BAND 922E6 

// --- SOIL MOISTURE CONFIGURATION ---
int nilaiKering = 3200;
int nilaiBasah  = 1300;
const int jumlahSampling = 20;

// --- HW-477 CALIBRATION & COUNTER ---
int baseline = 0;
int threshold = 100;
bool lastDigitalState = false;

unsigned long countTip = 0;
unsigned long lastDetectTime = 0;
const unsigned long debounceMs = 250;

// Waktu pengiriman LoRa (tiap 5 detik)
unsigned long lastSendTime = 0;
const unsigned long intervalSend = 5000; 

// --- INISIALISASI SENSOR ADXL345 ---
// Memberikan ID acak '12345' untuk sensor
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

int bacaSensorRataRata() {
  long total = 0;
  for (int i = 0; i < jumlahSampling; i++) {
    total += analogRead(SOIL_PIN);
    delay(5);
  }
  return total / jumlahSampling;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  analogSetPinAttenuation(HALL_PIN, ADC_11db);

  // --- PROSES KALIBRASI HW-477 ---
  Serial.println("=== KALIBRASI SENSOR MAGNET ===");
  long totalHall = 0;
  for (int i = 0; i < 300; i++) {
    totalHall += analogRead(HALL_PIN);
    delay(10);
  }
  baseline = totalHall / 300;
  Serial.print("Baseline Magnet: "); Serial.println(baseline);

  // --- INISIALISASI WIRE (I2C) ---
  Wire.begin(I2C_SDA, I2C_SCL);

  // --- INISIALISASI ADXL345 ---
  Serial.println("Memulai ADXL345...");
  // Mencoba inisialisasi pada alamat default 0x53, jika gagal coba alamat alternatif 0x1D
  if(!accel.begin(0x53)) { 
    Serial.println("Gagal mendeteksi ADXL345 di alamat 0x53. Mencoba alamat alternatif 0x1D...");
    if(!accel.begin(0x1D)) {
      Serial.println("Gagal total mendeteksi ADXL345! Periksa kembali kabel SDA/SCL kamu.");
      while(1); 
    }
  }
  
  // Mengatur range sensor ke 2G (paling sensitif untuk mendeteksi kemiringan statis)
  accel.setRange(ADXL345_RANGE_2_G);
  Serial.println("ADXL345 Berhasil Diaktifkan!");

  // --- INISIALISASI LORA ---
  Serial.println("Memulai LoRa...");
  
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("Gagal memulai LoRa! Periksa hardware/tipe chip kamu.");
    while (1); 
  }
  
  Serial.println("LoRa Berhasil Diaktifkan (Frekuensi 922 MHz)!");
  Serial.println("===============================");
}

void loop() {
  // 1. MEMBACA SENSOR KELEMBAPAN TANAH
  int nilaiADC_Soil = bacaSensorRataRata();
  int kelembapan = map(nilaiADC_Soil, nilaiKering, nilaiBasah, 0, 100);
  kelembapan = constrain(kelembapan, 0, 100);

  // 2. MEMBACA SENSOR MAGNET HW-477
  int rawHall = analogRead(HALL_PIN);
  int diff = rawHall - baseline;
  int absDiff = abs(diff);
  bool digitalState = absDiff > threshold;

  if (digitalState && !lastDigitalState) {
    unsigned long now = millis();
    if (now - lastDetectTime > debounceMs) {
      lastDetectTime = now;
      countTip++;
    }
  }
  lastDigitalState = digitalState;

  int nilaiPWM = map(absDiff, threshold, 1500, 0, 255);
  nilaiPWM = constrain(nilaiPWM, 0, 255); 

  // 3. MEMBACA SENSOR ADXL345 & MENGHITUNG SUDUT TILT (KEMIRINGAN)
  sensors_event_t event; 
  accel.getEvent(&event);
  
  // Menghitung total gaya G gabungan (X, Y, Z)
  float gTotal = sqrt(event.acceleration.x * event.acceleration.x + 
                      event.acceleration.y * event.acceleration.y + 
                      event.acceleration.z * event.acceleration.z);
                      
  if (gTotal == 0) gTotal = 0.001; // Mencegah pembagian dengan angka nol
  
  // Menghitung sudut kemiringan berdasarkan sumbu Z terhadap total gravitasi
  float sudutTilt = acos(event.acceleration.z / gTotal) * 180.0 / PI;
  if (isnan(sudutTilt)) sudutTilt = 0.0;

  // 4. OUTPUT LOKAL UNTUK SERIAL PLOTTER
  Serial.print("Kelembapan_Tanah_Persen:"); Serial.print(kelembapan); Serial.print(",");
  Serial.print("Raw_Magnet:"); Serial.print(rawHall); Serial.print(",");
  Serial.print("Digital_Status_Magnet:"); Serial.print(digitalState ? 1000 : 0); Serial.print(",");
  Serial.print("Total_Hitungan_Tip:"); Serial.print(countTip); Serial.print(",");
  Serial.print("Nilai_Kalkulasi_PWM:"); Serial.print(nilaiPWM); Serial.print(",");
  Serial.print("Sudut_Tilt_Derajat:"); Serial.println(sudutTilt);

  // 5. LOGIKA PENGIRIMAN DATA VIA LORA (Non-blocking menggunakan millis)
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= intervalSend) {
    lastSendTime = currentMillis;

    Serial.println(">> Mengirim data paket via LoRa...");

    // Format paket string disesuaikan dengan tambahan data TILT ADXL345
    String dataPaket = "SOIL:" + String(kelembapan) + 
                       ",MAG_RAW:" + String(rawHall) + 
                       ",TIP:" + String(countTip) + 
                       ",PWM:" + String(nilaiPWM) +
                       ",TILT:" + String(sudutTilt, 2);

    // Proses Pengiriman Paket LoRa
    LoRa.beginPacket();
    LoRa.print(dataPaket);
    LoRa.endPacket();

    Serial.print("Data terkirim: ");
    Serial.println(dataPaket);
  }

  delay(50); 
}