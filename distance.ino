#include <MeMegaPi.h> // don't use the uno, import this library and use hats on the megaPi

// port config
MeLineFollower      lineFinder(PORT_5);  // line follower on shield port 5
MeUltrasonicSensor  ultrasonic(PORT_6);  // distance sensor on shield port 6
MeMegaPiDCMotor     motor(PORT1B);       // motor on driver slot 1 (see note below)

const int FAST = 200;    // speed range is -255 to 255, + is clockwise
const int SLOW = 100;
const int STOP_DISTANCE_CM = 10;

void setup() { // runs once at the beggining
  Serial.begin(9600); // start communication channel to arduino at 9600 bits per sec
}

void loop() {
  int line = lineFinder.readSensors();
  double distance = ultrasonic.distanceCm();
  int speed = 0;

  if (distance < STOP_DISTANCE_CM) {
    speed = 0;                       // obstacle close: stop
  } else {
    switch (line) {
      case S1_IN_S2_IN:   speed = FAST;   break;  // both on black: full forward
      case S1_IN_S2_OUT:  speed = SLOW;   break;  // only sensor 1 on black: slow forward
      case S1_OUT_S2_IN:  speed = -SLOW;  break;  // only sensor 2 on black: slow reverse
      case S1_OUT_S2_OUT: speed = 0;      break;  // both on white: stop
    }
  }

  motor.run(speed);

  Serial.print("Line: ");     Serial.print(line);
  Serial.print("  Dist: ");   Serial.print(distance);
  Serial.print(" cm  Speed: "); Serial.println(speed);

  delay(50);
}