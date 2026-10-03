// Uji logika FilterSensor di PC:
//   g++ -std=c++11 -Wall -Wextra -I. -I../../src uji.cpp -o uji && ./uji
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FilterSensor.h"

static bool dekat(float a, float b, float tol = 1e-4f) { return fabsf(a - b) <= tol; }

// Bilangan acak dengan seed tetap supaya hasil sama di semua PC.
static uint32_t acak = 12345;
static float seragam() { acak = acak * 1664525u + 1013904223u; return ((acak >> 8) + 0.5f) / 16777216.0f; }
static float gauss() { return sqrtf(-2 * logf(seragam())) * cosf(6.2831853f * seragam()); }

// Error RMS filter terhadap nilai sebenarnya, untuk sinyal: 100 + noise (simpangan
// baku 5), dan bila lonjakan = true, 5% sampel melonjak +-200 (bacaan nyasar).
// Mengembalikan juga berapa sampel sampai hasil mencapai 90% dari langkah 0 -> 100.
template <class F>
static void ukur(F f, const char *nama, float &rmsNoise, float &rmsLonjak, int &tunda) {
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
    (skenario ? rmsLonjak : rmsNoise) = sqrt(jumlah / n);
  }
  f.reset();
  for (int i = 0; i < 50; i++) f.saring(0);
  tunda = 0;
  while (f.saring(100) < 90) tunda++;
  tunda++;
  printf("| %-32s | %5.2f | %6.2f | %3d |\n", nama, rmsNoise, rmsLonjak, tunda);
}

int main() {
  int kasus = 0;
  { // nilai konstan: semua filter langsung tepat sejak sampel pertama
    RataRataBergerak<8> r; FilterMedian<5> m; FilterEMA e(0.1f); FilterKalman k(5, 0.5f);
    assert(r.hasil() == 0 && m.hasil() == 0 && e.hasil() == 0 && k.hasil() == 0);
    for (int i = 0; i < 20; i++) {
      assert(r.saring(42) == 42 && m.saring(42) == 42 && e.saring(42) == 42 && dekat(k.saring(42), 42));
    }
    kasus++;
  }
  { // sebelum buffer penuh: rata-rata/median dari sampel yang sudah ada, bukan ditarik ke 0
    RataRataBergerak<10> r;
    assert(r.saring(10) == 10);
    assert(r.saring(20) == 15);
    assert(r.saring(30) == 20);
    FilterMedian<5> m;
    assert(m.saring(10) == 10);
    assert(m.saring(30) == 20); // genap: rata-rata dua nilai tengah
    assert(m.saring(20) == 20);
    assert(m.saring(5) == 15);
    assert(m.saring(100) == 20);
    kasus++;
  }
  { // buffer penuh: hanya N sampel terakhir yang dihitung
    RataRataBergerak<4> r;
    for (int i = 1; i <= 10; i++) r.saring(i);
    assert(r.hasil() == 8.5f); // 7, 8, 9, 10
    FilterMedian<3> m;
    float urutan[] = {5, 1, 9, 2, 8, 3, 3, 3, 7};
    float harap[] = {5, 3, 5, 2, 8, 3, 3, 3, 3};
    for (int i = 0; i < 9; i++) assert(m.saring(urutan[i]) == harap[i]);
    kasus++;
  }
  { // median: cocok dengan median yang dihitung ulang (sort penuh) untuk 5000 sampel acak, termasuk nilai kembar
    FilterMedian<7> m;
    float riwayat[7];
    for (int i = 0; i < 5000; i++) {
      // Banyak nilai kembar, lalu negatif/pecahan, ±0 dan ±tak hingga: median disimpan
      // sebagai kunci bilangan bulat, jadi urutan tanda dan eksponen harus tetap benar.
      float x = i < 2500 ? (float)(int)(seragam() * 20) : (seragam() - 0.5f) * 2000;
      if (i >= 2500 && i % 50 == 0) x = (i / 50 % 4 == 0) ? -0.0f : (i / 50 % 4 == 1) ? 0.0f : (i / 50 % 4 == 2) ? INFINITY : -INFINITY;
      riwayat[i % 7] = x;
      float h = m.saring(x);
      int n = i < 7 ? i + 1 : 7;
      float s[7];
      for (int a = 0; a < n; a++) s[a] = riwayat[a];
      for (int a = 1; a < n; a++) for (int b = a; b > 0 && s[b - 1] > s[b]; b--) { float t = s[b]; s[b] = s[b - 1]; s[b - 1] = t; }
      float harap = (n & 1) ? s[n / 2] : (s[n / 2 - 1] + s[n / 2]) / 2;
      assert(h == harap);
    }
    kasus++;
  }
  { // spike tunggal: median membuang total, rata-rata ikut tergeser
    FilterMedian<5> m; RataRataBergerak<5> r;
    for (int i = 0; i < 10; i++) { m.saring(50); r.saring(50); }
    assert(m.saring(1000) == 50);
    assert(r.saring(1000) == 240);
    for (int i = 0; i < 5; i++) { assert(m.saring(50) == 50); r.saring(50); }
    assert(r.hasil() == 50); // lonjakan sudah keluar dari buffer
    kasus++;
  }
  { // langkah (step): EMA menempuh 63% setelah waktuRespon
    FilterEMA e(FilterEMA::alphaDariWaktu(100, 10)); // respon 100 ms, sampel tiap 10 ms
    e.saring(0);
    for (int i = 0; i < 10; i++) e.saring(100);
    assert(dekat(e.hasil(), 63.2f, 0.1f));
    assert(FilterEMA::alphaDariWaktu(0, 10) == 1);
    FilterEMA batas(5); // alpha di luar 0..1 dibatasi
    batas.saring(0);
    assert(batas.saring(7) == 7);
    kasus++;
  }
  { // Kalman: sampel pertama langsung dipakai, lalu menuju nilai baru; noise 0 = ikut bacaan
    FilterKalman k(5, 0.5f);
    assert(k.saring(20) == 20);
    float h = 20;
    for (int i = 0; i < 200; i++) { float b = k.saring(30); assert(b >= h); h = b; }
    assert(dekat(h, 30, 0.01f));
    FilterKalman nol(0, 0);
    nol.saring(1);
    assert(nol.saring(9) == 9);
    kasus++;
  }
  { // NAN (sensor gagal dibaca) diabaikan
    RataRataBergerak<4> r; FilterMedian<3> m; FilterEMA e(0.5f); FilterKalman k(1, 1);
    r.saring(10); m.saring(10); e.saring(10); k.saring(10);
    assert(r.saring(NAN) == 10 && m.saring(NAN) == 10 && e.saring(NAN) == 10 && k.saring(NAN) == 10);
    RataRataBergerak<4> kosong;
    assert(kosong.saring(NAN) == 0);
    kasus++;
  }
  { // reset(): mulai dari awal, sampel berikutnya langsung dipakai
    RataRataBergerak<4> r; FilterMedian<3> m; FilterEMA e(0.1f); FilterKalman k(5, 0.1f);
    for (int i = 0; i < 9; i++) { r.saring(500); m.saring(500); e.saring(500); k.saring(500); }
    r.reset(); m.reset(); e.reset(); k.reset();
    assert(r.hasil() == 0 && m.hasil() == 0 && e.hasil() == 0 && k.hasil() == 0);
    assert(r.saring(3) == 3 && m.saring(3) == 3 && e.saring(3) == 3 && k.saring(3) == 3);
    kasus++;
  }
  { // rata-rata bergerak tidak drift: 1 juta sampel bernilai besar, dibanding hitung double
    RataRataBergerak<10> r;
    double riwayat[10];
    float x = 0;
    for (long i = 0; i < 1000000L; i++) {
      x = 10000 + (float)(i % 7) * 0.37f + (i % 1000 == 0 ? 50000 : 0);
      riwayat[i % 10] = x;
      r.saring(x);
    }
    double harap = 0;
    for (int a = 0; a < 10; a++) harap += riwayat[a];
    harap /= 10;
    assert(fabs(r.hasil() - harap) < 0.01);
    kasus++;
  }

  // Perbandingan antarfilter (masuk README).
  float nR, lR, nM, lM, nE, lE, nK, lK, nA, lA;
  int tR, tM, tE, tK, tA;
  printf("| Filter                           | Noise | +Lonjakan | Tunda |\n");
  struct Mentah { float x = 0; float saring(float v) { return x = v; } void reset() {} };
  ukur(Mentah(), "Tanpa filter", nA, lA, tA);
  ukur(RataRataBergerak<8>(), "RataRataBergerak<8>", nR, lR, tR);
  ukur(FilterMedian<5>(), "FilterMedian<5>", nM, lM, tM);
  ukur(FilterEMA(0.2f), "FilterEMA(0.2)", nE, lE, tE);
  ukur(FilterKalman(5, 1), "FilterKalman(5, 1)", nK, lK, tK);
  struct Gabungan {
    FilterMedian<5> m; FilterEMA e{0.3f};
    float saring(float v) { return e.saring(m.saring(v)); }
    void reset() { m.reset(); e.reset(); }
  };
  float nG, lG; int tG;
  ukur(Gabungan(), "FilterMedian<5> + FilterEMA(0.3)", nG, lG, tG);
  assert(nG < nM && lG < lM); // median lalu EMA: tahan lonjakan dan lebih halus
  assert(nR < nA && nM < nA && nE < nA && nK < nA); // semua meredam noise
  assert(lM < lR && lM < lE && lM < lK);             // median paling tahan lonjakan
  assert(tM <= 3);                                   // median 5 mengikuti langkah dalam 3 sampel
  kasus++;

  printf("Semua uji lolos (%d kasus; sizeof: RataRataBergerak<8> = %u, FilterMedian<5> = %u, FilterEMA = %u, FilterKalman = %u byte)\n",
         kasus, (unsigned)sizeof(RataRataBergerak<8>), (unsigned)sizeof(FilterMedian<5>), (unsigned)sizeof(FilterEMA), (unsigned)sizeof(FilterKalman));
  return 0;
}
