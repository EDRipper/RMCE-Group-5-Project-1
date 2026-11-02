# AI Usage

**Student statement:** I acknowledge the use of Claude (Claude Code, https://claude.ai/code) and VS Code inline autocomplete to ask clarifying questions with code examples, fix a struct naming bug, re-indent `grabber.ino`, generate a throwaway motor port test sketch (`drive_port_test/drive_port_test.ino`) and produce this document. The prompts used were:

1. "do we need this line [`struct grabber grabberState;`] or can we just do [`grabber.isOpen = state`]"
2. "take this code and make it tab (4 space) indented, not 2 space indented"
3. "return just pastebale grabber test code to on initialisation cycle the grabber with output, 1 by 1 testing each port"
4. "it printed raw bytes i need output i can read"
5. "neither motor moved only the grabber moved"
6. "still nothing? they are plugged into the funny little hat with ribbon cables and the motor says encoder dc on it"
7. "read the above conversation history and produce a markdown file documenting each prompt and response in AI Usage.md"

The output from these prompts was used to understand the difference between the struct type and its instance and fix the resulting compile error, to reformat code I had already written, to find out which MegaPi port each motor was plugged into (the test sketch is diagnostic only and none of it runs on the robot), and to write up this record. The full prompts and responses are below.

I use AI while coding. I write in VScode, with autocomplete enabled.
I also use claude to ask clarifying questions with code examples.
I do not paste in code I do not understand, and I document the prompts and responses here.
note this file itself is made by claude, I asked it to read the above conversation history and produce a markdown file documenting each prompt and response in AI Usage.md

## Grabber state struct

### Prompt

```cpp
struct grabber grabberState;
```

> do we need this line or can we just do

```cpp
grabber.isOpen = state
```

### Response

I need to see the full context of the code to answer this properly. Let me check the file.

You've spotted a bug! There's a **namespace collision** here:

- Line 21: `MeMegaPiDCMotor grabber(grabberMotorPort);` — the motor object
- Line 35: `struct grabber grabberState;` — the state struct instance

Then in your functions (lines 42, 45, 52, 55), you're trying to access `grabber.isOpen`, but `grabber` refers to the **motor object**, not the struct instance. This won't compile.

**You need to either:**

1. **Use `grabberState.isOpen` consistently** (since that's the struct instance you declared):

   ```cpp
   if (grabberState.isOpen) { // ✓ correct
       grabberState.isOpen = true;
   }
   ```

2. **Or rename the motor to avoid the collision:**

   ```cpp
   MeMegaPiDCMotor grabberMotor(grabberMotorPort);
   // then use grabber.isOpen in your functions
   ```

I'd recommend option 1 (use `grabberState.isOpen`) since you already have the struct declared. You just need to update the function bodies to use `grabberState` instead of `grabber` when accessing the state boolean.

## Re-indenting grabber.ino

### Prompt

> take this code and make it tab (4 space) indented, not 2 space indented

(with `grabber.ino` open in the editor)

### Response

Done. `grabber.ino` now uses 4-space indentation throughout. Every indented line was converted by doubling its leading 2-space runs, so nesting levels are preserved (one level is 4 spaces, two levels is 8, three is 12). Only indentation changed. Code, comments, and blank lines are untouched.

One thing I noticed while in there, unrelated to indentation: the `closeGrabber` function (lines 46-54) is missing a closing brace for its else block, so the file will not compile as is. I left it alone since you only asked for re-indenting.

## Motor port test sketch (drive_port_test.ino)

**Declaration:** `drive_port_test/drive_port_test.ino` is entirely AI generated. It is a throwaway diagnostic I used to work out which MegaPi port each motor was plugged into, it is not part of the robot's code and nothing from it runs on the robot. The findings from running it (grabber on PORT3B at the time, back motors on PORT1B and PORT2B as plain DC) are what went into `grabber.ino`.

### Prompt

> return just pastebale grabber test code to on initialisation cycle the grabber with output, 1 by 1 testing each port

### Response

Standalone sketch. It cycles through every MegaPi DC motor port one at a time on boot, running each forward then backward with serial output, so you can see which port the grabber is actually on.

(sketch given at 115200 baud, same as the one below but with `Serial.begin(115200)`)

Set the serial monitor to 115200. If the claw only twitches on PORT3B, that matches the current grabber.ino. Remember PORT3B is also assigned to backMotorRight in that file, so the two will fight once you wire the drive motors.

### Prompt

> it printed raw bytes i need output i can read

### Response

Raw bytes means a baud rate mismatch. My sketch used 115200 but your serial monitor is on 9600, same as your other sketches. Here is the same test at 9600 so it matches what you already have open.

```cpp
// grabber_port_test.ino
// On startup, drives each MegaPi DC motor port one by one (open, pause, close, pause)
// and prints which port is being tested. Watch the claw and note the port that moves it.
// Serial monitor: 9600 baud.

#include <MeMegaPi.h>

const int RUN_SPEED = 255;          // -255..255
const unsigned long RUN_MS  = 1000; // how long to run each direction
const unsigned long GAP_MS  = 500;  // pause between steps

const uint8_t ports[] = { PORT1A, PORT1B, PORT2A, PORT2B, PORT3A, PORT3B, PORT4A, PORT4B };
const char*   names[] = { "PORT1A", "PORT1B", "PORT2A", "PORT2B", "PORT3A", "PORT3B", "PORT4A", "PORT4B" };
const uint8_t PORT_COUNT = sizeof(ports) / sizeof(ports[0]);

void testPort(uint8_t index) {
    MeMegaPiDCMotor motor(ports[index]);

    Serial.print("[");
    Serial.print(index + 1);
    Serial.print("/");
    Serial.print(PORT_COUNT);
    Serial.print("] ");
    Serial.print(names[index]);
    Serial.println(": forward (open)");
    motor.run(RUN_SPEED);
    delay(RUN_MS);
    motor.stop();
    delay(GAP_MS);

    Serial.print("      ");
    Serial.print(names[index]);
    Serial.println(": reverse (close)");
    motor.run(-RUN_SPEED);
    delay(RUN_MS);
    motor.stop();
    Serial.print("      ");
    Serial.print(names[index]);
    Serial.println(": done");
    delay(GAP_MS);
}

void setup() {
    Serial.begin(9600);
    delay(1500); // give the serial monitor time to attach
    Serial.println("=== MegaPi DC motor port test ===");
    Serial.println("Each port runs forward 1s, then reverse 1s. Note which one moves the grabber.");
    Serial.println();

    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        testPort(i);
    }

    Serial.println();
    Serial.println("=== All ports tested. Reset the board to run again. ===");
}

void loop() {
    // nothing, test runs once on startup
}
```

If it still looks garbled at 9600, check the baud dropdown in the bottom right of the Arduino serial monitor, since it keeps its own setting independent of the sketch.

### Prompt

> neither motor moved only the grabber moved

(after the back motors had been wired to ports 1 and 2 and `grabber.ino` was driving them as `MeMegaPiDCMotor` on `PORT1A` / `PORT2A`)

### Response

Nothing moving on both usually means the wrong sub-port or the wrong motor type. Slots 1 and 2 can be plain DC outputs (A/B) or encoder motor slots, and the original port test ran before the back motors were plugged in. I'll make a standalone test that tries all of them one by one.

Created `drive_port_test/drive_port_test.ino`. It is in its own folder so the Arduino IDE opens it as a separate sketch.

Upload it and watch the serial monitor. It drives each candidate for one second in each direction, in this order:

- DC PORT1A, PORT1B, PORT2A, PORT2B
- Encoder SLOT1, SLOT2

Note the label that was printing when each wheel moved, and whether "forward" actually went forward. Then tell me the two labels and I will put them into grabber.ino.

If the motors moved on the DC lines, it was just A versus B and the fix is one line. If they only moved on the encoder lines, the back motors are encoder motors and grabber.ino needs to drive them with MeEncoderOnBoard instead of MeMegaPiDCMotor, which I will change for you.

### Prompt

> still nothing? they are plugged into the funny little hat with ribbon cables and the motor says encoder dc on it

### Response

Let me check the actual library source on your machine, since the encoder driver has a few quirks.

The pin tables explain a lot. Slot 1's encoder driver uses exactly the same three pins as PORT1A, and slot 2's uses the same pins as PORT2A. So the DC test and the encoder test were driving the same wires, and if neither moved a wheel, the code is reaching pins nothing is connected to. I'll widen the test to all four slots at full power so we can find where the motors actually are.

I updated `drive_port_test/drive_port_test.ino` to try every slot, encoder mode first and then DC, at full power. Upload it with the robot lifted and tell me which labels made a wheel move. Do that first, since the earlier tests were only run at speed 120 and the robot may just not have had enough torque on the ground.

If still nothing moves on any label, the cause is physical rather than code. Things to check on the board:

- **Each back motor needs its own driver hat.** The MegaPi has four slots. The grabber works, so slot 3 has a hat. Slots 1 and 2 need hats too. If the ribbon cables go into a hat that sits in slot 3 or 4, the labels you see in the test will be SLOT3 or SLOT4, and that is fine, I just need to know.
- **Ribbon goes in the 6-pin encoder socket**, not the two-pin DC pads on the hat.
- **The hat is pushed all the way down** on the slot header. A half-seated hat can leave the PWM pin disconnected while the LED still lights.
- **Motor power switch is on** and the battery is charged. The grabber moving shows this is probably fine.

Once you tell me which SLOT labels moved the left and right wheels, and whether "forward" went forward, I will put them straight into grabber.ino.

The file as written by that response (and as committed):

```cpp
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
```

I ran it and replied "1B and 2B DC", which is what went into `grabber.ino`.
