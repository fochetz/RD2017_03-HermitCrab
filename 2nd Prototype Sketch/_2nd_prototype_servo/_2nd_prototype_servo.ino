/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Sweep
*/

#include <Servo.h>
#define SERVOPIN 6
Servo shell;
int pos;   // variable to store the servo position

void setup() {
  shell.attach(SERVOPIN);
}

void loop() {
  for (pos = 0; pos <= 180; pos += 1) 
  {
    shell.write(pos);
    delay(1);
  }
  for (pos = 180; pos >= 0; pos -= 1) 
  {
    shell.write(pos);
    delay(1);
   }
  delay(500);
}

