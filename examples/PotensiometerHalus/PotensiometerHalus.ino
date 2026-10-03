// Potensiometer yang angkanya tidak bergoyang-goyang, untuk mengatur kecepatan,
// volume, atau setpoint. Hasil dicetak di Serial Monitor (115200) hanya saat
// berubah.
//
// Sambungan: kaki tengah potensiometer ke A0, kaki pinggir ke GND dan 5V/3V3.
#include <FilterSensor.h>

#if defined(ESP32)
const float ADC_MAKS = 4095; // ADC 12 bit
#else
const float ADC_MAKS = 1023; // ADC 10 bit (Uno, Nano, Mega, STM32)
#endif

// Respon 200 ms dengan sampel tiap 10 ms: cukup cepat saat diputar, tenang saat diam.
FilterEMA halus(FilterEMA::alphaDariWaktu(200, 10));

void setup() {
  Serial.begin(115200);
}

void loop() {
  static uint32_t terakhir = 0;
  if (millis() - terakhir < 10) return;
  terakhir = millis();

  float nilai = halus.saring(analogRead(A0));

  // Skala 0-100 dengan histeresis 0,7 langkah: angka hanya berganti jika
  // bacaan halus benar-benar sudah pindah, bukan bergoyang di batas.
  static int persen = -1;
  float skala = nilai * 100 / ADC_MAKS;
  if (persen < 0 || fabs(skala - persen) > 0.7) {
    persen = round(skala);
    Serial.print("Persen: ");
    Serial.println(persen);
  }
}
