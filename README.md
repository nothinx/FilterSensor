# FilterSensor

[English](README.en.md)

Library Arduino berbahasa Indonesia untuk **meredam noise bacaan sensor**: rata-rata bergerak, median, EMA, dan Kalman 1D. Semua filter dipakai dengan pola yang sama:

```cpp
float halus = filter.saring(bacaan);
```

## Fitur

- **Empat filter, satu pola**: `RataRataBergerak<N>`, `FilterMedian<N>`, `FilterEMA`, `FilterKalman`. Ganti filter cukup dengan mengganti satu baris deklarasi.
- **Tanpa alokasi dinamis**. Ukuran buffer ditentukan saat compile (`RataRataBergerak<8>`), jadi RAM terlihat di laporan compile.
- **Benar sejak sampel pertama**. Sebelum buffer penuh, hasil adalah rata-rata/median dari sampel yang sudah ada. EMA dan Kalman langsung mulai dari bacaan pertama, bukan dari 0.
- **Rata-rata bergerak tanpa drift**. Jumlah dihitung ulang sekali setiap putaran buffer, sehingga galat pembulatan float tidak menumpuk walau berjalan berhari-hari.
- **Median tahan lonjakan**. Satu bacaan ngawur (HC-SR04, sensor IR) dibuang total. Tiap sampel baru hanya menggeser satu nilai, tanpa mengurutkan ulang seluruh buffer.
- **EMA dari waktu respon**: `FilterEMA::alphaDariWaktu(500, 20)` jika tidak mau menebak-nebak alpha.
- **Kalman dengan noise dalam satuan sensor**: "bacaan meloncat ±0,5 derajat" langsung jadi `FilterKalman(0.5, ...)`.
- **Bacaan `NAN` diabaikan** (mis. DHT gagal dibaca, HC-SR04 tanpa pantulan). Hasil tetap nilai terakhir yang sah.

## Board yang didukung

| Board | Teruji compile |
|---|---|
| Arduino Uno / Nano | ✅ |
| Arduino Mega | CI |
| ESP32 DevKit | ✅ |
| ESP32-C3 / S3 | CI |
| STM32 Blackpill F411 | CI |
| STM32 Bluepill F103 | ✅ |

✅ = dicompile tanpa warning saat rilis. CI = dicompile otomatis oleh GitHub Actions setiap ada perubahan.

Library ini murni perhitungan (header saja, tanpa akses hardware), jadi seharusnya bekerja di board Arduino apa pun.

## Instalasi

**Library Manager:** Arduino IDE → *Sketch → Include Library → Manage Libraries…* → cari **FilterSensor** → *Install*.

**Manual:** unduh ZIP dari GitHub → *Sketch → Include Library → Add .ZIP Library…*

## Contoh cepat

```cpp
#include <FilterSensor.h>

FilterMedian<5> median;   // buang bacaan nyasar
FilterEMA ema(0.2);       // haluskan sisanya

void setup() {
  Serial.begin(115200);
}

void loop() {
  float mentah = analogRead(A0);
  float halus = ema.saring(median.saring(mentah));
  Serial.println(halus);
  delay(20);
}
```

Filter tidak memakai waktu sendiri. Satu panggilan `saring()` = satu sampel, jadi panggil dengan selang waktu yang tetap (mis. tiap 20 ms dengan `millis()`).

## Memilih filter

| Masalah | Filter |
|---|---|
| Angka bergoyang halus (noise acak) | `FilterEMA` atau `FilterKalman` |
| Sesekali ada angka ngawur (lonjakan) | `FilterMedian<5>` |
| Keduanya (HC-SR04, sensor jarak IR) | `FilterMedian<5>` lalu `FilterEMA` |
| Butuh rata-rata N sampel terakhir yang pasti | `RataRataBergerak<N>` |
| RAM sangat sempit | `FilterEMA` (9 byte) |

Hasil uji di PC (`extras/test`): nilai asli 100, noise acak dengan simpangan baku 5, seed tetap, 2000 sampel. Kolom *+Lonjakan* menambahkan 5% bacaan nyasar ±200. *Tunda* = jumlah sampel sampai hasil menempuh 90% dari perubahan mendadak 0 → 100.

| Filter | Error RMS (noise) | Error RMS (+lonjakan) | Tunda (sampel) |
|---|---|---|---|
| Tanpa filter | 5,03 | 43,60 | 1 |
| `RataRataBergerak<8>` | 1,83 | 15,25 | 8 |
| `FilterMedian<5>` | 2,77 | 2,73 | 3 |
| `FilterEMA(0.2)` | 1,70 | 14,39 | 11 |
| `FilterKalman(5, 1)` | 1,61 | 13,63 | 12 |
| `FilterMedian<5>` + `FilterEMA(0.3)` | 1,94 | 1,88 | 9 |

Makin halus hasilnya, makin lambat filter mengikuti perubahan. Contoh `MemilihFilter` menjalankan perbandingan yang sama langsung di board.

## Referensi fungsi

Semua filter punya tiga fungsi yang sama:

| Fungsi | Keterangan |
|---|---|
| `float saring(float bacaan)` | Masukkan satu sampel, kembalikan hasil terbaru. `NAN` diabaikan. |
| `float hasil()` | Hasil terakhir tanpa menambah sampel. `0` jika belum ada sampel. |
| `void reset()` | Kosongkan filter. Sampel berikutnya dipakai sebagai titik awal. |

### Filter

| Deklarasi | RAM di Uno | Keterangan |
|---|---|---|
| `RataRataBergerak<N> f;` | 4N + 6 B | Rata-rata N sampel terakhir (N = 1–255). |
| `FilterMedian<N> f;` | 8N + 2 B | Median N sampel terakhir. N ganjil: 3, 5, 7, … Lonjakan hingga (N−1)/2 sampel berturut-turut dibuang. |
| `FilterEMA f(alpha);` | 9 B | `hasil += alpha × (bacaan − hasil)`. alpha 0–1: kecil = halus tapi lambat, 1 = tanpa filter. |
| `FilterKalman f(noiseUkur, noiseProses);` | 16 B | Kalman 1D untuk nilai yang berubah pelan. Lihat penjelasan di bawah. |

RAM diukur di Arduino Uno: selisih memori global sketch dengan dan tanpa satu filter.

### Pengaturan

| Fungsi | Keterangan |
|---|---|
| `static float FilterEMA::alphaDariWaktu(waktuRespon, intervalSampel)` | alpha agar hasil menempuh 63% perubahan setelah `waktuRespon` (95% setelah 3×). Satuan keduanya sama, mis. ms. |
| `void FilterEMA::aturAlpha(float alpha)` | Ganti alpha saat berjalan. Dibatasi ke 0–1. |
| `void FilterKalman::aturNoise(float noiseUkur, float noiseProses)` | Ganti noise saat berjalan. |

## Menyetel FilterKalman

Kedua angka ditulis dalam **satuan sensor** (derajat, cm, nilai ADC):

- **`noiseUkur`**: seberapa jauh bacaan biasanya melompat dari nilai sebenarnya. Cara mudah: diamkan sensor, lihat bacaan mentah di Serial Monitor, ambil kira-kira separuh selisih terbesar dan terkecilnya. LM35 di Uno meloncat ±0,5 derajat → `0.5`.
- **`noiseProses`**: seberapa jauh nilai sebenarnya bisa berubah dari satu sampel ke sampel berikutnya. Suhu ruangan dengan 10 sampel per detik → `0.01`.

Hasil terlalu bergoyang? Kecilkan `noiseProses`. Hasil terlalu lambat mengikuti perubahan? Besarkan `noiseProses`. Yang menentukan hanya perbandingan kedua angka.

Di awal, filter langsung memakai bacaan pertama lalu makin halus. Setelah stabil, `FilterKalman` 1D berperilaku sama dengan EMA dengan alpha tetap. Kelebihannya ada di awal yang cepat dan cara menyetel yang memakai satuan sensor.

## Contoh yang tersedia

*File → Examples → FilterSensor*

| Contoh | Isi |
|---|---|
| `BandingkanFilter` | Bacaan mentah dan keempat filter sekaligus di Serial Plotter. |
| `PotensiometerHalus` | Potensiometer 0–100% yang angkanya tidak bergoyang. |
| `JarakHCSR04Halus` | HC-SR04 bebas bacaan nyasar: median lalu EMA, bacaan tanpa pantulan diabaikan. |
| `SuhuAnalogHalus` | Suhu LM35 yang tenang dengan `FilterKalman`. |
| `MemilihFilter` | Tabel perbandingan error dan tunda tiap filter, tanpa sensor. |

## Dibanding library lain

Dari membaca source code library populer di Library Manager (Oktober 2026):

| Library | Temuan |
|---|---|
| RunningAverage | Buffer dibuat dengan `malloc()`. `getFastAverage()` memakai jumlah berjalan float yang, menurut README-nya sendiri, bisa drift. `getAverage()` menjumlah ulang seluruh buffer di setiap panggilan. |
| RunningMedian | Buffer dibuat dengan `malloc()` secara default (`RUNNING_MEDIAN_USE_MALLOC`). |
| movingAvg | Buffer dibuat dengan `new` di `begin()`. Hanya untuk `int`, hasil dibulatkan ke bilangan bulat. |
| SimpleKalmanFilter | Estimasi awal 0. Dengan setelan contohnya `(2, 2, 0.01)` dan bacaan tetap 25, hasil pertama 12,5 dan baru mencapai 99% setelah 66 sampel (diukur dengan menyalin rumusnya ke PC). |

FilterSensor tidak memakai alokasi dinamis, mulai dari bacaan pertama, dan menghitung ulang jumlah rata-rata bergerak setiap putaran buffer.

## Pengujian

Hasil filter diuji otomatis di PC (`extras/test`): nilai konstan, langkah, lonjakan tunggal, buffer belum penuh, `NAN`, `reset()`, median dibanding sort penuh untuk 5000 sampel acak, rata-rata bergerak 1 juta sampel tanpa drift, dan tabel perbandingan di atas.

```sh
cd extras/test
g++ -std=c++11 -I. -I../../src uji.cpp -o uji && ./uji
```

## Status

Versi 1.0.0 sudah lolos uji logika otomatis di PC dan compile tanpa warning di Uno, ESP32, dan STM32 Bluepill (CI menguji 7 board). Library ini murni perhitungan, jadi tidak bergantung pada hardware tertentu. Contoh yang memakai sensor belum dicoba dengan sensor sungguhan. Jika menemukan masalah, silakan buka *issue* di GitHub.

## Lisensi

MIT © 2026 Amadeo Wisesa. Lihat [LICENSE](LICENSE).
