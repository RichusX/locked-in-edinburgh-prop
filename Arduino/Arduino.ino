// Developed by Ritvars Timermanis
//        04/12/2015
//        Arduino.ino
//    "Locked In Edinburgh"
// Special thanks to Mike Walters

#include <LiquidCrystal.h>
#include <string.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

#define ARRAYSIZE(arr) (sizeof(arr)/sizeof(arr[0]))

const int openSwitch = 53;
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button
bool ARMED;
bool testPassed = false;

bool buttonPressed = true;
bool doCountdown = false;
bool countdownDone = false;
unsigned long lastCount = 0;

int siren = 24; //Siren pin

const int hoursX =    0; // start hours
const int minutesX =  10; //start min
const int secondsX =  0; //start seconds
int hours = hoursX;
int minutes = minutesX;
int seconds = secondsX;

const int errorLED[5] = {31, 33, 35, 37, 39};

const int R = 4; //Red
const int G = 3; //Green
const int B = 2; //Blue

const int defuseOrder[10] = {36, 46, 30, 40, 34, 32, 48, 38, 44, 42}; // Yellow > White > Brown > Blue > Orange > Red > Black > Green > Grey > Purple
int errorCount = 0;
int previousErrorCount = 0;
int wiresCut = 0;

const char* wire[] = {"wire_BROWN", "wire_RED", "wire_ORANGE", "wire_YELLOW", "wire_GREEN", "wire_BLUE", "wire_PURPLE", "wire_GREY", "wire_WHITE", "wire_BLACK"};
bool wireState[10];
bool wireLastState[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool boomed = false;

int expectedWire = 0;

void setup() {
  //Serial.begin(9600);

  //Enable the internal pullup resistor & set wire pins as input
  for (unsigned int i = 0; i < ARRAYSIZE(wireState); i++){
      pinMode(defuseOrder[i], INPUT_PULLUP);
  }

  //Set RGB LED pins to OUTPUT
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);

  rgbLED(0); //RGB LED off

  //Set error LED pins to OUTPUT
  for (unsigned int a = 0; a < ARRAYSIZE(errorLED); a++){
      pinMode(errorLED[a], OUTPUT);
      digitalWrite(errorLED[a], LOW);
  }

  //Enable the pullup resistor for the switch
  pinMode(openSwitch, INPUT_PULLUP);

  //Initialize the LCD
  lcd.begin(16, 2);
  lcd.clear();

  //Check if all wires are connected
  wiretest();

  pinMode(siren, OUTPUT);
}

void loop() {
  if (previousErrorCount != errorCount){
    previousErrorCount = errorCount;
  }

  for (unsigned int i = 0; i < ARRAYSIZE(wireState); i++){
    if (i == expectedWire)
      continue;

    wireState[i] = digitalRead(defuseOrder[i]);

    if ((wireState[i] == 1) && (wireState[i] != wireLastState[i])){
      wireLastState[i] = wireState[i];
      wiresCut++;
      errorCount++;
    }
  }

  for (int i = 0; i < ARRAYSIZE(errorLED); i++) {
    if (i < errorCount) {
      digitalWrite(errorLED[i], HIGH);
    }
  }

  if (!boomed){
    buttonState = digitalRead(openSwitch);
      if (buttonState != lastButtonState) {
        if (buttonState == HIGH && ARMED && testPassed) {
          if (!doCountdown) {
            //Set RGB LED to Yellow
            rgbLED(4);

            doCountdown = true;
            countdownDone = false;
            lastCount = millis() - 1000;
            hours = hoursX;
            minutes = minutesX;
            seconds = secondsX;
          }
        } else if (buttonState == HIGH && !ARMED){
          ARMED = true;
        }
        delay(50); // Delay a little bit to avoid bouncing
      }
    lastButtonState = buttonState;

    if (doCountdown) { //Start the countdown
      if (!countdownDone && millis() - lastCount >= 1000) {
        bool done = countdown();
        if (done) {
          boom();
          countdownDone = true;
        }
        lastCount = millis();
      }
    }

    if (errorCount >= 5){
      boom();
    }
  }

  wireState[expectedWire] = digitalRead(defuseOrder[expectedWire]);
  if ((wireState[expectedWire] == 1) && (wireState[expectedWire] != wireLastState[expectedWire])){
    wireLastState[expectedWire] = wireState[expectedWire];
    do {
      expectedWire++;
      if (expectedWire >= 10) {
        defused();
      }
    } while(wireState[expectedWire]);
  }
}
bool countdown(){
  lcd.setCursor(0,0);
  lcd.print("Status: ACTIVE");

  lcd.setCursor(4, 2);

  (hours < 10) ? lcd.print("0") : NULL;
  lcd.print(hours);
  lcd.print(":");
  (minutes < 10) ? lcd.print("0") : NULL;
  lcd.print(minutes);
  lcd.print(":");
  (seconds < 10) ? lcd.print("0") : NULL;
  lcd.print(seconds);
  lcd.display();
  if (seconds > 0){
    seconds -= 1;
  }else{
    if (minutes > 0){
      seconds = 59;
      minutes -= 1;
    }else{
      if (hours > 0){
        seconds = 59;
        minutes = 59;
        hours -= 1;
      }else{
        return true;
      }
    }
  }
  return false;
}

bool boom(){
  lcd.clear();
  boomed = true;
  doCountdown = false;

  rgbLED(1);

  digitalWrite(siren, HIGH);

  for (unsigned int x = 0; x < 16; x++){
    lcd.setCursor(x, 0);
    lcd.print("#");
    delay(100);
  }
  for (unsigned int i = 0; i < 16; i++){
    lcd.setCursor(i, 1);
    lcd.print("#");
    delay(100);
  }

  while (1);
}

void defused(){
  digitalWrite(R, HIGH);
  digitalWrite(G, LOW);
  digitalWrite(B, HIGH);

  bool xy = false;
    while (1){
      lcd.setCursor(4,2);
      (hours < 10) ? lcd.print("0") : NULL;
      lcd.print(hours);
      lcd.print(":");
      (minutes < 10) ? lcd.print("0") : NULL;
      lcd.print(minutes);
      lcd.print(":");
      (seconds < 10) ? lcd.print("0") : NULL;
      lcd.print(seconds);

      if (!xy){
        lcd.setCursor(0,0);
        lcd.print("Status: DEFUSED");
        delay(1000);
        xy = true;
      }
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Status:");

        lcd.setCursor(4,2);
        (hours < 10) ? lcd.print("0") : NULL;
        lcd.print(hours);
        lcd.print(":");
        (minutes < 10) ? lcd.print("0") : NULL;
        lcd.print(minutes);
        lcd.print(":");
        (seconds < 10) ? lcd.print("0") : NULL;
        lcd.print(seconds);

        delay(1000);
        xy = false;
      }
    }
}

void rgbLED(int ledColour){
  switch (ledColour){
    case 0:   digitalWrite(R, HIGH);  //Off
              digitalWrite(G, HIGH);
              digitalWrite(B, HIGH);
              break;
    case 1:   digitalWrite(R, LOW);   //Red
              digitalWrite(G, HIGH);
              digitalWrite(B, HIGH);
              break;
    case 2:   digitalWrite(R, HIGH);  //Green
              digitalWrite(G, LOW);
              digitalWrite(B, HIGH);
              break;
    case 3:   digitalWrite(R, HIGH);  //Blue
              digitalWrite(G, HIGH);
              digitalWrite(B, LOW);
              break;
    case 4:   digitalWrite(R, LOW);   //Yellow
              digitalWrite(G, LOW);
              digitalWrite(B, HIGH);
              break;
    default:  digitalWrite(R, LOW);  //All on
              digitalWrite(G, LOW);
              digitalWrite(B, LOW);
              break;
  }
}

void wiretest(){
  lcd.clear();
  lcd.print("Status: ");
  const char* result[] = {"FAIL", "PASS"};

  //Read the current state of wire pins and store them in wireState
  for (unsigned int x = 0; x < ARRAYSIZE(defuseOrder); x++){
    wireState[x] = digitalRead(defuseOrder[x]);
  }

  //Check if all wires are connected (LOW)
  if (wireState[0] == LOW &&
      wireState[1] == LOW &&
      wireState[2] == LOW &&
      wireState[3] == LOW &&
      wireState[4] == LOW &&
      wireState[5] == LOW &&
      wireState[6] == LOW &&
      wireState[7] == LOW &&
      wireState[8] == LOW &&
      wireState[9] == LOW ){
        lcd.print(result[1]);
        delay(1000);
        lcd.clear();
        lcd.print("Status: ARMED");
        testPassed = true;
  } else {
    lcd.print(result[0]); //If wires aren't connected display fail message
    testPassed = false;
    while (1);
  }
}
