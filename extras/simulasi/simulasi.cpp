// Simulasi FilterSensor di PC memakai kode library asli (../../src). Dipanggil oleh gambar.py.
//   g++ -std=c++11 -O2 -I../test -I../../src simulasi.cpp -o sim && ./sim
// Keluaran: beberapa bagian tabel (pemisah ;), tiap bagian diawali "# nama" lalu baris nama kolom.
// Sinyal sama dengan uji di extras/test/uji.cpp: noise gauss simpangan baku 5, seed 12345,
// 5% bacaan nyasar +-200.
#include <math.h>
#include <stdio.h>
#include "FilterSensor.h"

static uint32_t acak = 12345;
static float seragam() { acak = acak * 1664525u + 1013904223u; return ((acak >> 8) + 0.5f) / 16777216.0f; }
static float gauss() { return sqrtf(-2 * logf(seragam())) * cosf(6.2831853f * seragam()); }

struct Mentah { float x = 0; float saring(float v) { return x = v; } void reset() {} };
struct Gabungan {
  FilterMedian<5> m; FilterEMA e{0.3f};
  float saring(float v) { return e.saring(m.saring(v)); }
  void reset() { m.reset(); e.reset(); }
};

// Sama persis dengan ukur() di extras/test/uji.cpp.
template <class F>
static void ukur(F f, const char *nama) {
  double rms[2];
  for (int skenario = 0; skenario < 2; skenario++) {
    acak = 12345;
    f.reset();
    double jumlah = 0;
    int n = 0;
    for (int i = 0; i < 2000; i++) {
      float x = 100 + 5 * gauss();
      if (skenario == 1 && seragam() < 0.05f) x += seragam() < 0.5f ? 200 : -200;
      float h = f.saring(x);
      if (i >= 100) { jumlah += (h - 100.0) * (h - 100.0); n++; }
    }
    rms[skenario] = sqrt(jumlah / n);
  }
  f.reset();
  for (int i = 0; i < 50; i++) f.saring(0);
  int tunda = 0;
  while (f.saring(100) < 90) tunda++;
  tunda++;
  printf("%s;%.4f;%.4f;%d\n", nama, rms[0], rms[1], tunda);
}

// Sinyal langkah 0 -> 100 di sampel 60 dengan noise dan lonjakan, hasil tiap filter.
static void deret() {
  printf("# deret\nsampel;nyata;mentah;rata;median;ema;kalman;gabungan\n");
  RataRataBergerak<8> r; FilterMedian<5> m; FilterEMA e(0.2f); FilterKalman k(5, 1); Gabungan g;
  acak = 12345;
  for (int i = 0; i < 160; i++) {
    float nyata = i < 60 ? 0 : 100;
    float x = nyata + 5 * gauss();
    if (seragam() < 0.05f) x += seragam() < 0.5f ? 200 : -200;
    printf("%d;%g;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f\n", i, nyata, x, r.saring(x), m.saring(x), e.saring(x), k.saring(x),
           g.saring(x));
  }
}

int main() {
  deret();
  printf("# tabel\nfilter;rms_noise;rms_lonjakan;tunda\n");
  ukur(Mentah(), "Tanpa filter");
  ukur(RataRataBergerak<8>(), "RataRataBergerak<8>");
  ukur(FilterMedian<5>(), "FilterMedian<5>");
  ukur(FilterEMA(0.2f), "FilterEMA(0.2)");
  ukur(FilterKalman(5, 1), "FilterKalman(5, 1)");
  ukur(Gabungan(), "Median<5> + EMA(0.3)");

  // Pengaruh parameter: alpha EMA dan N rata-rata bergerak (tanpa lonjakan).
  printf("# ema\nfilter;rms_noise;rms_lonjakan;tunda\n");
  const float alpha[] = {0.05f, 0.1f, 0.15f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f, 1.0f};
  for (float a : alpha) {
    char nama[16];
    snprintf(nama, sizeof nama, "%g", a);
    ukur(FilterEMA(a), nama);
  }
  printf("# rata\nfilter;rms_noise;rms_lonjakan;tunda\n");
  ukur(RataRataBergerak<1>(), "1");
  ukur(RataRataBergerak<2>(), "2");
  ukur(RataRataBergerak<4>(), "4");
  ukur(RataRataBergerak<8>(), "8");
  ukur(RataRataBergerak<16>(), "16");
  ukur(RataRataBergerak<32>(), "32");
  return 0;
}
