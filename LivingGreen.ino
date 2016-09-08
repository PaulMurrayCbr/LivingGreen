#include <Adafruit_NeoPixel.h>

// http://hyperphysics.phy-astr.gsu.edu/hbase/waves/watwav2.html

// v is proportional to sqrt(l tanh(d/l)) - d is water depth
// if I care about that, I probably need to precompute it, too.

const int sampleSize = 50;
float sampleValue[sampleSize];

const int nPixels = 50;

class SincPulse {
    float width = 1;
    float pos = 0;
    float v = 0;
    uint32_t startt;

  public:
    void start() {
      width = 20;
      v = 1;
      startt = millis();
    }

    void update() {
      pos = (millis() - startt) / 1000.0; // seconds
      pos *= v; // pixels per second
      pos -= width; // offset to begin whole of pulse before strip start
    }

    float y(float x) {
      x -= pos; // position of x along the current pulse;
      x /= width; // position within the puls
      x *= sampleSize; // position within the sync sample
      if (x < 0 || x >= sampleSize - 1) return 0;
      int fl = floor(x); // lower sample
      return sampleValue[fl] * (1 - (x - fl)) + sampleValue[fl + 1] * (x - fl); // interpolate
    }
};

SincPulse pulse;

const Adafruit_NeoPixel pixels(nPixels, 6,
                               //  NEO_RGB + NEO_KHZ800); // Neopixel strip
                               NEO_GRB + NEO_KHZ800); // light strip

void setup() {
  Serial.begin(115200);

  precompute_sample();
  pinMode(11, INPUT_PULLUP);
  pixels.begin();
  pixels.clear();
  pixels.show();
  pulse.start();
}

void loop() {
  {
    static int v = HIGH;
    int vWas = v;
    v = digitalRead(11);
    if (v == LOW && vWas == HIGH) {
      pulse.start();
    }
  }

  {
    static uint32_t tt;
    if (pixels.canShow()) {
      tt = millis();
      pulse.update();
      for (int x = 0; x < nPixels; x++) {
        float y = pulse.y(x);
        if (y >= 1) {
          pixels.setPixelColor(x, cc(1, 0, 0));
        }
        else if (y <= -1) {
          pixels.setPixelColor(x, cc(0, 0, 1));
        }
        else if (y == 0) {
          pixels.setPixelColor(x, 0);
        }
        else if (y > 0) {
          pixels.setPixelColor(x, cc(0, y, y));
        }
        else {
          pixels.setPixelColor(x, cc(-y, -y, 0));
        }
      }
      pixels.show();
    }
  }

}



// https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix

const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

uint32_t cc(float r, float g, float b) {
  if (r < 0) r = 0; if (r > .999) r = .999;
  if (g < 0) g = 0; if (g > .999) g = .999;
  if (b < 0) b = 0; if (b > .999) b = .999;

  uint8_t rr = r * 256;
  uint8_t gg = g * 256;
  uint8_t bb = b * 256;

  // gamma corection
  // https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
  return pixels.Color(
           pgm_read_byte(&gamma8[rr]),
           pgm_read_byte(&gamma8[gg]),
           pgm_read_byte(&gamma8[bb])
         );
}

void precompute_sample() {
  for (int i = 0; i < sampleSize; i++) {
#if 0
    // test pulse
    sampleValue[i] = ((float)i) / sampleSize * 3 - 1.5;

#else
    float x = ((float)i / (float)sampleSize * 2 - 1);

    sampleValue[i] = 
    sin(x*2*3.14159)   // a sine wave. 
    * exp(-2.5 * x * x); // gaussian window width seems to be about right

#endif
    Serial.println(sampleValue[i] );
  }
}

