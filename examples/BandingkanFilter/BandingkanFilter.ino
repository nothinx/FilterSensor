// Bandingkan bacaan mentah dengan semua filter di Serial Plotter (115200).
//
// Sambungan: sensor analog apa saja ke A0, mis. potensiometer (kaki tengah ke A0,
// kaki pinggir ke GND dan 5V/3V3). Untuk melihat tahan-lonjakan, sentuh kabel A0
// dengan jari atau biarkan A0 tidak tersambung.
#include <FilterSensor.h>

RataRataBergerak<8> rataRata;
FilterMedian<5> median;
FilterEMA ema(0.2);
FilterKalman kalman(5, 1); // noise bacaan +-5, nilai asli berubah +-1 per sampel

void setup() {
  Serial.begin(115200);
}

void loop() {
  static uint32_t terakhir = 0;
  if (millis() - terakhir < 20) return; // 50 sampel per detik
  terakhir = millis();

  float mentah = analogRead(A0);
  Serial.print("mentah:");
  Serial.print(mentah);
  Serial.print(",rataRata:");
  Serial.print(rataRata.saring(mentah));
  Serial.print(",median:");
  Serial.print(median.saring(mentah));
  Serial.print(",ema:");
  Serial.print(ema.saring(mentah));
  Serial.print(",kalman:");
  Serial.println(kalman.saring(mentah));
}
