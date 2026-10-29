// drive_port_test.ino
// Finds the back motors. On startup it drives every MegaPi slot one at a time,
// first as an encoder motor (SLOT1-4), then as plain DC (PORT1A-4B), at full power,
// and prints which one it is testing. Serial monitor: 9600 baud.
// Lift the robot so the wheels can spin freely, then watch which wheel moves.

#include <MeMegaPi.h>

const int RUN_SPEED = 255;
const unsigned long RUN_MS = 1000;
const unsigned long GAP_MS = 500;

const uint8_t slots[]     = { SLOT1, SLOT2, SLOT3, SLOT4 };
const char*   slotNames[] = { "SLOT1", "SLOT2", "SLOT3", "SLOT4" };

const uint8_t dcPorts[] = { PORT1A, PORT1B, PORT2A, PORT2B, PORT3A, PORT3B, PORT4A, PORT4B };
const char*   dcNames[] = { "PORT1A", "PORT1B", "PORT2A", "PORT2B", "PORT3A", "PORT3B", "PORT4A", "PORT4B" };

void say(const char* kind, const char* name, const char* what) {
    Serial.print(kind); Serial.print(" "); Serial.print(name); Serial.print(": "); Serial.println(what);
}

void testEncoder(uint8_t i) {
    MeEncoderOnBoard motor(slots[i]);
    say("ENCODER", slotNames[i], "forward");
    motor.setMotorPwm(RUN_SPEED);  delay(RUN_MS); motor.setMotorPwm(0); delay(GAP_MS);
    say("ENCODER", slotNames[i], "reverse");
    motor.setMotorPwm(-RUN_SPEED); delay(RUN_MS); motor.setMotorPwm(0); delay(GAP_MS);
}

void testDC(uint8_t i) {
    MeMegaPiDCMotor motor(dcPorts[i]);
    say("DC", dcNames[i], "forward");
    motor.run(RUN_SPEED);  delay(RUN_MS); motor.stop(); delay(GAP_MS);
    say("DC", dcNames[i], "reverse");
    motor.run(-RUN_SPEED); delay(RUN_MS); motor.stop(); delay(GAP_MS);
}

void setup() {
    Serial.begin(9600);
    delay(1500);
    Serial.println("=== motor port test (all slots) ===");
    Serial.println();

    for (uint8_t i = 0; i < 4; i++) testEncoder(i);
    for (uint8_t i = 0; i < 8; i++) testDC(i);

    Serial.println();
    Serial.println("=== done. Reset the board to run again. ===");
}

void loop() {}
