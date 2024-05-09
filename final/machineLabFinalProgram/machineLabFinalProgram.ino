/// TODO:
/// boat servo (import code)
/// wave servo 1-3
/// clam servo 1-2


// include SPI, MP3 and SD libraries
#include <Servo.h>
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif
// define the pins used
//#define CLK 13       // SPI Clock, shared with SD card
//#define MISO 12      // Input data, from VS1053/SD card
//#define MOSI 11      // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins.
// See http://arduino.cc/en/Reference/SPI "Connections"

// These are the pins used for the music maker shield
#define SHIELD_RESET -1  // VS1053 reset pin (unused!)
#define SHIELD_CS 7      // VS1053 chip select pin (output)
#define SHIELD_DCS 6     // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4  // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3  // VS1053 Data request, ideally an Interrupt pin

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN A0
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 22
//72 was original

//Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

int boatServoPin = A3;
int startButton = A4;
int shellLed1 = 9;
int shellLed2 = 10;
int shellServoPin = 8;
int wave1Pin = 2;
int wave2Pin = A2;
int wave3Pin = A1;

int boatPos = 90;
int shellPos = 0;
bool isStarted = false;
unsigned long currentTime = 0;


class Sweeper {
  Servo servo;               // the servo
  int pos;                   // current servo position
  int increment;             // increment to move for each interval
  int updateInterval;        // interval between updates
  unsigned long lastUpdate;  // last update of position
  int minPos;
  int maxPos;

public:
  Sweeper(int interval, int incrementamt, int minpos, int maxpos, int startingpos) {
    updateInterval = interval;
    increment = incrementamt;
    minPos = minpos;
    maxPos = maxpos;
    pos = startingpos;
  }

  void Attach(uint8_t pin) {
    servo.attach(pin);
  }

  void Detach() {
    servo.detach();
  }

  void Update() {
    if ((millis() - lastUpdate) > updateInterval)  // time to update
    {
      lastUpdate = millis();
      pos += increment;
      servo.write(pos);
      //Serial.println(pos);
      if ((pos >= maxPos) || (pos <= minPos))  // end of sweep
      {
        // reverse direction
        increment = -increment;
      }
    }
  }
};

class NeoPixelControl {
  int pixPin = 0;
  int numPix = 0;

  unsigned long pixelPrevious = 0;    // Previous Pixel Millis
  unsigned long patternPrevious = 0;  // Previous Pattern Millis
  int patternCurrent = 0;             // Current Pattern Number
  int patternInterval = 5000;         // Pattern Interval (ms)
  bool patternComplete = false;

  int pixelInterval = 50;  // Pixel Interval (ms)
  int pixelQueue = 0;      // Pattern Pixel Queue
  int pixelCycle = 0;      // Pattern Pixel Cycle
  //uint16_t      pixelNumber = LED_COUNT;

  Adafruit_NeoPixel pixels;

public:
  NeoPixelControl(int _neoPixelPin, int _numberOfPixels,
                  int _delayBetweenSteps) {
    pixPin = _neoPixelPin;
    numPix = _numberOfPixels;

    pixels = Adafruit_NeoPixel(numPix, pixPin, NEO_GRB + NEO_KHZ800);
  }

  void startupPixels() {
    pixels.begin();
    pixels.clear();
    pixels.show();
    pixels.setBrightness(25);
  }

  void colorWipe(uint32_t color, int wait) {
    static uint16_t current_pixel = 0;
    pixelInterval = wait;                          //  Update delay time
    pixels.setPixelColor(current_pixel++, color);  //  Set pixel's color (in RAM)
    pixels.show();                                 //  Update strip to match
    if (current_pixel >= numPix) {                 //  Loop the pattern from the first LED
      current_pixel = 0;
      patternComplete = true;
    }
  }

  void colorAlternate(uint32_t color1, uint32_t color2, int wait) {
    static uint32_t loop_count = 0;
    pixelInterval = wait;

    if (loop_count % 2 == 0) {
      for (int i = 0; i < numPix; i++) {
        if (i % 2 == 0) {
          pixels.setPixelColor(i, color1);
        } else {
          pixels.setPixelColor(i, color2);
        }
      }
    } else {
      for (int i = 0; i < numPix; i++) {
        if (i % 2 == 0) {
          pixels.setPixelColor(i, color2);
        } else {
          pixels.setPixelColor(i, color1);
        }
      }
    }
    pixels.show();
    loop_count++;
    if (loop_count >= 4) {
      loop_count = 0;
      patternComplete = true;
    }
  }

  void blink(uint32_t color, int wait) {
    static uint32_t loop_count = 0;
    pixelInterval = wait;
    if (loop_count % 2 == 0) {
      for (int i = 0; i < numPix; i++) {
        pixels.setPixelColor(i, color);
      }
    } else {
      for (int i = 0; i < numPix; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
    }
    pixels.show();
    loop_count++;
    if (loop_count >= 30) {
      loop_count = 0;
      patternComplete = true;
    }
  }

  void randomColors(int wait) {
    static uint32_t loop_count = 0;
    pixelInterval = wait;

    for (int i = 0; i < numPix; i++) {
      int randR = random(0, 255);
      int randG = random(0, 255);
      int randB = random(0, 255);
      pixels.setPixelColor(i, pixels.Color(randR, randG, randB));
    }
    pixels.show();
    loop_count++;
    if (loop_count >= 30) {
      loop_count = 0;
      patternComplete = true;
    }
  }

  void update() {

    // Serial.print("neopixel strip update function; state = ");
    // Serial.print(patternCurrent);
    // Serial.println();



    unsigned long currentMillis = millis();                                         //  Update current time
    if (patternComplete || (currentMillis - patternPrevious) >= patternInterval) {  //  Check for expired time
      patternComplete = false;
      patternPrevious = currentMillis;
      patternCurrent++;  //  Advance to next pattern
      if (patternCurrent >= 3)
        patternCurrent = 0;
    }

    if (currentMillis - pixelPrevious >= pixelInterval) {  //  Check for expired time
      pixelPrevious = currentMillis;                       //  Run current frame
      switch (patternCurrent) {
        case 3:
          randomColors(100);
          Serial.println(F("3"));
          break;
        case 2:
          blink(pixels.Color(255, 255, 0), 300);
          Serial.println(F("2"));
          break;
        case 1:
          colorAlternate(pixels.Color(255, 255, 0), pixels.Color(0, 0, 0), 500);
          Serial.println(F("1"));
          break;
        default:
          colorWipe(pixels.Color(255, 255, 0), 50);
          Serial.println(F("0"));
          break;
      }
    }
  }
};

//music stuff doesnt work, idk why
// class MusicControl {
//   int resetPin;
//   int csPin;
//   int cmdPin;
//   int cardPin;
//   int dreqPin;

//   int count = 0;

//   Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

// public:
//   MusicControl(int8_t _resetPin, int8_t _csPin, int8_t _cmdPin, int8_t _dreqPin, int8_t _cardPin) {
//     resetPin = _resetPin;
//     csPin = _csPin;
//     cmdPin = _cmdPin;
//     cardPin = _cardPin;
//     dreqPin = _dreqPin;

//     // musicPlayer = Adafruit_VS1053_FilePlayer(resetPin, csPin, cmdPin, dreqPin, cardPin);
//   }

//   void setup() {
//     if (!musicPlayer.begin()) {  // initialise the music player
//       Serial.println(F("Couldn't find Music Player, do you have the right pins defined?"));
//       while (1)
//         ;
//     }
//     Serial.println(F("Music Player found"));

//     if (!SD.begin(CARDCS)) {
//       Serial.println(F("SD failed, or not present"));
//       // Adafruit_NeoPixel strip = Adafruit_NeoPixel(22, A0, NEO_GRB + NEO_KHZ800);
//       // strip.setPixelColor(0, strip.Color(255,0,0));
//       while (1)
//         ;  // don't do anything more
//     }
//     musicPlayer.setVolume(0, 0);
//     musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
//   }

//   void printDirectory(File dir, int numTabs) {
//     while (true) {

//       File entry = dir.openNextFile();
//       if (!entry) {
//         // no more files
//         //Serial.println("**nomorefiles**");
//         break;
//       }
//       for (uint8_t i = 0; i < numTabs; i++) {
//         Serial.print('\t');
//       }
//       Serial.print(entry.name());
//       if (entry.isDirectory()) {
//         Serial.println("/");
//         printDirectory(entry, numTabs + 1);
//       } else {
//         // files have sizes, directories do not
//         Serial.print("\t\t");
//         Serial.println(entry.size(), DEC);
//       }
//       entry.close();
//     }
//   }

//   void update() {
//     // if (musicPlayer.stopped() && count%2 == 0) {
//     //   Serial.println(F("Playing track 001"));
//     //   musicPlayer.startPlayingFile("/track001.mp3");
//     //   count++;
//     // } else if (musicPlayer.stopped() && count%2 == 1) {
//     //   Serial.println(F("Playing track 002"));
//     //   musicPlayer.startPlayingFile("/track001.mp3");
//     //   count++;
//     // }
//     if (musicPlayer.stopped()) {
//       Serial.println(F("Playing track 001"));
//       musicPlayer.startPlayingFile("/track001.mp3");
//     }
//   }
// };

//PWM pins are not taking analog outputs for whatever reason, so just keeping them as digital
// class LEDControl {
//   int pin;
//   int brightness = 64;
//   int increment = 5;
//   int updateInterval;
//   unsigned long lastUpdate;

// public:
//   LEDControl(uint8_t _pin, int _updateInterval) {
//     pin = _pin;
//     updateInterval = _updateInterval;
//   }

//   void startupLED() {
//     pinMode(pin, OUTPUT);
//   }

//   void update() {
//     if ((millis() - lastUpdate) > updateInterval)  // time to update
//     {
//       lastUpdate = millis();
//       brightness += increment;
//       analogWrite(pin, 128);
//       //digitalWrite(pin, HIGH);
//       if ((brightness >= 255) || (brightness <= 64))  // end of sweep
//       {
//         // reverse direction
//         increment = -increment;
//       }
//     }
//   }
// };

Sweeper boatServo(20, 1, 45, 135, 90);
Sweeper shellServo(20, 1, 0, 180, 90);
Sweeper wave1(10, 1, 70, 130, 90);
Sweeper wave2(10, 1, 70, 130, 90);
Sweeper wave3(10, 1, 70, 130, 90);
// LEDControl pearl1(shellLed1, 20);
// LEDControl pearl2(shellLed2, 20);
NeoPixelControl strip(A0, 22, 10);
//MusicControl musicController(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

void setup() {
  Serial.begin(9600);
  Serial.print("\n\n\n\n\n\n\n\n");
  // if (!musicPlayer.begin()) {  // initialise the music player
  //   Serial.println(F("Couldn't find Music Player, do you have the right pins defined?"));
  //   while (1)
  //     ;
  // }
  // Serial.println(F("Music Player found"));

  // if (!SD.begin(CARDCS)) {
  //   Serial.println(F("SD failed, or not present"));
  //   while (1)
  //     ;  // don't do anything more
  // }

  // // list files
  // //printDirectory(SD.open("/"), 0);

  // // Set volume for left, right channels. lower numbers == louder volume!
  // musicPlayer.setVolume(20, 20);

  // // Timer interrupts are not suggested, better to use DREQ interrupt!
  // //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

  // // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // // audio playing
  // musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  // if (!musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT)) {
  //   Serial.println(F("DREQ pin is not an interrupt pin"));
  // }
  // if (SD.exists("/track001.mp3")) {
  //   Serial.println(F("found"));
  // } else {
  //   Serial.println(F("not found"));
  // }
  // if (!musicPlayer.startPlayingFile("/track001.mp3")) {
  //   Serial.println(F("failed"));
  // } else {
  //   Serial.println(F("playing"));
  // }




  // #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  //   clock_prescale_set(clock_div_1);
  // #endif
  // END of Trinket-specific code.
  Serial.println(F("Initializing Neopixels..."));
  strip.startupPixels();

  //Serial.println("Looking for Music Player...");
  //musicController.setup();
  //musicController.printDirectory(SD.open("/"), 0);

  Serial.println(F("Initializing Servos..."));
  shellServo.Attach(shellServoPin);
  // boatServo.Attach(boatServoPin);
  wave1.Attach(wave1Pin);
  wave2.Attach(wave2Pin);
  wave3.Attach(wave3Pin);

  Serial.println(F("Initializing LEDs..."));
  // pearl1.startupLED();
  // pearl2.startupLED();
  pinMode(shellLed1, OUTPUT);
  pinMode(shellLed2, OUTPUT);

  //button setup
  pinMode(startButton, INPUT);

  Serial.println(F("Ready to Start!"));
}



void loop() {
  int switchPosition = digitalRead(startButton);
  if (switchPosition == HIGH && !isStarted) {
    Serial.println(F("Started"));
    isStarted = true;
    currentTime = millis();
    // Serial.println(F("Playing track 001"));
    // musicPlayer.startPlayingFile("/track001.mp3");
    digitalWrite(shellLed1, HIGH);
    digitalWrite(shellLed2, HIGH);
  }

  if (isStarted) {
    Serial.println(F("update"));
    boatServo.Update();
    shellServo.Update();
    wave1.Update();
    //wave2.Update();
    wave3.Update();
    pearl1.update();
    pearl2.update();


    strip.update();


    //musicController.update();

    // if (musicPlayer.stopped()) {
    //   Serial.println(F("Playing track 001"));
    //   musicPlayer.startPlayingFile("/track001.mp3");
    // }

    if (millis() - currentTime >= 45000) {
      isStarted = false;
      digitalWrite(shellLed1, LOW);
      digitalWrite(shellLed2, LOW);
      Serial.println(F("Done"));
    }
  }

  //delay(100);
}
