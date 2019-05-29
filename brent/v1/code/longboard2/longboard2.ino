#include <Adafruit_NeoPixel.h>

/*
the relatively more organized version of my electric skateboard code

 still kind of just hacked together though
 */

#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <Servo.h>

#define DISPLAY
#define LOOP_DELAY 50
#define BAUDRATE 19200

Servo esc;
ArduinoNunchuk nunchuk = ArduinoNunchuk();
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, 2, NEO_GRB + NEO_KHZ800);
float throttle = 0;

uint8_t deathCount = 0;
uint16_t oldAccelX = 1024;
uint16_t oldAccelY = 1024;

#define USE_DO

uint16_t voltage = 0;

uint8_t nunchuk_reset_counter = 0;

int heartbeat = 0;
int heartbeat_dir = 1;

void setup()
{
  esc.attach(9);
  nunchuk.init();
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.clear();
  pixels.show();
}

void loop()
{
  long start_time = millis();

  nunchuk.update();

  nunchuk_reset_counter++;
  nunchuk_reset_counter %= 30;
  if (nunchuk_reset_counter == 0) {
    nunchuk.init();
  }

  int8_t stickY = nunchuk.analogY - 128;
  int8_t stickX = nunchuk.analogX - 128;


  byte leds = 0x00;
  if (deathCount <= 250 && (nunchuk.analogX > 255 || nunchuk.analogY > 255 || nunchuk.analogX < 0 || nunchuk.analogY < 0
                            || nunchuk.accelX < 0 || nunchuk.accelX > 1023 || nunchuk.accelY < 0 || nunchuk.accelY > 1023
                            || (nunchuk.accelX == nunchuk.accelY && nunchuk.accelX == oldAccelX && nunchuk.accelY == oldAccelY))) {
    deathCount += 5;
  }
  else if (deathCount <= 254 && (nunchuk.accelX == oldAccelX && nunchuk.accelY == oldAccelY)) {
    deathCount += 1;
  }
  else {
    if (deathCount >= 25) {
      deathCount -= 25;
    }
    else {
      deathCount = 0;
    }
  }
  oldAccelX = nunchuk.accelX;
  oldAccelY = nunchuk.accelY;

  uint16_t output = 1500;
  if (deathCount >= 30) { //problem with the nunchuck :(
    throttle *= 0.95;
    nunchuk.init();
  }
  else { //everything's working!
    if (nunchuk.zButton && nunchuk.cButton) {
      //nothing
    }
    else if (nunchuk.zButton) { //apply throttle!
      if (throttle >= 0 && throttle < 8) {
        throttle = 8;
      }
      else if (abs(stickY) > 15) {
        if (throttle > 25) {
          throttle += sqrt(abs(stickY)) * signum(stickY) / 25.0;
        }
        else {
          throttle += sqrt(abs(stickY)) * signum(stickY) / 10.0;
        }
      }
    }
    else if (nunchuk.cButton) { //brake and reverse!
      if (stickY < -50) {
        throttle = -100;
      } else if (stickY > 50) {
        throttle = -30;
      } else {
        throttle = -70;
      }
    }
    else { //no button input, assume coast
      if (throttle < 0) {
        throttle = 0;
      }
      else {
        throttle -= 0.5;
      }
      if (abs(throttle) < 8) {
        throttle = 0;
      }
    }

    if (abs(throttle) > 100) {
      throttle = 100 * signum(throttle);
    }

    if (nunchuk.zButton || throttle < 0) {
      output += 500.0 * throttle / 100.0;
    }
  }


  heartbeat += heartbeat_dir;
  if (heartbeat >= 80) {
    heartbeat_dir = -1;
  } else if (heartbeat <= 0) {
    heartbeat_dir = 1;
  }

  pixels.clear();
  for (int i = 0; i < 8; i++) {
    if (output == 1500) {
      pixels.setPixelColor(i, pixels.Color(heartbeat / 5, 0, heartbeat / 4));
    }
    else {
      if ( i * 100 / 8 < (int)abs(throttle)) {
        if (output < 1500) {
          pixels.setPixelColor(i, pixels.Color(throttle / 2, 0, 0));
        }
        else {
          pixels.setPixelColor(i, pixels.Color(0, 0, throttle / 2));
        }
      } else {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
    }
  }

  pixels.show();
  esc.writeMicroseconds(output);
  long delayLength = LOOP_DELAY - (millis() - start_time);
  if (delayLength > 0) {
    delay(delayLength);
  }
}

int8_t signum(int val) {
  if (val < 0) return -1;
  if (val == 0) return 0;
  return 1;
}



