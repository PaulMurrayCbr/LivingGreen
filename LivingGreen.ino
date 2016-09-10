#include <Adafruit_NeoPixel.h>

// PINOUT

const byte pixelPin = 6;
const byte hitidePin = 7;
const byte lotidePin = 8;
const byte infoPin = 13;
const byte potPin = A3;

/* 200 is a lot, but I'm working on the assumption that we will be stringing a few strands together. */

const int nPixels = 200;

const Adafruit_NeoPixel pixels(nPixels, 6,
                               NEO_RGB + NEO_KHZ800);   // value for the individual strip thing
//                               NEO_GRB + NEO_KHZ800);   // value for the neopixel string


const uint32_t buttonPauseMs = 1000;
uint32_t buttonChangeMs = 0;
byte buttonState = 0;

float hitideBrighness = 1;
float lotideBrighness = .1;
float baseSpeed = 1;

enum State {
  DRAWING = 0,
  ADJUSTING_LOTIDE = 1,
  ADJUSTING_HITIDE = 2,
  ADJUSTING_SPEED = 3
} state = DRAWING;


void setup() {
  // put your setup code here, to run once:

  pinMode(pixelPin, OUTPUT);
  pinMode(infoPin, OUTPUT);
  pinMode(hitidePin, INPUT_PULLUP);
  pinMode(lotidePin, INPUT_PULLUP);
  pinMode(potPin, INPUT);

  pixels.begin();
  pixels.clear();
  pixels.show();
}

void loop() {
  read_buttons();

  switch (state) {
    case DRAWING: break;
    case ADJUSTING_LOTIDE:
      lotideBrighness = analogRead(potPin) / 1023.0;
      break;
    case ADJUSTING_HITIDE:
      hitideBrighness = analogRead(potPin) / 1023.0;
      break;
    case ADJUSTING_SPEED:
      // speed goes from times 10 to one tenth. Maybe that's too much adjustment - we'll see what it looks like.
      baseSpeed =  exp((analogRead(potPin) - 512.0) / 512.0 * 2.302585092994046);
      break;
  }

  draw();
}

void read_buttons() {
  byte buttonStateWas = buttonState;

  buttonState = (digitalRead(lotidePin) == LOW ? 1 : 0)  | (digitalRead(hitidePin) == LOW ? 2 : 0);
    
  if (buttonState == 0) {
    digitalWrite(infoPin, LOW);
    state = DRAWING;
  }
  else if (buttonStateWas != buttonState) {
    buttonChangeMs = millis();
    digitalWrite(infoPin, HIGH);
    state = DRAWING;
  }
  else if (millis() - buttonChangeMs >= buttonPauseMs) {
    digitalWrite(infoPin, LOW);
    state = buttonState & 3; // the button state bitmask matches the state enum
  }
}

#define PHI  1.6180339887499
#define PI  3.141592653589793


class Sine {
  float speedHz = 1; // cycles per second
  float theta = 0; // current value;
  float wavelengthPix = 1; // wavelength in pixels

  static Sine* head;
  Sine* next;

  void advanceMicros(float us) {
    theta += 2 * PI * us / 1e6 * speedHz;
    while(theta < 0) theta += 2 * PI;
    while(theta >= 2 * PI) theta -= 2 * PI;
  }

public:
  Sine(float speedHz, float wavelengthPix)
  : speedHz(speedHz), wavelengthPix(wavelengthPix)
  {
    next = head;
    head = this; 
  }


  static void advanceAllMicros(float us) {
    for(Sine *p = head; p; p = p->next) {
      p->advanceMicros(us);
    }
  }

  float v(int pix) {
      return sin(theta + pix/wavelengthPix *  2 * PI) / 2 + .5;
  }

  float v_for_mul(int pix) {
      return sin(theta + pix/wavelengthPix *  2 * PI);
  }
  
};

Sine *Sine::head = NULL;

uint32_t mostRecentDrawUs = 0;

// we have ripples with a wavelength of the golden ratio.
Sine rippleBase(1.0/5, 1/PHI);
// the ripples are multiplied in amplitude by a sine that moves quickly and has a long wavelength
Sine rippleMul(1, 30);

// the tide has a cycle of once every 30 minutes. This means that we compress a day into an hour
Sine tide(1.0/60/30, 1);

// The waves moving in have a long wavelenght and move reasonably fast
Sine waveIn(.15, 20);
// The waves moving out have a slightly shorter wavelenght and move slower in the opposote direction
Sine waveOut(-.1, 17.1234);

// the waves move in and out nice and slow to make it obvious. I use a wavlenght of 2 pixels and sampe the amplitude at 0 and 1
// to give me the weighting of wave in vs wave out.  

Sine waveInOut(1.0/120, 2);

void draw() {
  if(!pixels.canShow()) return;
  delay(1); // have at least a little delay
  uint32_t us = micros();
  Sine::advanceAllMicros((us - mostRecentDrawUs) * baseSpeed);
  mostRecentDrawUs = us;

  float waveInWeight = waveInOut.v(0);
  float waveOutWeight = 1 - waveOutWeight;

  float tideV;

  if(state == ADJUSTING_HITIDE) {
    tideV = 1; // force high tide
  }
  else if(state == ADJUSTING_LOTIDE) {
    tideV = 0; // force low tide
  }
  else {
    tideV = tide.v(0);
  }

  tideV = (tideV * hitideBrighness) + ((1-tideV) * lotideBrighness); 

  for(int i = 0; i<nPixels; i++) {
    float rippleV = rippleBase.v_for_mul(i) * (rippleMul.v(i)*1+1)/2;
    // correct rippleV
    rippleV = rippleV/2 + .5;

    float inV = (waveIn.v(i)) * waveInWeight * tideV;
    float outV = (waveOut.v(i)) * waveOutWeight * tideV;
    
    float r,g,b;

    // base colour - a nice sea-green maybe a bit washed out
    // the brighness is .3, so the other components have .7 to play with 
    r = .1;
    g = .3;
    b = .1;

    // ripple colour: blue, and increase saturation. 

    r += -.05 * rippleV;
    b += .2 * rippleV;

    // wave in: a bright green

    g += inV * .7;
    b += inV * .2;

    // wave out, a dimmer cyan

    g += outV * .5;
    b += outV * .5;
    
    pixels.setPixelColor(i, cc(r,g,b));

    
  }
  pixels.show();
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

