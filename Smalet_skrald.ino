const int switchCPin = 2;
const int switchVPin = 3;

// -------- DC Motor C --------
const int driveC_pwm = 5;
const int driveC_dir = 7;

// -------- DC Motor V --------
const int driveV1 = 6;
const int driveV2 = 4;

int speedValue = 180;

// -------- COLOR SENSOR --------
#define s0 8 
#define s1 9
#define s2 10
#define s3 11
#define out 12

// calibration references
int redRef[3];
int greenRef[3];
int blueRef[3];
int metalRef[3];

// switch states
int stateC = HIGH, lastStateC = HIGH;
int stateV = HIGH, lastStateV = HIGH;

unsigned long lastDebounceTimeC = 0;
unsigned long lastDebounceTimeV = 0;

const unsigned long debounceDelay = 50;

int countC = 0;
int countV = 0;
int step = 0;
int activeColor = 0;
bool C_done = false;
bool V_done = false;

// -------- COLOR FUNCTIONS --------

int readColor(int S2, int S3) {
  digitalWrite(s2, S2);
  digitalWrite(s3, S3);
  delay(40);

  long sum = 0;
  for (int i = 0; i < 5; i++) {
    int v = pulseIn(out, LOW, 30000);
    if (v == 0) v = 30000;
    sum += v;
  }
  return sum / 5;
}

void readRGB(int &r, int &g, int &b) {
  r = readColor(LOW, LOW);
  b = readColor(LOW, HIGH);
  g = readColor(HIGH, LOW);
}

int dist(int r1,int g1,int b1,int r2,int g2,int b2){
  return abs(r1-r2)+abs(g1-g2)+abs(b1-b2);
}

void calibrateColor(int ref[3]) {
  long r=0,g=0,b=0;
  for(int i=0;i<5;i++){
    int tr,tg,tb;
    readRGB(tr,tg,tb);
    r+=tr; g+=tg; b+=tb;
    delay(200);
  }
  ref[0]=r/5;
  ref[1]=g/5;
  ref[2]=b/5;
}

// -------- MOTOR FUNCTIONS --------

void motorC_forward() {
  analogWrite(driveC_pwm, speedValue);
  digitalWrite(driveC_dir, LOW);
}

void motorC_backward() {
  analogWrite(driveC_pwm, speedValue);
  digitalWrite(driveC_dir, HIGH);
}

void motorC_stop() {
  analogWrite(driveC_pwm, 0);
}

void motorV_forward() {
  analogWrite(driveV1, speedValue);
  analogWrite(driveV2, 0);
}

void motorV_backward() {
  analogWrite(driveV1, 0);
  analogWrite(driveV2, speedValue);
}

void motorV_stop() {
  analogWrite(driveV1, 0);
  analogWrite(driveV2, 0);
}

// -------- SETUP --------

void setup() {

  Serial.begin(9600);

  pinMode(switchCPin, INPUT_PULLUP);
  pinMode(switchVPin, INPUT_PULLUP);

  pinMode(driveC_pwm, OUTPUT);
  pinMode(driveC_dir, OUTPUT);
  pinMode(driveV1, OUTPUT);
  pinMode(driveV2, OUTPUT);

  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(out, INPUT);

  digitalWrite(s0, HIGH);
  digitalWrite(s1, HIGH);

  // ---- CALIBRATION ----
  Serial.println("Show RED");
  delay(4000);
  calibrateColor(redRef);

  Serial.println("Show GREEN");
  delay(4000);
  calibrateColor(greenRef);

  Serial.println("Show BLUE");
  delay(4000);
  calibrateColor(blueRef);

  Serial.println("Show METAL / EMPTY");
  delay(4000);
  calibrateColor(metalRef);

  Serial.println("Calibration done");
}

// -------- LOOP --------

void loop() {

  // -------- SWITCH C --------
  int readingC = digitalRead(switchCPin);
  if (readingC != lastStateC) lastDebounceTimeC = millis();

  if ((millis() - lastDebounceTimeC) > debounceDelay) {
    if (readingC != stateC) {
      stateC = readingC;
      if (stateC == LOW) countC++;

      Serial.print("Switch C CLICK | Count: ");
      Serial.println(countC);
    }
  }
  lastStateC = readingC;

  // -------- SWITCH V --------
  int readingV = digitalRead(switchVPin);
  if (readingV != lastStateV) lastDebounceTimeV = millis();

  if ((millis() - lastDebounceTimeV) > debounceDelay) {
    if (readingV != stateV) {
      stateV = readingV;
      if (stateV == LOW) countV++;

      Serial.print("Switch V CLICK | Count: ");
      Serial.println(countV);
    }
  }
  lastStateV = readingV;

  if (countC > 96) countC = 0;
  if (countV > 96) countV = 0;

  // -------- COLOR DETECTION --------
  if (activeColor == 0) {

    int r,g,b;
    readRGB(r,g,b);

    int dRed   = dist(r,g,b, redRef[0],redRef[1],redRef[2]);
    int dGreen = dist(r,g,b, greenRef[0],greenRef[1],greenRef[2]);
    int dBlue  = dist(r,g,b, blueRef[0],blueRef[1],blueRef[2]);
    int dMetal = dist(r,g,b, metalRef[0],metalRef[1],metalRef[2]);

    Serial.print("R:"); Serial.print(r);
    Serial.print(" G:"); Serial.print(g);
    Serial.print(" B:"); Serial.print(b);
    Serial.print(" | dR:"); Serial.print(dRed);
    Serial.print(" dG:"); Serial.print(dGreen);
    Serial.print(" dB:"); Serial.print(dBlue);
    Serial.print(" dM:"); Serial.println(dMetal);

    int minDist = dRed;
    int detected = 1;

    if (dGreen < minDist) { minDist = dGreen; detected = 3; }
    if (dBlue  < minDist) { minDist = dBlue;  detected = 2; }
    if (dMetal < minDist) { minDist = dMetal; detected = 4; }

    if (minDist < 40) {
      activeColor = detected;
      step = 0;

      Serial.print("Locked color: ");
      Serial.println(activeColor);
    }
  }

  // -------- STEP SYSTEM --------
  if (activeColor == 1) {

    // STEP 0
    if (step == 0) {
      motorC_forward();
      motorV_forward();

      // C finishes
      if (countC >= 24 && !C_done) {
        motorC_stop();
        C_done = true;
        Serial.println("C reached 24");
      }

      // V finishes
      if (countV >= 1 && !V_done) {
        motorV_stop();
        V_done = true;
        Serial.println("V reached 1");
      }

      // BOTH finished → next step
      if (C_done && V_done) {
        Serial.println("Step 0 done");

        countC = 0;
        countV = 0;
        C_done = false;
        V_done = false;

        step = 1;
      }
    }

    // STEP 1
    else if (step == 1) {
      motorV_stop();
      motorV_forward();

      if (countV >= 2) {
        Serial.println("Step 1 done");

        motorV_stop();
        countV = 0;

        step = 2;
      }
    }

    // STEP 2
    else if (step == 2) {
      motorC_forward();
      motorV_forward();

      if (countC >= 72 && !C_done) {
        motorC_stop();
        C_done = true;
      }

      if (countV >= 1 && !V_done) {
        motorV_stop();
        V_done = true;
      }

      if (C_done && V_done) {
        Serial.println("RED cycle DONE");
        resetCycle();
      }
    }
  }

  else if (activeColor == 2) { // BLUE
    // STEP 0
    if (step == 0) {
      motorC_forward();
      motorV_forward();

      // C finishes
      if (countC >= 48 && !C_done) {
        motorC_stop();
        C_done = true;
        Serial.println("C reached 48");
      }

      // V finishes
      if (countV >= 1 && !V_done) {
        motorV_stop();
        V_done = true;
        Serial.println("V reached 1");
      }

      // BOTH finished → next step
      if (C_done && V_done) {
        Serial.println("Step 0 done");

        countC = 0;
        countV = 0;
        C_done = false;
        V_done = false;

        step = 1;
      }
    }

    // STEP 1
    else if (step == 1) {
      motorV_stop();
      motorV_forward();

      if (countV >= 2) {
        Serial.println("Step 1 done");

        motorV_stop();
        countV = 0;

        step = 2;
      }
    }

    // STEP 2
    else if (step == 2) {
      motorC_forward();
      motorV_forward();

      if (countC >= 48 && !C_done) {
        motorC_stop();
        C_done = true;
      }

      if (countV >= 1 && !V_done) {
        motorV_stop();
        V_done = true;
      }

      if (C_done && V_done) {
        Serial.println("Blue cycle DONE");
        resetCycle();
      }
    }
  }

  else if (activeColor == 3) { // GREEN
    // STEP 0
    if (step == 0) {
      motorC_forward();
      motorV_forward();

      // C finishes
      if (countC >= 72 && !C_done) {
        motorC_stop();
        C_done = true;
        Serial.println("C reached 72");
      }

      // V finishes
      if (countV >= 1 && !V_done) {
        motorV_stop();
        V_done = true;
        Serial.println("V reached 72");
      }

      // BOTH finished → next step
      if (C_done && V_done) {
        Serial.println("Step 0 done");

        countC = 0;
        countV = 0;
        C_done = false;
        V_done = false;

        step = 1;
      }
    }

    // STEP 1
    else if (step == 1) {
      motorV_stop();
      motorV_forward();

      if (countV >= 2) {
        Serial.println("Step 1 done");

        motorV_stop();
        countV = 0;

        step = 2;
      }
    }

    // STEP 2
    else if (step == 2) {
      motorC_forward();
      motorV_forward();

      if (countC >= 24 && !C_done) {
        motorC_stop();
        C_done = true;
      }

      if (countV >= 1 && !V_done) {
        motorV_stop();
        V_done = true;
      }

      if (C_done && V_done) {
        Serial.println("RED cycle DONE");
        resetCycle();
      }
    }
  }

  else if (activeColor == 4) { // Metal
    if (step == 0) {
      resetCycle();
    }
  }
}

// -------- RESET --------

void resetCycle() {
  motorC_stop();
  motorV_stop();
  step = 0;
  activeColor = 0;
  countC = 0;
  countV = 0;
  C_done = false;
  V_done = false;
  delay(1000);
}  