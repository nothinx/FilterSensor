// Suhu LM35 yang tenang. ADC 10 bit di Uno hanya punya langkah +-0,5 derajat,
// jadi angkanya mudah meloncat 24,4 - 24,9 - 24,4. Filter Kalman meredamnya.
// Hasil di Serial Monitor / Serial Plotter (115200), satu kali per detik.
//
// Sambungan LM35 (tampak depan, kaki di bawah): kiri ke 5V, tengah ke A0, kanan ke GND.
#include <FilterSensor.h>

// Bacaan biasa meloncat +-0,5 derajat, suhu asli berubah paling +-0,01 derajat
// per sampel (100 ms). Naikkan angka kedua jika hasil terasa terlambat.
FilterKalman suhu(0.5, 0.01);

#ifdef __AVR__
const float MV_PER_LANGKAH = 5000.0 / 1023; // Uno, Nano, Mega: ADC 10 bit, 5 V
#else
const float MV_PER_LANGKAH = 3300.0 / 1023; // STM32: ADC 10 bit, 3,3 V
#endif

void setup() {
  Serial.begin(115200);
}

void loop() {
  static uint32_t terakhir = 0;
  static uint8_t hitung = 0;
  if (millis() - terakhir < 100) return; // 10 sampel per detik
  terakhir = millis();

#if defined(ESP32)
  float mv = analogReadMilliVolts(A0); // sudah dikoreksi pabrik
#else
  float mv = analogRead(A0) * MV_PER_LANGKAH;
#endif
  float mentah = mv / 10; // LM35: 10 mV per derajat Celsius
  float halus = suhu.saring(mentah);

  if (++hitung < 10) return;
  hitung = 0;
  Serial.print("mentah:");
  Serial.print(mentah, 1);
  Serial.print(",suhu:");
  Serial.println(halus, 1);
}
