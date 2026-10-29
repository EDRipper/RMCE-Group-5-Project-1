// grabber.ino
// MegaPi robot control. Serial monitor: 9600 baud.
// https://support.makeblock.com/hc/en-us/articles/4412894483095-About-MegaPi

#include <MeMegaPi.h>

// ports (motors verified with port tests)
const uint8_t GRABBER_PORT    = PORT3A;
const uint8_t BACK_LEFT_PORT  = PORT2B;
const uint8_t BACK_RIGHT_PORT = PORT1B;
const uint8_t STEER_PORT      = PORT3B;
const uint8_t ULTRASONIC_PORT = PORT_5;
const uint8_t LINE_PORT       = PORT_6;

// grabber: positive speed closes the claw, negative opens it (verified with port test)
const int CLOSE_SPEED = 255;
const int OPEN_SPEED  = -255;
const unsigned long GRABBER_MOVE_MS = 1300; // how long the motor runs for a full open/close

// drive: motors are mirrored, so the right one runs with the opposite sign
// if the robot goes the wrong way, swap the sign of RIGHT_DIR
const int LEFT_DIR  = 1;
const int RIGHT_DIR = -1;
const int SLOW_SPEED  = 60;                // -255..255, used for the startup reverse
const int DRIVE_SPEED = 150;               // -255..255, forward speed in the main loop
const unsigned long REVERSE_MS = 3500;     // reverse time on startup
const unsigned long AFTER_OPEN_MS = 2000;  // pause after opening the grabber

// obstacle avoidance
const double STOP_DISTANCE_CM = 15.0;      // stop driving when something is closer than this

// line following / steering
// the two eyes straddle the line. Each reads HIGH on white and LOW on black.
// both white = centred, do nothing. One eye LOW = that side has drifted onto the line, nudge that way.
// sensor1 is assumed to be the LEFT eye; set LINE_SWAP to true if it steers the wrong way
const bool LINE_SWAP = false;
const int  STEER_LEFT_DIR = 1;             // sign that turns the steering motor left; flip if reversed
const int  STEER_SPEED = 150;              // -255..255
const unsigned long NUDGE_MS = 25;         // how long one nudge runs the steering motor
const unsigned long NUDGE_INTERVAL_MS = 180; // minimum time between nudges

MeMegaPiDCMotor grabber(GRABBER_PORT);
MeMegaPiDCMotor backLeft(BACK_LEFT_PORT);
MeMegaPiDCMotor backRight(BACK_RIGHT_PORT);
MeMegaPiDCMotor steer(STEER_PORT);
MeUltrasonicSensor ultrasonic(ULTRASONIC_PORT);
MeLineFollower lineFinder(LINE_PORT);

bool grabberOpen = false;
bool driving = false;
unsigned long lastNudge = 0;

// --- grabber ---

void closeGrabber() {
    Serial.println("grabber: closing");
    grabber.run(CLOSE_SPEED);
    delay(GRABBER_MOVE_MS);
    grabber.stop();
    grabberOpen = false;
    Serial.println("grabber: closed");
}

void openGrabber() {
    Serial.println("grabber: opening");
    grabber.run(OPEN_SPEED);
    delay(GRABBER_MOVE_MS);
    grabber.stop();
    grabberOpen = true;
    Serial.println("grabber: open");
}

// --- drive ---

// positive = forward, negative = reverse
void drive(int speed) {
    backLeft.run(speed * LEFT_DIR);
    backRight.run(speed * RIGHT_DIR);
}

void stopDrive() {
    backLeft.stop();
    backRight.stop();
}

// --- steering ---

// dir: -1 = left, +1 = right. Runs the steering motor for one short burst.
void nudge(int dir, const char* reason) {
    if (millis() - lastNudge < NUDGE_INTERVAL_MS) return;

    steer.run(-dir * STEER_LEFT_DIR * STEER_SPEED); // -dir because left is -1
    delay(NUDGE_MS);
    steer.stop();

    lastNudge = millis();

    Serial.print("steer: nudge ");
    Serial.print(dir < 0 ? "left" : "right");
    Serial.print(" (");
    Serial.print(reason);
    Serial.println(")");
}

void followLine() {
    uint8_t state = lineFinder.readSensors();
    bool leftBlack  = !(state & 0x02); // sensor1 bit, LOW = black
    bool rightBlack = !(state & 0x01); // sensor2 bit
    if (LINE_SWAP) { bool t = leftBlack; leftBlack = rightBlack; rightBlack = t; }

    if (leftBlack && !rightBlack) {
        nudge(-1, "left eye on line");
    } else if (rightBlack && !leftBlack) {
        nudge(+1, "right eye on line");
    }
    // both white (centred) or both black (crossing a wide mark): leave the steering alone
}

// --- main ---

void setup() {
    Serial.begin(9600);
    delay(1500); // give the serial monitor time to attach
    Serial.println("=== robot ===");

    closeGrabber();

    Serial.println("drive: reversing");
    drive(-SLOW_SPEED);
    delay(REVERSE_MS);
    stopDrive();
    Serial.println("drive: stopped");

    openGrabber();
    delay(AFTER_OPEN_MS);

    Serial.println("drive: forward (main loop)");
}

void loop() {
    double distance = ultrasonic.distanceCm();
    bool obstacle = distance < STOP_DISTANCE_CM;

    if (obstacle && driving) {
        stopDrive();
        driving = false;
        Serial.println("drive: obstacle, stopped");
    } else if (!obstacle && !driving) {
        drive(DRIVE_SPEED);
        driving = true;
        Serial.println("drive: clear, forward");
    }

    if (driving) {
        followLine();
    }
}
