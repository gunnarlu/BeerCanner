#include <Button.h>
#include <EEPROM.h>

int buttonLeftPin = 2;
int buttonRightPin = 3;

int leftSensorPin = 1;
int leftGasPin = 10;
int leftLiquidPin = 9;

int rightSensorPin = 0;
int rightGasPin = 8;
int rightLiquidPin = 7;

int leftLedRedPin = 13;

int leftLedBluePin = 12;
int leftLedGreenPin = 11;

int rightLedRedPin = 6;
int rightLedBluePin = 5;
int rightLedGreenPin = 4;

int leftButtonState = 0;
int rightButtonState = 0;

unsigned long currentTime = 0;
unsigned long CountClicksPreviousTime = 0;
int gasPurgeTimer = 3000;

int CountingPurgeClicks = 0;

Button leftButton = Button(2, BUTTON_PULLDOWN, true, 10);
Button rightButton = Button(3, BUTTON_PULLDOWN, true, 10);

unsigned long eepromAddressLeftPurgeTime = 0;
int eepromAddressLeftPurgeSensorValue = 5;
int eepromAddressLeftLiquidFillSensorValue = 10;

unsigned long eepromAddressRightPurgeTime = 15;
int eepromAddressRightPurgeValue = 20;
int eepromAddressRightLiquidFillSensorValue = 25;


struct ledToBlink {
  volatile byte ledState;
  unsigned long blinkTimer;
  int blinkState;
  int blinkInterval;
  int ledPin;
  int doubleBlinkState;

};

struct stateArtifact {

  int state;
  ledToBlink blueBlink;
  ledToBlink redBlink;
  ledToBlink greenBlink;

  int gasPin;
  int liquidPin;
  int sensorPin;

  unsigned long gasPurgePreviousTimeInSecs;
  unsigned long gasPurgeTimeLimitInSecs;
  unsigned long liquidFillerSensorValue;
  unsigned long previousTime;

  int eepromAddressPurgeTime;
  int eepromAddressLiquidFillSensorValue;


};


stateArtifact leftState = {

  0,
  {LOW, 0, 0, 300, leftLedBluePin, 0 },
  {LOW, 0, 0, 300, leftLedRedPin, 0 },
  {LOW, 0, 0, 300, leftLedGreenPin, 0 },
  leftGasPin,
  leftLiquidPin,
  leftSensorPin,
  0L,
  0L,
  0L,
  eepromAddressLeftPurgeTime,
  eepromAddressLeftLiquidFillSensorValue,

};

stateArtifact rightState = {

  0,
  {LOW, 0, 0, 300, rightLedBluePin, 0 },
  {LOW, 0, 0, 300, rightLedRedPin, 0 },
  {LOW, 0, 0, 300, rightLedGreenPin, 0 },
  rightGasPin,
  rightLiquidPin,
  rightSensorPin,
  0L,
  0L,
  0L,
  eepromAddressRightPurgeTime,
  eepromAddressRightLiquidFillSensorValue,


};

void setup() {

  //TODO: Calibrate sensors at start!
  Serial.begin(9600);

  pinMode(leftLedRedPin, OUTPUT);
  pinMode(leftLedBluePin, OUTPUT);
  pinMode(leftLedGreenPin, OUTPUT);
  pinMode(rightLedRedPin, OUTPUT);
  pinMode(rightLedBluePin, OUTPUT);
  pinMode(rightLedGreenPin, OUTPUT);

  pinMode(leftSensorPin, INPUT);
  pinMode(leftGasPin, OUTPUT);
  pinMode(leftLiquidPin, OUTPUT);

  pinMode(rightSensorPin, INPUT);
  pinMode(rightGasPin, OUTPUT);
  pinMode(rightLiquidPin, OUTPUT);

  leftButton.releaseHandler(leftButtonOnRelease);
  rightButton.releaseHandler(rightButtonOnRelease);
  rightButton.pressHandler(rightButtonOnPress);
  leftButton.pressHandler(leftButtonOnPress);
  leftButton.holdHandler(leftButtonOnHold, 3000);
  rightButton.holdHandler(rightButtonOnHold, 3000);


  EEPROM.get(leftState.eepromAddressPurgeTime, leftState.gasPurgeTimeLimitInSecs);
  EEPROM.get(rightState.eepromAddressPurgeTime, rightState.gasPurgeTimeLimitInSecs);

  EEPROM.get(leftState.eepromAddressLiquidFillSensorValue, leftState.liquidFillerSensorValue);

  EEPROM.get(rightState.eepromAddressLiquidFillSensorValue, rightState.liquidFillerSensorValue);
  unsigned long previousTime;

  Serial.println("Setup completed");

}

void leftButtonOnRelease(Button& b) {

  if (leftState.state == 1 or  leftState.state == 2)  {
    return;
  }
  buttonReleaseSwitch (leftState);

}

void leftButtonOnPress(Button& b) {

  switch (leftState.state) {
    case 5:
      leftState.state = 6;
      break;
  }
}

void leftButtonOnHold(Button& b) {
  // We dont start any management operations if a fill is going on in any of the lines
  if (leftState.state != 0 ) {
    return;
  }

  // We have detected a button hold
  leftState.state = 3;
}

void rightButtonOnRelease(Button& b) {

  if (rightState.state == 1 or  rightState.state == 2)  {
    return;
  }
  buttonReleaseSwitch (rightState);

}

void rightButtonOnHold(Button& b) {

  // We dont start any management operations if a fill is going on in any of the lines

  if (rightState.state != 0 ) {
    return;
  }

  // We have detected a button hold

  rightState.state = 3;


}

void rightButtonOnPress(Button& b) {

  switch (rightState.state) {
    case 5:
      rightState.state = 6;
      break;
  }

}

void buttonReleaseSwitch ( stateArtifact &state) {

  switch (state.state) {

    case 0:
      //Start the filling sequence
      state.state = 1;
      break;
    case 2:
      //The fill sequece has ended
      state.state = 0;
      break;
    case 3:
      //The user wants to set the purge time
      state.state = 7;
      state.previousTime = 0;
      break;
    case 4:
      //The user wants to set purge time
      state.state = 8;
      state.previousTime = 0;
      break;
    case 6:
      state.state = 0;
      break;
    case 7:
      state.previousTime = 0;
      CountingPurgeClicks++;
      break;
    case 8:
      state.state = 9;
      break;
  }
}

void blinkLed (ledToBlink &led) {

  if ((led.blinkState == 1) && (currentTime - led.blinkTimer > led.blinkInterval)) {
    led.blinkTimer = currentTime;
    led.ledState =  !led.ledState;
    digitalWrite(led.ledPin, led.ledState);
  }
}

void blinkTwoLeds (ledToBlink &firstLed, ledToBlink &secondLed ) {

  if ((firstLed.doubleBlinkState == 1) && (secondLed.doubleBlinkState == 1) && (currentTime - firstLed.blinkTimer > firstLed.blinkInterval)) {
    firstLed.blinkState = 0;
    secondLed.blinkState = 0;

    firstLed.blinkTimer = currentTime;
    if (firstLed.ledState ==  secondLed.ledState) {
      firstLed.ledState =  !firstLed.ledState;
    }
    firstLed.ledState =  !firstLed.ledState;
    secondLed.ledState =  !secondLed.ledState;
    digitalWrite(firstLed.ledPin, firstLed.ledState);
    digitalWrite(secondLed.ledPin, secondLed.ledState);
  }
}

void loop() {
  //IMPORTANT: Update the button internals first of all
  leftButton.process();
  rightButton.process();
  currentTime = millis();

  stateMachine(leftState);
  stateMachine(rightState);
}


void stateMachine (stateArtifact &state) {

  switch (state.state) {
    case 0:
      //Reset state
      state.gasPurgePreviousTimeInSecs = 0;
      state.previousTime = 0;
      if (state.greenBlink.blinkState == 0) {
        digitalWrite(state.greenBlink.ledPin, HIGH);
      }
      break;

    case 1:
      // Starting the purge - turning the blue led on to signal purge start
      digitalWrite(state.greenBlink.ledPin, LOW);
      state.blueBlink.blinkState = 1;
      if (state.gasPurgePreviousTimeInSecs == 0 ) {
        state.gasPurgePreviousTimeInSecs = currentTime;
      }
      digitalWrite(state.gasPin, HIGH);
      if (currentTime - state.gasPurgePreviousTimeInSecs > state.gasPurgeTimeLimitInSecs) {
        digitalWrite(state.gasPin, LOW);
        state.gasPurgePreviousTimeInSecs = 0;
        state.blueBlink.blinkState = 0;
        digitalWrite(state.blueBlink.ledPin, LOW);
        state.state = 2;
      }
      break;

    case 2:
      //Starting the liquid fill
      state.redBlink.blinkState = 1;

      if (state.previousTime == 0) {
        state.previousTime = currentTime;
      }

      digitalWrite(state.liquidPin, HIGH);
      if ((analogRead(state.sensorPin) > state.liquidFillerSensorValue) && (currentTime - state.previousTime > 2000)) {
        //state.liquidFillerPreviousTime = 0;
        digitalWrite(state.liquidPin, LOW);
        state.redBlink.blinkState = 0;
        digitalWrite(state.redBlink.ledPin, LOW);
        state.state = 0;
      }
      break;

    case 3:
      // We have detected a button hold.  Check if we get a button push within 3 seconds.  If so we will send them to state 7. Handled in button release handler above
      state.greenBlink.blinkState = 1;
      if (state.previousTime == 0) {
        state.previousTime = currentTime;
      }

      if ( currentTime - state.previousTime > 3000) {
        //We go no button click, so user did not want to set purge time.
        state.state = 4;
        state.previousTime = currentTime;
      }
      break;

    case 4:
      //Check if the user wants to set set fill level
      digitalWrite(state.greenBlink.ledPin, LOW);
      state.greenBlink.blinkState = 0;
      state.blueBlink.blinkState = 1;
      if ( currentTime - state.previousTime > 3000) {
        //move to next action
        state.state = 5;
        state.previousTime = 0;
      }
      break;

    case 5:
      // Start the cleaning mode. Open the line and wait for a putton push to close
      digitalWrite(state.blueBlink.ledPin, LOW);
      state.blueBlink.blinkState = 0;
      state.greenBlink.doubleBlinkState = 1;
      state.redBlink.doubleBlinkState = 1;
      digitalWrite(state.liquidPin, HIGH);
      break;

    case 6:
      //Button press detected to stop the cleaning
      digitalWrite(state.liquidPin, LOW);
      digitalWrite(state.redBlink.ledPin, LOW);
      state.greenBlink.doubleBlinkState = 0;
      state.redBlink.doubleBlinkState = 0;
      break;

    case 7:
      //We come from state 3 and this indicates that the user wants to set purge time
      //We need to count clicks of the button until there are no clicks in 3 seconds
      if (state.previousTime == 0) {
        state.previousTime = currentTime;
      }

      if ( currentTime - state.previousTime > 3000) {
        digitalWrite(state.greenBlink.ledPin, LOW);
        state.greenBlink.blinkState = 0;
        state.gasPurgeTimeLimitInSecs = CountingPurgeClicks * 1000;
        EEPROM.put(state.eepromAddressPurgeTime, state.gasPurgeTimeLimitInSecs);
        state.state = 0;
      }
      break;

    case 8:
      //We come from state 4 and we are setting the fill state.
      if (state.previousTime == 0) {
        state.previousTime = currentTime;
      }
      digitalWrite(state.blueBlink.ledPin, LOW);
      state.blueBlink.blinkState = 0;
      state.blueBlink.doubleBlinkState = 1;
      state.redBlink.doubleBlinkState = 1;
      digitalWrite(state.liquidPin, HIGH);
      break;

    case 9:

      state.liquidFillerSensorValue = analogRead(state.sensorPin);
      digitalWrite(state.liquidPin, LOW);
      state.blueBlink.doubleBlinkState = 0;
      state.redBlink.doubleBlinkState = 0;
      digitalWrite(state.blueBlink.ledPin, LOW);
      digitalWrite(state.redBlink.ledPin, LOW);
      EEPROM.put(state.eepromAddressLiquidFillSensorValue, state.liquidFillerSensorValue);
      state.previousTime = 0;
      state.state = 0;
      break;

  }

  blinkLed(state.blueBlink);
  blinkLed(state.redBlink);
  blinkLed(state.greenBlink);
  blinkTwoLeds(state.blueBlink, state.redBlink);
  blinkTwoLeds(state.greenBlink, state.redBlink);

}
