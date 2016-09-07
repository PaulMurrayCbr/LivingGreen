#include <Adafruit_NeoPixel.h>

const Adafruit_NeoPixel pixels(50, 5,
                               NEO_RGB + NEO_KHZ800);
//NEO_GRB + NEO_KHZ800);

const int sincSz = 200;

float sincValue[sincSz];


void setup() {
  precompute_sinc();
}

void loop() {}


void precompute_sinc() {
  for (int i = 0; i < sincSz; i++) {
    float x = ((float)(i - sincSz / 2)) / (sincSz / 2) * 3.1415926f * 12;

    if (x > -1e-4 && x < 1e4) {
      sincValue[i] = 1;
    }
    else {
      sincValue[i] = sin(x) / x;
    }
  }
}

