// https://support.makeblock.com/hc/en-us/articles/4412894483095-About-MegaPi
// interfaces / pinout ^

// setup / prerequisites
#include <MeMegaPi.h> //librabry for interfacing with megaPi (we dont wire directly to the arduinos)
// refer to https://github.com/Makeblock-official/Makeblock-Libraries/tree/master


// ports config
// 1-4 are motor ports and have an A & B Part
// sockets 5-8 are sensors (the ports on the shield)
const uint8_t grabberMotorPort = PORT3B; 
const uint8_t lineFollowerPort = PORT_5;
const uint8_t frontMotorSlot = SLOT1; // encoder motor, needs moving IRL
const uint8_t ultrasonicPort = PORT_6;

// objects for each device
MeLineFollower      lineFinder(lineFollowerPort);  // line follower
MeUltrasonicSensor  ultrasonic(ultrasonicPort);  // ultrasonic distance sensor
MeEncoderOnBoard    steerMotor(frontMotorSlot);  // the steering motor out front, tracks its own position
MeMegaPiDCMotor     grabber(grabberMotorPort); // grabber motor

const int STEER_ANGLE = 30; // degrees of motor shaft to turn when correcting (TODO: calibrate)
const int STEER_SPEED = 100; // rpm the steering motor moves at

// 
void steerEncoderISR() {
    if (digitalRead(steerMotor.getPortB()) == 0) {
        steerMotor.pulsePosMinus();
    } else {
        steerMotor.pulsePosPlus();
    }
}

struct grabber
// multiple functions affect grabber state, so we can group state vars together in a struct
// and then each function can mutate and read the same data.
{
    bool isOpen = false;
};


struct grabber grabberState; // create an instance of the grabber struct to track the current grabber state

// grabber motor is in range -255 to 255
// 255 opens the claw, but therefore closes the latch
// for consistency we will always speak in terms of the claw not latch    
   
void openGrabber() {
    if (grabberState.isOpen) {
        return; // already open
    } else {
        grabberState.isOpen = true;
        grabber.run(255); // open the claw
        delay(1000); // open fully (TODO: calibrate this time)
        grabber.stop();
    }
}
void closeGrabber() {
    if (!grabberState.isOpen) {
        return; // already closed
    } else {
        grabberState.isOpen = false;
        grabber.run(-255); // close the claw
        delay(1000); // close fully (TODO: calibrate this time)
        grabber.stop(); // stop the motor
    }
}


void setup() { // runs once at the beginning
    Serial.begin(9600); // start communication channel to arduino at 9600 bits per sec

    // steering encoder setup, values from makeblock's MegaPi encoder examples
    attachInterrupt(steerMotor.getIntNum(), steerEncoderISR, RISING);
    TCCR1A = _BV(WGM10); // set motor PWM to 8KHz, as makeblock's examples do
    TCCR1B = _BV(CS11) | _BV(WGM12);
    TCCR2A = _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS21);
    steerMotor.setPulse(8); // encoder pulses per motor revolution
    steerMotor.setRatio(46.67); // gearbox ratio (check the label on the motor)
    steerMotor.setPosPid(1.8, 0, 1.2);
    steerMotor.setSpeedPid(0.18, 0, 0);
    steerMotor.setPulsePos(0); // wheels must be straight at power on, this becomes angle 0

    grabber.run(255); // range is -255 to 255
    delay(1000); // wait 1 second (arduino has no wait(), it's delay())
    grabber.run(-255); // reverse the motor
    delay(1000); // wait 1 second
    grabber.stop(); // stop the motor (same as run(0))
}


void loop() { // runs in forever loop
    steerMotor.loop(); // lets the library move the steering motor towards its target angle

    // readout distance, when the latch backs up, open the grabber, when it gets really close (lined up) close the grabber
    double distance = ultrasonic.distanceCm();
    if (distance < 20) {
        if (distance < 5) {
            closeGrabber();
            Serial.println("Closing grabber");
        }
        else {
            openGrabber();
        }
    } else {
        closeGrabber();
    }
    // also steer based on line follower
    // moveTo() sets a target angle, the library's PID eases the motor there (no jitter handling needed)
    int line = lineFinder.readSensors();
    switch (line)
    {
    case 0b11: // both sensors see line
        steerMotor.moveTo(0, STEER_SPEED); // revert angle to straight
        break;
    case 0b10: // left sensor sees line
        steerMotor.moveTo(STEER_ANGLE, STEER_SPEED); // turn right (flip sign if it turns the wrong way)
        break;
    case 0b01: // right sensor sees line
        steerMotor.moveTo(-STEER_ANGLE, STEER_SPEED); // turn left
        break;
    default:
        // lost the line, hold the current angle
        break;
    }

}
