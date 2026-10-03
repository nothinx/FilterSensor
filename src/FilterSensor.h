// FilterSensor - filter untuk meredam noise bacaan sensor: rata-rata bergerak,
// median, EMA, dan Kalman 1D. Semua dipakai dengan pola yang sama:
//   float halus = filter.saring(bacaan);
// Copyright (c) 2026 Amadeo Wisesa. Lisensi MIT.
//
// - Tanpa alokasi dinamis. Ukuran buffer ditentukan saat compile: RataRataBergerak<8>.
// - Hasil langsung benar sejak sampel pertama (tidak ditarik ke 0 saat buffer belum penuh).
// - Bacaan NAN (mis. sensor DHT gagal dibaca) diabaikan, hasil tetap nilai sebelumnya.
// - Sebelum ada sampel, hasil() = 0.
#pragma once
#include <Arduino.h>
#include <math.h>

// Rata-rata N sampel terakhir. Bagus untuk noise acak, lemah terhadap lonjakan.
template <uint8_t N>
class RataRataBergerak {
  static_assert(N >= 1, "N minimal 1");

public:
  float saring(float x) {
    if (isnan(x)) return hasil();
    if (_n < N) _n++;
    else _jumlah -= _buf[_i];
    _buf[_i] = x;
    _jumlah += x;
    if (++_i == N) {
      // Hitung ulang jumlah sekali tiap putaran agar galat float tidak menumpuk.
      _i = 0;
      _jumlah = 0;
      for (uint8_t k = 0; k < N; k++) _jumlah += _buf[k];
    }
    return hasil();
  }
  float hasil() const { return _n ? _jumlah / _n : 0; } // rata-rata sampel yang sudah masuk
  void reset() { _i = _n = 0; _jumlah = 0; }

private:
  float _buf[N];
  float _jumlah = 0;
  uint8_t _i = 0, _n = 0;
};

// Median N sampel terakhir (N ganjil, mis. 3, 5, 7). Membuang lonjakan sesaat
// tanpa ikut menggeser hasil. Tiap sampel hanya menggeser satu nilai (tanpa sort penuh).
template <uint8_t N>
class FilterMedian {
  static_assert(N % 2 == 1, "N harus ganjil, mis. 3, 5, atau 7");

public:
  float saring(float x) {
    if (isnan(x)) return hasil();
    uint8_t j;
    if (_n < N) {
      j = _n++;
    } else {
      // Ganti nilai tertua di urutan dengan nilai baru, lalu geser ke tempatnya.
      j = 0;
      while (_urut[j] != _buf[_i]) j++;
    }
    _buf[_i] = x;
    if (++_i == N) _i = 0;
    while (j > 0 && _urut[j - 1] > x) { _urut[j] = _urut[j - 1]; j--; }
    while (j + 1 < _n && _urut[j + 1] < x) { _urut[j] = _urut[j + 1]; j++; }
    _urut[j] = x;
    return hasil();
  }
  // Median sampel yang sudah masuk; jumlah genap (buffer belum penuh) = rata-rata dua nilai tengah.
  float hasil() const {
    if (!_n) return 0;
    return (_n & 1) ? _urut[_n / 2] : (_urut[_n / 2 - 1] + _urut[_n / 2]) / 2;
  }
  void reset() { _i = _n = 0; }

private:
  float _buf[N];  // urutan datang
  float _urut[N]; // urutan nilai
  uint8_t _i = 0, _n = 0;
};

// Exponential Moving Average: hasil += alpha * (bacaan - hasil). Hanya 9 byte RAM.
class FilterEMA {
public:
  // alpha 0..1. Kecil = lebih halus tapi lebih lambat. 1 = tanpa filter.
  explicit FilterEMA(float alpha) { aturAlpha(alpha); }
  // alpha dari waktu respon: setelah waktuRespon, hasil sudah menempuh 63% dari
  // perubahan (95% setelah 3x waktuRespon). Satuan keduanya sama, mis. ms.
  // Contoh: FilterEMA ema(FilterEMA::alphaDariWaktu(500, 20)); // respon 500 ms, sampel tiap 20 ms
  static float alphaDariWaktu(float waktuRespon, float intervalSampel) {
    return waktuRespon > 0 ? 1 - exp(-intervalSampel / waktuRespon) : 1;
  }

  float saring(float x) {
    if (isnan(x)) return _hasil;
    _hasil = _ada ? _hasil + _alpha * (x - _hasil) : x; // sampel pertama langsung dipakai
    _ada = true;
    return _hasil;
  }
  float hasil() const { return _hasil; }
  void reset() { _ada = false; _hasil = 0; }
  void aturAlpha(float alpha) { _alpha = alpha < 0 ? 0 : alpha > 1 ? 1 : alpha; }

private:
  float _alpha, _hasil = 0;
  bool _ada = false;
};

// Kalman 1D untuk nilai yang berubah pelan (suhu, jarak, level). Kedua noise
// ditulis dalam satuan sensor:
// - noiseUkur: seberapa jauh bacaan biasa melompat dari nilai sebenarnya
//   (kira-kira simpangan baku noise). Besar = lebih halus.
// - noiseProses: seberapa jauh nilai sebenarnya bisa berubah antar sampel.
//   Besar = lebih cepat mengikuti perubahan.
// Di awal filter cepat mengikuti bacaan, lalu makin halus sampai stabil.
class FilterKalman {
public:
  FilterKalman(float noiseUkur, float noiseProses) { aturNoise(noiseUkur, noiseProses); }

  float saring(float x) {
    if (isnan(x)) return _hasil;
    if (_p < 0) { // sampel pertama
      _hasil = x;
      _p = _r;
    } else {
      _p += _q;
      float k = _p + _r > 0 ? _p / (_p + _r) : 1;
      _hasil += k * (x - _hasil);
      _p *= 1 - k;
    }
    return _hasil;
  }
  float hasil() const { return _hasil; }
  void reset() { _p = -1; _hasil = 0; }
  void aturNoise(float noiseUkur, float noiseProses) { _r = noiseUkur * noiseUkur; _q = noiseProses * noiseProses; }

private:
  float _r, _q, _p = -1, _hasil = 0; // _p < 0 = belum ada sampel
};
