/*
the relatively more organized version of my electric skateboard code
 
 still kind of just hacked together though
 */

#include <TM1638.h>
#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <Servo.h> 

#define DISPLAY true
#define LOOP_DELAY 40
#define BAUDRATE 19200

Servo esc;
ArduinoNunchuk nunchuk = ArduinoNunchuk();
float throttle = 0;
#ifndef DISPLAY
TM1638 module(5, 3, 4);
#endif

uint8_t deathCount = 0;
uint16_t oldAccelX = 1024;
uint16_t oldAccelY = 1024;

//#define VOLTAGE_COEFF 44000
#define VOLTAGE_COEFF 54000

#define DISPLAY_VOLTAGE 0
#define DISPLAY_THROTTLE 1
#define DISPLAY_OUTPUT 2

#define USE_DO

uint8_t displayMode = DISPLAY_VOLTAGE;
boolean displayOn = false;
boolean oldCButton = false;

uint16_t voltage = 0;

uint8_t nunchuk_reset_counter = 0;

void setup()
{
  Serial.begin(BAUDRATE);
  esc.attach(9);
#ifndef DISPLAY
  module.clearDisplay();
  module.setLEDs(0xFF00);
#endif
  delay(100);
  nunchuk.init();
}

void loop()
{
  long start_time = millis();


  nunchuk.update();

  nunchuk_reset_counter++;
  nunchuk_reset_counter %= 30;
  if(nunchuk_reset_counter == 0){
    nunchuk.init(); 
  }
  
  int8_t stickY = nunchuk.analogY - 128;
  int8_t stickX = nunchuk.analogX - 128;


  byte leds = 0x00;
  if(deathCount <= 250 && (nunchuk.analogX > 255 || nunchuk.analogY > 255 || nunchuk.analogX < 0 || nunchuk.analogY < 0
    || nunchuk.accelX < 0 || nunchuk.accelX > 1023 || nunchuk.accelY < 0 || nunchuk.accelY > 1023
    || (nunchuk.accelX == nunchuk.accelY && nunchuk.accelX == oldAccelX && nunchuk.accelY == oldAccelY))){
    deathCount += 5;
  }
  else if(deathCount <= 254 && (nunchuk.accelX == oldAccelX && nunchuk.accelY == oldAccelY)){
    deathCount += 1;
    leds = 0x0F;
  }
  else{
    if(deathCount >= 25){
      deathCount -= 25;
    }
    else{
      deathCount = 0; 
    }
  }
  oldAccelX = nunchuk.accelX;
  oldAccelY = nunchuk.accelY;
  if(nunchuk.cButton && oldCButton){
    oldCButton = nunchuk.cButton;
  }

  else{
    oldCButton = nunchuk.cButton;
    nunchuk.cButton = false; 
  }

  uint8_t output = 90;
  if(deathCount >= 15){ //problem with the nunchuck :(    
    throttle *= 0.95;
#ifndef DISPLAY
    module.setDisplayToString("SIGNAL");
#endif
    nunchuk.init();
  }
  else{//everything's working!
    if(nunchuk.zButton && nunchuk.cButton){ //coast!
      throttle = 0;
    }
    else if(nunchuk.zButton){ //apply throttle!
      leds |= 1 << 7;

      leds &= ~(11 << 5);
      if(throttle == 6){
        leds |= 1 << 6; 
      }

      if(throttle >= 0 && throttle < 6){
        throttle = 6;
      }
      else if(abs(stickY) > 5){
        leds |= 1 << 5;
        if(throttle > 25){
          throttle += sqrt(abs(stickY)) * signum(stickY) / 50.0;
        }
        else{
          throttle += sqrt(abs(stickY)) * signum(stickY) / 20.0;
        }
      }
    }
    else if(nunchuk.cButton){ //brake and reverse!
      if(stickY < -50){
        throttle = -40.0;
      }
      else{
        throttle = min(-25, throttle - 12);
        leds |= 1 << 4;
      }
    }
    else{ //no button input, assume coast
      if(throttle < 0){
        throttle = 0;
      }
      else{
        throttle -= 0.1;
        throttle *= 0.9925;
      }
      if(abs(throttle) < 5){
        throttle = 0;
      }
    }

    if(abs(throttle) > 100){
      throttle = 100 * signum(throttle);
    }

    if(nunchuk.zButton || throttle < 0){
      if(throttle > 0){
        output += 90.0 * throttle / 100.0;
      }
      else if(nunchuk.cButton && stickY < -100){
        output += 25.0 * throttle / 100.0;
      }
      else{
        output += 25.0 * throttle / 100.0;
      }
    }

#ifndef DISPLAY

    module.clearDisplay();
    if(stickX < -100){
      displayOn = false;
      module.clearDisplay();
    }
    else if(stickX > 100){
      displayOn = true;
      voltage = 0;
    }
#endif
  }

  esc.write(output);
#ifndef DISPLAY
  module.setLEDs(leds);
  byte buttons = module.getButtons();
  if(buttons & 1){
    displayMode = DISPLAY_VOLTAGE;
  }
  else if(buttons >> 1 & 1){
    displayMode = DISPLAY_THROTTLE;
  }
  else if(buttons >> 2 & 1){
    displayMode = DISPLAY_OUTPUT;
  }

  if(displayOn && deathCount < 150){
    switch(displayMode){
    case DISPLAY_VOLTAGE:
      {
        //voltage reading has hella noise
        //let's "fix" that
        //...
        uint16_t a = analogRead(A1) * VOLTAGE_COEFF / 1023;
        delay(1);
        uint16_t b = analogRead(A1) * VOLTAGE_COEFF / 1023;
        delay(1);
        uint16_t c = analogRead(A1) * VOLTAGE_COEFF / 1023;

        //get the median! (and treat it as current voltage)
        uint16_t c_voltage;
        if(signum(b - a) == signum(a - c)){
          c_voltage = a;
        }
        else if(signum(a - b) == signum(b - c)){
          c_voltage = b;
        }
        else{
          c_voltage = c;
        }

        if(voltage == 0){
          voltage = c_voltage;
        }
        else if(voltage > c_voltage){
          voltage -= 5;
        }
        else if(voltage < c_voltage){
          voltage += 5;
        }

        module.clearDisplay();
        module.setDisplayToDecNumber(voltage / 10, 0b00000100, false);
        break;
      }
    case DISPLAY_THROTTLE:
      {
        module.clearDisplay();
        module.setDisplayToSignedDecNumber((int)(throttle * 100), 0b00000100, false);
        break;
      }
    case DISPLAY_OUTPUT:
      {
        module.clearDisplay();
        module.setDisplayToSignedDecNumber(output, 0x00, false);
        break;
      }
    }
  }
#endif

  long delayLength = LOOP_DELAY - (millis() - start_time);
  if(delayLength > 0){
    delay(delayLength);
  }
}

int8_t signum(int val) {
  if (val < 0) return -1;
  if (val==0) return 0;
  return 1;
}



