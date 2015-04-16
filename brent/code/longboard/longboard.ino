/**

I really need to rewrite this.

**/


#include <stdlib.h>
#include <TM1638.h>
#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <Servo.h> 

#define LOOP_DELAY 40
#define BAUDRATE 19200


Servo esc;
ArduinoNunchuk nunchuk = ArduinoNunchuk();
float throttle = 0;
TM1638 module(5, 3, 4);

boolean startup = true;

uint8_t deathCount = 3;
uint16_t oldAccelX = 1024;
uint16_t oldAccelY = 1024;

boolean displayThrottle = false;

void setup()
{
  Serial.begin(BAUDRATE);
  esc.attach(9);
  module.clearDisplay();
  module.setLEDs(0x0F);
  delay(100);
  nunchuk.init();
  module.setLEDs(0x00);
  
}

void loop()
{
  long start_time = millis();

  if(module.getButtons() == 0b00000001){
    displayThrottle = !displayThrottle;
    delay(100);
  }
  
  if(displayThrottle){
    module.setLED(TM1638_COLOR_GREEN, 0);
    module.setDisplayToSignedDecNumber((int)(throttle * 100), 0b00000100, false); 
  }else{
    module.setLED(TM1638_COLOR_NONE, 0); 
  }

  nunchuk.update();

  if(nunchuk.cButton){
    if(throttle > 1 || nunchuk.zButton){
      throttle = 0;
      delay(100);
    }
    else if(!nunchuk.zButton){
      throttle -= 2;
    }
  }

  int8_t stickY = nunchuk.analogY - 128;
  int8_t stickX = nunchuk.analogX - 128;
  
  module.setLED(TM1638_COLOR_NONE, 7);
  module.setLED(TM1638_COLOR_NONE, 6);
  if(!nunchuk.zButton && !nunchuk.cButton){
    if(throttle > 0){
      throttle -= 0.25;
    }else if(throttle < 0){
      throttle += 2;
    }
    if(abs(throttle) < 3){
      throttle = 0;
      startup = true; 
    }
    else{
      startup = false; 
    }
  }
  else if(nunchuk.zButton){
    if(abs(stickY) > 5){
      throttle += sqrt(abs(stickY)) * signum(stickY) / 100.0;
      module.setLED(TM1638_COLOR_RED, 6);
    }
    else if(startup){
      if(throttle < 4.5){
        throttle = 4.5;
      }
      module.setLED(TM1638_COLOR_RED, 7);
      throttle += 0.025; 
      if(throttle > 6){
        startup = false; 
      }
    }
  }


  if(abs(throttle) > 100){
    throttle = 100 * signum(throttle);
  }

  if(nunchuk.accelX == oldAccelX && nunchuk.accelY == oldAccelY && nunchuk.accelX == nunchuk.accelY){
    deathCount++; 
  }
  else{
    deathCount = 0;
  }

  if(deathCount > 3){//something bad happened
    esc.write(90);
    throttle *= 0.95;
    module.setDisplayToString("SIGNAl");
    nunchuk.init();
  }
  else{
    uint8_t output = 90;
    if(throttle > 0){
      output += 90.0 * throttle / 100.0;
    }else if(nunchuk.cButton && stickY < -100){
      output += 30.0 * throttle / 100.0; 
    }else{
      output += 20.0 * throttle / 100.0; 
    }
    
    if(throttle > 0 && !nunchuk.zButton){
      esc.write(90); 
    }else{
      esc.write(output);
    }
  }
  oldAccelX = nunchuk.accelX;
  oldAccelY = nunchuk.accelY;

  if(stickX > 50){
    long voltage = 0;
    for(int i = 0; i < 50; i++){
      voltage += ((int) analogRead(A1) * 40.0) * 100 / 1023; 
      delay(2);
    }
    voltage /= 50;
    voltage += 100; //sketchy calibration
    
    voltage /= 5;
    voltage *= 6;

    module.setDisplayToDecNumber(voltage, 0b00000100, false); 

    float cellVoltage = voltage / 600.0;

    byte leds = 0xFF;
    Serial.println(cellVoltage);
    if(cellVoltage > 4.15){
      leds = leds >> 0;
    }
    else if(cellVoltage > 4.05){
      leds = leds >> 1;
    }
    else if(cellVoltage > 4){
      leds = leds >> 2;
    }
    else if(cellVoltage > 3.9) {
      leds = leds >> 3;
    }
    else if(cellVoltage > 3.85) {
      leds = leds >> 4;
    }
    else if(cellVoltage > 3.8) {
      leds = leds >> 5;
    }
    else if(cellVoltage > 3.75){        
      leds = leds >> 6;
    }
    else if(cellVoltage > 3.7){
      leds = leds >> 7;
    }
    else{
      leds = leds >> 8;
    }
    module.setLEDs( leds );
  }
  if(stickX < -50){
    module.clearDisplay();
    module.setLEDs(0x00);
  }

  Serial.println(throttle);

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
