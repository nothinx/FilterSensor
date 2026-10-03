// Benchmark FilterSensor di ATmega328P 16 MHz (simavr). Cara menjalankan dan
// angka hasilnya: README bagian "Kecepatan & memori".
// Siklus.h: Timer1 tanpa prescaler, UKUR(nama, ulang, kode) mencetak
// "BENCH nama siklus_per_panggilan".
#include <FilterSensor.h>
#include "Siklus.h"

// 64 bacaan sekitar 500 ± 20 dengan sesekali lonjakan +300, seed tetap.
static float data[64];
volatile float keluaran;

RataRataBergerak<8> rata8;
FilterMedian<5> median5;
FilterMedian<15> median15;
FilterEMA ema(0.2f);
FilterKalman kalman(5, 1);

void setup() {
  Serial.begin(115200);
  uint32_t a = 12345;
  for (int i = 0; i < 64; i++) {
    a = a * 1664525UL + 1013904223UL;
    data[i] = 500 + (int)((a >> 20) % 41) - 20 + ((a >> 8) % 20 == 0 ? 300 : 0);
  }
  for (int i = 0; i < 64; i++) { // buffer penuh dulu
    rata8.saring(data[i]); median5.saring(data[i]); median15.saring(data[i]);
    ema.saring(data[i]); kalman.saring(data[i]);
  }
  UKUR("kosong", 1000, keluaran = data[_i & 63]);
  UKUR("RataRataBergerak<8>", 1000, keluaran = rata8.saring(data[_i & 63]));
  UKUR("FilterMedian<5>", 1000, keluaran = median5.saring(data[_i & 63]));
  UKUR("FilterMedian<15>", 1000, keluaran = median15.saring(data[_i & 63]));
  UKUR("FilterEMA", 1000, keluaran = ema.saring(data[_i & 63]));
  UKUR("FilterKalman", 1000, keluaran = kalman.saring(data[_i & 63]));
  selesai();
}

void loop() {}
