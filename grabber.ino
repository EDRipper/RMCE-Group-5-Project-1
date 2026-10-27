// https://support.makeblock.com/hc/en-us/articles/4412894483095-About-MegaPi
// interfaces / pinout ^


// setup / prerequisites
#include <MeMegaPi.h> //librabry for interfacing with megaPi (we dont wire directly to the arduinos)
// refer to https://github.com/Makeblock-official/Makeblock-Libraries/tree/master


// ports config
// 1-4 are motor ports and have an A & B Part
// sockets 5-8 are sensors (the ports on the shield)
const uint8_t grabberMotorPort = PORT3B;
const uint8_t ultrasonicPort = PORT_6;
const uint8_t lineFollowerPort = PORT_5;
const uint8_t frontMotorSlot = SLOT1; // encoder motor, needs moving IRL
const uint8_t backMotorSlot = SLOT2; // back motor left
const uint8_t backMotorLeftPort = PORT3A; // back motor left
const uint8_t backMotorRightPort = PORT3B; // back motor right


// objects for each device
MeUltrasonicSensor  ultrasonic(ultrasonicPort);  // ultrasonic distance sensor
MeMegaPiDCMotor     grabber(grabberMotorPort); // grabber motor
MeLineFollower      lineFinder(lineFollowerPort);  // line follower
MeEncoderOnBoard    steerMotor(frontMotorSlot);  // the steering motor out front, tracks its own position
MeMegaPiDCMotor     backMotorLeft(backMotorLeftPort); // back motor left
MeMegaPiDCMotor     backMotorRight(backMotorRightPort); // back motor right

const int STEER_ANGLE = 30; // degrees of motor shaft to turn when correcting (TODO: calibrate)
const int STEER_SPEED = 100; // rpm the steering motor moves at

// distance thresholds (cm) for the grabber logic
const double OPEN_DISTANCE = 20.0;  // closer than this -> open the claw, ready to grab
const double CLOSE_DISTANCE = 5.0;  // closer than this -> lined up, close the claw

// how often to print the distance (ms) so the serial monitor is not flooded
const unsigned long PRINT_INTERVAL = 200;
unsigned long lastPrint = 0;

steering encoder interrupt handler (disabled in this debug build)
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
        Serial.println(">>> GRABBER: OPENING (motor +255 for 1000ms)");
        grabberState.isOpen = true;
        grabber.run(255); // open the claw
        delay(1000); // open fully (TODO: calibrate this time)
        grabber.stop();
        Serial.println(">>> GRABBER: OPEN - motor stopped");
    }
}
void closeGrabber() {
    if (!grabberState.isOpen) {
        return; // already closed
    } else {
        Serial.println(">>> GRABBER: CLOSING (motor -255 for 1000ms)");
        grabberState.isOpen = false;
        grabber.run(-255); // close the claw
        delay(1000); // close fully (TODO: calibrate this time)
        grabber.stop(); // stop the motor
        Serial.println(">>> GRABBER: CLOSED - motor stopped");
    }
}


void setup() { // runs once at the beginning
    Serial.begin(9600); // start communication channel to arduino at 9600 bits per sec
    // on start the bot needs to back up until mechanically engaged, close the grabber, and then start line following.
    closeGrabber(); // start with the grabber closed
    delay(1000); // give the grabber some time to close fully
    backMotorLeft.run(-150); // run the back motor in reverse to back up
    backMotorRight.run(-150); // run the back motor in reverse to back up
    delay(1500); // this is more than enough time but it doesnt matter as it mechanically engages :)
    backMotorLeft.stop();
    backMotorRight.stop();
    openGrabber(); // open the grabber after backing up
    // note again that opening the grabber closes the latch
    // this may need flipping.

}


void loop() { // runs in forever loop
    // steerMotor.loop(); // lets the library move the steering motor towards its target angle
    // backMotorLeft.run(100); // run the back motor at a constant speed (adjust as needed)
    // backMotorRight.run(100); // run the back motor at a constant speed (adjust as needed)

    // readout distance, when the latch backs up, open the grabber, when it gets really close (lined up) close the grabber
    //double distance = ultrasonic.distanceCm();

    //Serial.print("distance: ");
    //Serial.print(distance);
    //Serial.print(" cm | grabber: ");
    //Serial.print(grabberState.isOpen ? "OPEN" : "CLOSED");
    //Serial.print(" | zone: ");
    //if (distance < CLOSE_DISTANCE) {
    //    Serial.println("CLOSE (<5cm) -> want CLOSED");
    //} else if (distance < OPEN_DISTANCE) {
    //    Serial.println("NEAR (<20cm) -> want OPEN");
    //} else {
    //    Serial.println("FAR (>=20cm) -> want CLOSED");
    //}

    // we could activate grabber based on USDS but mechanically is better - uncomment if you want?
}

    if (distance < OPEN_DISTANCE) {
        if (distance < CLOSE_DISTANCE) {
            closeGrabber();
        }
        else {
            openGrabber();
        }
    } else {
        closeGrabber();
    }
    // we have a couple of options to steer we can do moveTo() to set a position or run() to nudge continuously
    // nudging provides a faster feedback loop but can make the movement quite jittery
    // move to is smooth but slow  and oversteers the robot
    int line = lineFinder.readSensors();
    int speed = 100; // set the speed for the back motors (test at slow and increase)
    switch (line)
    {
    case 0b00: // both sensors on black

        steerMotor.run(0); // stop the steering motor
        backMotorLeft.run(speed); // run the back motor at a constant speed (adjust as needed)
        backMotorRight.run(speed); // run the back motor at a constant speed (adjust as needed)
        break;
    case 0b10: // left sensor sees white
        steerMotor.run(-STEER_SPEED); // turn right
        Serial.println("right bump");
        break;
    case 0b01: // right sensor sees white
        steerMotor.run(STEER_SPEED); // turn left
        Serial.println("left bump");
        break;
     default:
        // lost the line, hold the current angle
        break;
    }

}
