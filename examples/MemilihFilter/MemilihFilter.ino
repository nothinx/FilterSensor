// Memilih filter yang tepat. Sketch ini membuat sinyal buatan (nilai asli 100,
// noise +-5, dan 5% bacaan nyasar +-200), lalu mencetak error tiap filter di
// Serial Monitor (115200). Tidak perlu sensor.
//
// Panduan singkat:
// | Masalah                                | Filter                         |
// |----------------------------------------|--------------------------------|
// | Angka bergoyang halus (noise acak)     | FilterEMA atau FilterKalman    |
// | Sesekali ada angka ngawur (lonjakan)   | FilterMedian<5>                |
// | Keduanya (HC-SR04, sensor IR)          | FilterMedian<5> lalu FilterEMA |
// | Butuh rata-rata N sampel yang pasti    | RataRataBergerak<N>            |
// | RAM sangat sempit                      | FilterEMA (hanya 9 byte)       |
// Makin halus hasilnya, makin lambat filter mengikuti perubahan. Lihat kolom
// "tunda": berapa sampel sampai hasil menempuh 90% dari perubahan mendadak.
#include <FilterSensor.h>

RataRataBergerak<8> rataRata;
FilterMedian<5> median;
FilterEMA ema(0.2);
FilterKalman kalman(5, 1);
FilterMedian<5> medianDulu;
FilterEMA lalu(0.3);

const uint8_t JUMLAH = 6;
const char *const NAMA[JUMLAH] = {"Tanpa filter", "RataRataBergerak<8>", "FilterMedian<5>",
                                  "FilterEMA(0.2)", "FilterKalman(5, 1)", "Median<5> + EMA(0.3)"};

float saring(uint8_t f, float x) {
  switch (f) {
    case 1: return rataRata.saring(x);
    case 2: return median.saring(x);
    case 3: return ema.saring(x);
    case 4: return kalman.saring(x);
    case 5: return lalu.saring(medianDulu.saring(x));
    default: return x;
  }
}

void resetSemua() {
  rataRata.reset();
  median.reset();
  ema.reset();
  kalman.reset();
  medianDulu.reset();
  lalu.reset();
}

// Noise kira-kira normal dengan simpangan baku 5 (jumlah 12 angka acak).
float noise() {
  long s = 0;
  for (uint8_t i = 0; i < 12; i++) s += random(1000);
  return (s - 6000) * 5.0 / 1000;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Filter                  error-noise  error-lonjakan  tunda");
  for (uint8_t f = 0; f < JUMLAH; f++) {
    float error[2];
    for (uint8_t lonjakan = 0; lonjakan < 2; lonjakan++) {
      randomSeed(1);
      resetSemua();
      float jumlah = 0;
      for (int i = 0; i < 1000; i++) {
        float x = 100 + noise();
        if (lonjakan && random(100) < 5) x += random(2) ? 200 : -200;
        float h = saring(f, x);
        if (i >= 100) jumlah += (h - 100) * (h - 100);
      }
      error[lonjakan] = sqrt(jumlah / 900);
    }
    resetSemua();
    for (uint8_t i = 0; i < 50; i++) saring(f, 0);
    uint8_t tunda = 1;
    while (saring(f, 100) < 90) tunda++;

    Serial.print(NAMA[f]);
    for (uint8_t i = strlen(NAMA[f]); i < 24; i++) Serial.print(' ');
    Serial.print(error[0], 2);
    Serial.print("         ");
    Serial.print(error[1], 2);
    Serial.print("           ");
    Serial.println(tunda);
  }
}

void loop() {}
