// Ritvars Timermanis
//    04/12/2015
//    Arduino.ino
// "Locked In Edinburgh" 

#include <LiquidCrystal.h>
#include <string.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

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

const int defuseOrder[10] = {36, 46, 30, 40, 34, 32, 48, 38, 44, 42}; // Yellow > White > Brown > Blue > Orange > Red > Black > Green > Grey > Purple
int errorCount = 0;
int previousErrorCount = 0;
int wiresCut = 0;

const char* wire[] = {"wire_BROWN", "wire_RED", "wire_ORANGE", "wire_YELLOW", "wire_GREEN", "wire_BLUE", "wire_PURPLE", "wire_GREY", "wire_WHITE", "wire_BLACK"};
const int wirePin[] = {30, 32, 34, 36, 38, 40, 42, 44, 46, 48};
bool wireState[10];
bool wireLastState[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  Serial.begin(9600);

  //Enable the pullup resistor & set wire pins as input
  for (unsigned int i = 0; i < sizeof(wire); i++){
      pinMode(wirePin[i], INPUT_PULLUP);
  }

  //Set error LED pins to OUTPUT
  for (unsigned int a = 0; a < sizeof(errorLED); a++){
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
  digitalWrite(24, HIGH);
  delay(1000);
  digitalWrite(24, LOW);
  delay(1000);

  if (previousErrorCount != errorCount){
    digitalWrite(errorLED[errorCount], HIGH);
    previousErrorCount = errorCount;
  }

  for (unsigned int i = 0; i < sizeof(wireState); i++){
      wireState[i] = digitalRead(wirePin[i]);
      /*Serial.print("Wire: ");
      Serial.print(wire[i]);
      Serial.print(" State: ");
      Serial.print(wireState[i]);
      Serial.println("");*/

      if ((wireState[i] == HIGH) && (wireState[i] != wireLastState[i])){
        wireLastState[i] = wireState[i];
        wiresCut++;
        wireChanged(wirePin[i]);
      }
  }

  switch(errorCount){
    case 1: digitalWrite(errorLED[0], HIGH); break;
    case 2: digitalWrite(errorLED[1], HIGH); break;
    case 3: digitalWrite(errorLED[2], HIGH); break;
    case 4: digitalWrite(errorLED[3], HIGH); break;
    case 5: digitalWrite(errorLED[4], HIGH); break;
    case 6: boom(); break;
    default: break;}

  buttonState = digitalRead(openSwitch);
    if (buttonState != lastButtonState) {
      if (buttonState == HIGH && ARMED && testPassed) {
        if (!doCountdown) {
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

  if (errorCount == 5){
    boom();
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

void wireChanged(int x){
  if (x != defuseOrder[wiresCut]){
    errorCount++;
    Serial.print("ERROR: ");
    Serial.print(errorCount);
    Serial.println(" ");
  }
}

bool boom(){
  lcd.clear();
  doCountdown = false;
  countdownDone = true;

  for (unsigned int a = 0; a < sizeof(errorLED); a++){
    digitalWrite(errorLED[a], 1);
    delay(200);
  }

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
}

void wiretest(){
  lcd.clear();
  lcd.print("Status: ");
  const char* result[] = {"FAIL", "PASS"};

  //Read the current state of wire pins and store them in wireState
  for (unsigned int x = 0; x < sizeof(wirePin); x++){
    wireState[x] = digitalRead(wirePin[x]);
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
        delay(3000);
        lcd.clear();
        lcd.print("Status: ARMED");
        testPassed = true;
  } else {
    lcd.print(result[0]); //If wire/s not connected display fail message
    testPassed = false;
  }
}
