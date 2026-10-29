// https://support.makeblock.com/hc/en-us/articles/4412894483095-About-MegaPi
// interfaces / pinout ^

// setup / prerequisites
#include <MeMegaPi.h> //librabry for interfacing with megaPi (we dont wire directly to the arduinos)
// refer to https://github.com/Makeblock-official/Makeblock-Libraries/tree/master


// ports config
// 1-4 are motor ports and have an A & B part (the hats with the ribbon cables)
// sockets 5-8 are sensors (the ports on the shield)
const uint8_t GRABBER_PORT    = PORT3A;
const uint8_t BACK_LEFT_PORT  = PORT2B;
const uint8_t BACK_RIGHT_PORT = PORT1B;
const uint8_t STEER_PORT      = PORT3B; // shares the hat with the grabber
const uint8_t ULTRASONIC_PORT = PORT_5;
const uint8_t LINE_PORT       = PORT_6;

// grabber motor is in range -255 to 255
// + closes the claw, - opens it
const int CLOSE_SPEED = 255;
const int OPEN_SPEED  = -255;
const unsigned long GRABBER_MOVE_MS = 1300; // how long to run the motor for a full open/close, 1000 didnt quite close it

// driving
// the back motors face opposite ways so one of them has to spin backwards to go forwards
// if the whole robot drives the wrong way flip RIGHT_DIR
const int LEFT_DIR  = 1;
const int RIGHT_DIR = -1;
const int SLOW_SPEED  = 60;                // -255 to 255, used when reversing at the start
const int DRIVE_SPEED = 150;               // -255 to 255, forwards speed in the main loop
const unsigned long REVERSE_MS = 3500;     // how long to reverse for at the start
const unsigned long AFTER_OPEN_MS = 2000;  // wait after opening the grabber before driving off

// ultrasonic
const double STOP_DISTANCE_CM = 15.0;      // stop if something is closer than this

// line following / steering
// the two eyes on the line sensor sit either side of the black line
// HIGH = seeing white, LOW = seeing black
// both white means we're centred so do nothing, one eye going LOW means that side has drifted onto the line so nudge that way
// sensor1 is assumed to be the left eye, if it steers away from the line set LINE_SWAP to true
const bool LINE_SWAP = false;
const int  STEER_LEFT_DIR = 1;             // which way the steering motor spins for left, flip if its backwards
const int  STEER_SPEED = 150;              // -255 to 255
const unsigned long NUDGE_MS = 25;         // how long one bump of the steering motor lasts (TODO: calibrate, 40 was too much)
const unsigned long NUDGE_INTERVAL_MS = 180; // min gap between bumps so it doesnt tweak out

// objects for each device
MeMegaPiDCMotor grabber(GRABBER_PORT);     // grabber motor
MeMegaPiDCMotor backLeft(BACK_LEFT_PORT);  // back wheels
MeMegaPiDCMotor backRight(BACK_RIGHT_PORT);
MeMegaPiDCMotor steer(STEER_PORT);         // steering motor out front
MeUltrasonicSensor ultrasonic(ULTRASONIC_PORT); // ultrasonic distance sensor
MeLineFollower lineFinder(LINE_PORT);      // line follower

// state
bool grabberOpen = false;
bool driving = false;
unsigned long lastNudge = 0;

// --- grabber ---

void closeGrabber() {
    Serial.println("grabber: closing");
    grabber.run(CLOSE_SPEED);
    delay(GRABBER_MOVE_MS); // arduino has no wait(), it's delay()
    grabber.stop(); // same as run(0)
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

// + is forwards, - is reverse, the DIR constants sort out the mirrored motor
void drive(int speed) {
    backLeft.run(speed * LEFT_DIR);
    backRight.run(speed * RIGHT_DIR);
}

void stopDrive() {
    backLeft.stop();
    backRight.stop();
}

// --- steering ---

// dir is -1 for left or +1 for right
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
    bool leftBlack  = !(state & 0x02);
    bool rightBlack = !(state & 0x01);
    if (LINE_SWAP) { bool t = leftBlack; leftBlack = rightBlack; rightBlack = t; }

    if (leftBlack && !rightBlack) {
        nudge(-1, "left eye on line");
    } else if (rightBlack && !leftBlack) {
        nudge(+1, "right eye on line");
    }
}

// --- main ---

void setup() { // runs once at the beginning
    Serial.begin(9600); // start communication channel to arduino at 9600 bits per sec
    delay(1500);
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

void loop() { // runs in forever loop
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
