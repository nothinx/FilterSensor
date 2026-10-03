// Jarak HC-SR04 yang bebas bacaan nyasar. Sensor ultrasonik sering sesekali
// memberi angka ngawur (pantulan miring, gema) atau tidak menjawab sama sekali.
// Median 5 membuang angka ngawur itu, lalu EMA menghaluskan sisanya.
// Hasil di Serial Monitor / Serial Plotter (115200).
//
// Sambungan: VCC ke 5V, GND ke GND, Trig ke pin 4, Echo ke pin 5.
// Board 3,3 V (ESP32, STM32): pasang pembagi tegangan di Echo (mis. 1k dan 2k).
#include <FilterSensor.h>

const uint8_t PIN_TRIG = 4;
const uint8_t PIN_ECHO = 5;

FilterMedian<5> buangNyasar;
FilterEMA halus(0.3);

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
}

// Jarak dalam cm, NAN jika tidak ada pantulan (lebih dari +-5 m).
float bacaJarak() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  uint32_t us = pulseIn(PIN_ECHO, HIGH, 30000);
  return us ? us / 58.0 : NAN;
}

void loop() {
  static uint32_t terakhir = 0;
  if (millis() - terakhir < 60) return; // beri jeda agar gema sebelumnya hilang
  terakhir = millis();

  float mentah = bacaJarak();
  // NAN diabaikan oleh filter, hasil tetap jarak terakhir yang sah.
  float jarak = halus.saring(buangNyasar.saring(mentah));

  Serial.print("mentah:");
  Serial.print(isnan(mentah) ? 0 : mentah);
  Serial.print(",jarak:");
  Serial.println(jarak);
}
