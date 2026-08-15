#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

char txBuf[64];
char rxBuf[64];
String inputBuffer = "";
String inputBuffer0 = "";
String inputBuffer1 = "";
const int MAX_BUFFER_SIZE = 256;

// Serial 1 to display pins
#define RX_pin 1
#define TX_pin 2

Adafruit_PWMServoDriver stepPwm = Adafruit_PWMServoDriver(0x40);

constexpr uint16_t PCA9685_FREQUENCY_HZ = 50;
constexpr uint8_t PCA9685_STEP_CHANNEL_BASE = 0;
/*constexpr uint8_t GRINDER_FORWARD_PWM_CHANNEL = 47;
constexpr uint8_t GRINDER_REVERSE_PWM_CHANNEL = 48;
constexpr uint8_t GRINDER_PWM_DUTY = 100;*/
const int RPWM = 47;
const int LPWM = 48;

const int CH_R = 0;
const int CH_L = 1;
// -------- Pump Pins --------

#define PERISTALTIC_PUMP 10
#define FLUID_PUMP 11
#define HOTWATER_PUMP 12

// -------- Stirrer Pin --------

#define STR_EN 7

// -------- Stepper Pins --------

#define NEMA_CNT 6

int DIR_PIN[NEMA_CNT] = {35, 36, 37, 38, 39, 40};
uint8_t STEP_CHANNEL[NEMA_CNT] = {
    PCA9685_STEP_CHANNEL_BASE + 0,
    PCA9685_STEP_CHANNEL_BASE + 1,
    PCA9685_STEP_CHANNEL_BASE + 2,
    PCA9685_STEP_CHANNEL_BASE + 3,
    PCA9685_STEP_CHANNEL_BASE + 5,
    PCA9685_STEP_CHANNEL_BASE + 6};

// MOTOR    ROLE       MOVEMENT_TYPE
// 0       Onion           Linear
// 1       Tomato          Linear
// 2       Cutter          Linear
// 3       Dispenser       Linear
// 4       Spice Selector  Rotational
// 5       Spice Actuator  Linear

long stepsToMove[NEMA_CNT] = {25000, 25000, 50000, 15000, 3200 / 12, 14500};
int stepDelay[NEMA_CNT] = {100, 100, 50, 50, 300, 50}; // microseconds
int interDelay = 100;                                  // milliseconds

// -------- DRV8825 Microstepper Pins --------

#define MICRO_CNT 5

int MICRO_DIR_PIN[MICRO_CNT] = {15, 16, 17, 18, 41};
uint8_t MICRO_STEP_CHANNEL[MICRO_CNT] = {
    PCA9685_STEP_CHANNEL_BASE + 11,
    PCA9685_STEP_CHANNEL_BASE + 7,
    PCA9685_STEP_CHANNEL_BASE + 8,
    PCA9685_STEP_CHANNEL_BASE + 9,
    PCA9685_STEP_CHANNEL_BASE + 10};

// MOTOR    ROLE
// 0       Toor Dal
// 1       Blancher
// 2       Spinach
// 3       Cashew
// 4       Spice Flip

long stepsToMoveMicro[MICRO_CNT] = {128, 85, 256, 256, 192};
int microStepDelay = 5;

// -------- BTS7960 Pins --------

#define DC_CNT 1

int DC_IN1[DC_CNT] = {47};
int DC_IN2[DC_CNT] = {48};

// MOTOR    ROLE
// 0       Grinder     Continuous rotation for grindDelay ms
// 1       

int grindDelay = 4000; // milliseconds
int extendDelay = 4500;

void pulseStepChannel(uint8_t channel)
{
    stepPwm.setPWM(channel, 0, 2048);
    delayMicroseconds(4);
    stepPwm.setPWM(channel, 0, 0);
}

void grinderStartForward()
{
  ledcWrite(CH_R, 0);
  ledcWrite(CH_L, 100);
  delay(3000);
  Serial.println("GRINDER");
}

void grinderStop()
{
  ledcWrite(CH_R, 0);
  ledcWrite(CH_L, 0);
}

void moveSteps(int motorIndex, bool directionCW, int sectorId = 1);
void microMoveSteps(int motorIndex, bool directionCW, int sectorId = 1);
void moveToorDal(bool directionCW, int sectorId = 1);
void moveBlancher(bool directionCW, int sectorId = 1);
void moveSpinach(bool directionCW, int sectorId = 1);
void moveCashew(bool directionCW, int sectorId = 1);
void moveSpiceFlip(bool directionCW, int sectorId = 1);

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_pin, TX_pin);
    Serial.println("ESP32-S3 UART Slave Started");

    Wire.begin(20, 21);
    Wire.setClock(100000);
    stepPwm.begin();
    stepPwm.setPWMFreq(PCA9685_FREQUENCY_HZ);

    delay(200);

    pinMode(PERISTALTIC_PUMP, OUTPUT);
    pinMode(FLUID_PUMP, OUTPUT);
    pinMode(HOTWATER_PUMP, OUTPUT);

    for (int i = 0; i < NEMA_CNT; i++)
    {
        pinMode(DIR_PIN[i], OUTPUT);
        digitalWrite(DIR_PIN[i], HIGH);
        stepPwm.setPWM(STEP_CHANNEL[i], 0, 0);
    }

    for (int i = 0; i < MICRO_CNT; i++)
    {
        pinMode(MICRO_DIR_PIN[i], OUTPUT);
        digitalWrite(MICRO_DIR_PIN[i], HIGH);
        stepPwm.setPWM(MICRO_STEP_CHANNEL[i], 0, 0);
    }

    for (int i = 0; i < DC_CNT; i++)
    {
        pinMode(DC_IN1[i], OUTPUT);
        pinMode(DC_IN2[i], OUTPUT);
    }
    
    ledcSetup(CH_R, 1000, 8);
    ledcSetup(CH_L, 1000, 8);

    ledcAttachPin(RPWM, CH_R);
    ledcAttachPin(LPWM, CH_L);    
    /*ledcSetup(GRINDER_FORWARD_PWM_CHANNEL, 20000, 8);
    ledcAttachPin(DC_IN1[0], GRINDER_FORWARD_PWM_CHANNEL);
    ledcSetup(GRINDER_REVERSE_PWM_CHANNEL, 20000, 8);
    ledcAttachPin(DC_IN2[0], GRINDER_REVERSE_PWM_CHANNEL);*/
    grinderStop();

    digitalWrite(PERISTALTIC_PUMP, LOW);
    digitalWrite(FLUID_PUMP, LOW);
    digitalWrite(HOTWATER_PUMP, LOW);

    pinMode(STR_EN, OUTPUT);
    Serial.println("Starting Stirrer");
    digitalWrite(STR_EN, HIGH);
}

// -------- NEMA Stepper Function --------

void moveSteps(int motorIndex, bool directionCW, int sectorId)
{
    if (directionCW)
    {
        digitalWrite(DIR_PIN[motorIndex], LOW);
        Serial.println("Stepper: CLOCKWISE");
    }
    else
    {
        digitalWrite(DIR_PIN[motorIndex], HIGH);
        Serial.println("Stepper: ANTI-CLOCKWISE");
    }

    stepPwm.setPWMFreq(1500);
    for (long i = 0; i < stepsToMove[motorIndex] * sectorId; i++)
    {
        pulseStepChannel(STEP_CHANNEL[motorIndex]);
        delayMicroseconds(stepDelay[motorIndex]);
    }
    
    stepPwm.setPWMFreq(50);
    Serial.println("Stepper movement complete.");
}

// -------- DRV8825 Stepper Functions --------

void microMoveSteps(int motorIndex, bool directionCW, int sectorId)
{
    Serial.print("Rotating micro stepper motor: ");

    if (directionCW)
        Serial.println(" CLOCKWISE");
    else
        Serial.println(" ANTI-CLOCKWISE");

    for (long i = 0; i < stepsToMoveMicro[motorIndex] * sectorId; i++)
    {
        pulseStepChannel(MICRO_STEP_CHANNEL[motorIndex]);
        delayMicroseconds(microStepDelay);
    }
    Serial.println("Micro stepper movement complete.");
}

void moveToorDal(bool directionCW, int sectorId)
{
    microMoveSteps(0, directionCW, sectorId);
}

void moveBlancher(bool directionCW, int sectorId)
{
    microMoveSteps(1, directionCW, sectorId);
}

void moveSpinach(bool directionCW, int sectorId)
{
    microMoveSteps(2, directionCW, sectorId);
}

void moveCashew(bool directionCW, int sectorId)
{
    microMoveSteps(3, directionCW, sectorId);
}

void moveSpiceFlip(bool directionCW, int sectorId)
{
    microMoveSteps(4, directionCW, sectorId);
}

//------------- Hopper Helper Functions -------------

void chop()
{
    Serial.println("Cutting Vegetable...");
    stepPwm.setPWMFreq(1500);
    moveSteps(2, true); // Engage
    stepPwm.setPWMFreq(PCA9685_FREQUENCY_HZ);
    Serial.println("Chopper at closed position.");
}

void releaseChopper()
{
    Serial.println("Releasing Chopper...");
    stepPwm.setPWMFreq(1500);
    moveSteps(2, false); // Engage - Anti-clockwise
    stepPwm.setPWMFreq(PCA9685_FREQUENCY_HZ);
    Serial.println("Chopper back to original position.");
}

void feeder(int sel, int cnt = 1)
{
    if (sel == 0)
        Serial.println("Feeding Onion to Chopper...");
    if (sel == 1)
        Serial.println("Feeding Tomato to Chopper...");
    stepPwm.setPWMFreq(1500);
    for (int i = 0; i < cnt - 1; i++)
    {   
        
        moveSteps(sel, false); // Engage - ACW
        delay(interDelay);
        moveSteps(sel, true); // Disengage
        chop();
        releaseChopper();
    }

    delay(interDelay);
    moveSteps(sel, false); // Engage - ACW
    delay(interDelay);
    moveSteps(sel, true); // Disengage
    chop();
    grind();
    dispense();
    releaseChopper();
    stepPwm.setPWMFreq(PCA9685_FREQUENCY_HZ);
    Serial.println("Operation Complete.");
}

void blancher(int sel = 0)
{
    // sel not used here for now (default to 0)
    if (sel == 0)
        Serial.println("Feeding Dahl to Blancher...");

    moveBlancher(true);
    delay(interDelay);
    moveSpinach(true);
    delay(interDelay);
    moveSpinach(false);
    feeder(2);
}

void grind()
{
    Serial.println("Grinding vegetables...");

    grinderStartForward();
    delay(grindDelay);
    grinderStop();

    Serial.println("Completed grinding.");
}

void dispense()
{
    Serial.println("Dispensing paste...");

    //moveSteps(3, false); // Engage - Anti-clockwise
    delay(interDelay);
    grind();
    //moveSteps(3, true); // Disengage

    Serial.println("Base back to original position.");
}

//---------- Spice Dispenser Functions ----------

void selSpice(int n, int m=1)
{
    Serial.println("Choosing the spice");

    moveSteps(4, true, n);
    for(int i=0; i<m; i++){
        pushRetract();
        flipContents();
    }
    moveSteps(4, false, n);

    Serial.println("Spice dispensed. Selecting motor back to original position.");
}

void pushRetract()
{
    Serial.println("Extending Actuator");

    moveSteps(5, false);
    delay(interDelay * 2);
    delay(interDelay * 2);
    moveSteps(5, true);

    Serial.println("Actuator back to original position.");
}

void flipContents()
{
    Serial.println("Dispensing spice onto cooktop...");

    moveSpiceFlip(true);
    delay(interDelay * 2);
    moveSpiceFlip(false);

    Serial.println("Dispensing motor back to original position");
}

// ---------- Command line ----------

void parseAndExecute(String command)
{
    if (command.startsWith("MODULE:"))
        command.remove(0, 7);

    command.trim();

    String parts[3];
    int seg = 0, start = 0;
    while (seg < 3)
    {
        int bar = command.indexOf('|', start);
        if (bar == -1)
        {
            parts[seg++] = command.substring(start);
            break;
        }
        parts[seg++] = command.substring(start, bar);
        start = bar + 1;
    }
    while (seg < 3)
        parts[seg++] = "";

    String name = parts[0];
    int id = parts[1].length() ? parts[1].toInt() : -1;
    int value = parts[2].length() ? parts[2].toInt() : -1;

    if (name == "liquid")
    {
        if (id == 0)
        {
            Serial.println("Peristaltic ON");
            digitalWrite(PERISTALTIC_PUMP, HIGH);
            delay(1000 * value);
            digitalWrite(PERISTALTIC_PUMP, LOW);
        }
        if (id == 1)
        {
            Serial.println("Fluid Pump ON");
            digitalWrite(FLUID_PUMP, HIGH);
            delay(1000 * value);
            digitalWrite(FLUID_PUMP, LOW);
        }
        if (id == 2)
        {
            Serial.println("Hot Water Pump ON");
            digitalWrite(HOTWATER_PUMP, HIGH);
            delay(1000 * value);
            digitalWrite(HOTWATER_PUMP, LOW);
        }

        Serial.println("Status: liquid Done");
        Serial.println("DONE");
    }
    else if (name == "hopper")
    {
        Serial.println("Hopper Module Triggered");
        if (id == 0 || id == 1)
            feeder(id, value);
        else if (id == 2)
        {
            moveToorDal(true, 2);
            delay(2000);
            Serial.println("Hot Water Pump ON");
            digitalWrite(HOTWATER_PUMP, HIGH);
            delay(4000);
            digitalWrite(HOTWATER_PUMP, LOW);
        }
        else if (id == 3)
            dispense();
        else if (id == 4)
            blancher(value > 0 ? value : 0);
        else if (id == 5)
        {
            blancher(value > 0 ? value : 0);
            Serial.println("Fluid Pump ON");
            digitalWrite(FLUID_PUMP, HIGH);
            delay(4000);
            digitalWrite(FLUID_PUMP, LOW);
        }
        else if (id == 6)
        {
            int turns = value > 0 ? value : 1;
            moveSpinach(true, turns);
        }
        else if (id == 7)
        {
            int turns = value > 0 ? value : 1;
            moveCashew(true, turns);
        }

        Serial.println("Status: hopper Done");
        Serial.println("DONE");
    }
    else if (name == "spice")
    {
        selSpice(id);
        flipContents();
        Serial.println("DONE");
    }
    else
    {
        Serial.println("Status: " + name + " Done");
        Serial1.println("DONE");
        Serial.println("DONE");
    }
}

void readSerialInput(HardwareSerial &serialPort, String &buffer)
{
    while (serialPort.available() > 0)
    {
        char c = serialPort.read();
        if (c == '\n' || c == '\r')
        {
            if (buffer.length() > 0)
            {
                buffer.trim();
                if (buffer.length() > 0)
                {
                    Serial.println("Received: " + buffer);

                    if (buffer.startsWith("MODULE:"))
                    {
                        parseAndExecute(buffer);
                    }
                    else
                    {
                        Serial.println("Invalid command format: " + buffer);
                    }
                }
                buffer = "";
            }
        }
        else
        {
            if (buffer.length() < MAX_BUFFER_SIZE)
            {
                buffer += c;
            }
            else
            {
                Serial.println("Buffer overflow! Clearing buffer.");
                buffer = "";
            }
        }
    }
}

void loop()
{
    readSerialInput(Serial, inputBuffer0);
    readSerialInput(Serial1, inputBuffer1);
}
