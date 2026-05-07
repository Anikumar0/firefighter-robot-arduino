// Fire-fighting robot (Arduino Uno)
// Power: Arduino via USB (PC), Pump via external adapter
// Connections:
// Flame sensor (analog) -> A0
// Pan servo -> D5
// Tilt servo -> D6
// Relay (pump) IN -> D8
// Button (manual spray, optional) -> D4 (INPUT_PULLUP)
// LED (status, optional) -> D13

#include <Servo.h>

Servo panServo;
Servo tiltServo;

const int FLAME_A = A0;
const int PAN_PIN = 11;
const int TILT_PIN = 6;
const int RELAY_PIN = 8;     // Relay IN pin (active LOW)
const int BUTTON_PIN = 4;
const int LED_PIN = 13;

const int PAN_MIN = 30;    
const int PAN_MAX = 150;   
const int PAN_STEP = 8;    
const int TILT_ANGLE = 70; 
const int TILT_HOLD = 80;  
const int NUM_READS_PER_ANGLE = 3;
const int READ_DELAY_MS = 40;

const int PUMP_RUN_MS = 6000;      
const int CONFIRM_THRESHOLD_DIFF = 80;
int flameThreshold = 300; // Calibrate after testing

void setup() {
  Serial.begin(9600);
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Relay is active LOW, so keep it OFF initially
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);

  // Initial servo positions
  panServo.write((PAN_MIN + PAN_MAX) / 2);
  tiltServo.write(TILT_HOLD);

  Serial.println("Auto Fire Fighter (Uno) starting...");
}

int readFlameAverage() {
  long sum = 0;
  for (int i = 0; i < NUM_READS_PER_ANGLE; i++) {
    sum += analogRead(FLAME_A);
    delay(READ_DELAY_MS);
  }
  return sum / NUM_READS_PER_ANGLE;
}

bool isButtonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;
}

void sprayPump(unsigned long duration) {
  Serial.println("Pump ON");
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(RELAY_PIN, LOW); // active LOW relay → ON
  unsigned long start = millis();

  while (millis() - start < duration) {
    delay(50);
  }

  digitalWrite(RELAY_PIN, HIGH); // OFF
  digitalWrite(LED_PIN, LOW);
  Serial.println("Pump OFF");
}

void loop() {
  // Manual spray button
  if (isButtonPressed()) {
    Serial.println("Manual spray button pressed.");
    sprayPump(3000);
    delay(500);
  }

  int bestAngle = (PAN_MIN + PAN_MAX) / 2;
  int bestVal = 0;

  // Sweep left to right
  for (int ang = PAN_MIN; ang <= PAN_MAX; ang += PAN_STEP) {
    panServo.write(ang);
    delay(150);
    int val = readFlameAverage();
    Serial.print("Angle "); Serial.print(ang);
    Serial.print(" -> "); Serial.println(val);

    if (val > bestVal) {
      bestVal = val;
      bestAngle = ang;
    }
  }

  Serial.print("Best angle: "); Serial.print(bestAngle);
  Serial.print(" | Value: "); Serial.println(bestVal);

  panServo.write((PAN_MIN + PAN_MAX) / 2);
  delay(150);
  int ambient = readFlameAverage();
  Serial.print("Ambient: "); Serial.println(ambient);

  if (bestVal < ambient + CONFIRM_THRESHOLD_DIFF || bestVal < flameThreshold) {
    Serial.println("No flame detected. Waiting...");
    delay(800);
    return;
  }

  // Aim
  panServo.write(bestAngle);
  delay(300);
  tiltServo.write(TILT_ANGLE);
  delay(300);

  int confirmVal = readFlameAverage();
  Serial.print("Confirm flame: "); Serial.println(confirmVal);

  if (confirmVal < flameThreshold) {
    Serial.println("Flame gone. Re-scanning...");
    tiltServo.write(TILT_HOLD);
    delay(300);
    return;
  }

  // Spray water
  sprayPump(PUMP_RUN_MS);

  // Return to idle
  tiltServo.write(TILT_HOLD);
  delay(500);
}