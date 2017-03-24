/*
' *************************************************************
' FILE:    IambicKeyer.ino
'
' PURPOSE:
' This application implements an Iambic Keyer with multiple applications.
'
' DOCUMENTATION:
' See PROJECT - Iambic Electronic Keyer Utilizing The Arduino Duo.odt
' See https://github.com/WyomingGeezer/IambicKeyer 
'
' REVISIONS:
' 03/15/2017 JRH Initial creation
' 03/23/2017 JRH Published "as is" state to GitHub
'
'
' TODO:
1. Add code for an LCD display
2. Add code for recording Morse code as sent.
3. Flesh out documentation, including a schematic of the desired implementation.
'****************************************************************
*/


// INCLUDE FILES FROM ARDUINO LIBRARY
// See http://playground.arduino.cc/Code/Timer1
#include <TimerOne.h>
// See https://bitbucket.org/teckel12/arduino-toneac2/wiki/Home
#include <toneAC2.h>


//INCLUDE FILES FROM JRH LIBRARY

#include "morse.h"
#include "copyright.h"

/*
 * GENERAL INFORMATION:
 * 
 */
// ---------------------------------------------------------------------------
// TODO: Explain this with a diagram. For now, see toneAC2.h documentation
// Be sure to include an inline 100 ohm resistor on one pin as you normally do when connecting a piezo or speaker.
// ---------------------------------------------------------------------------



// MORSE CODE BASICS
/* The basic element of Morse code is the dot and all other elements can be defined in terms of multiples of the dot length.
  The word PARIS is used because this is the length of a typical word in English plain text,
  it has a total length of 50 dot lengths. If the word PARIS can be sent ten times in a
  minute using normal Morse code timing then the code speed is 10 WPM.

  The character speed is related to dot length in seconds by the following formula:

  Speed (WPM) = 2.4 * (Dots per second)

  Here are the ratios for the other code elements:
  Dash length
  Dot length x 3
  Pause between elements
  Dot length
  Pause between characters
  Dot length x 3
  Pause between words (see note)
  Dot length x 7
  REF: http://www.nu-ware.com/NuCode%20Help/index.html?morse_code_structure_and_timing_.htm
*/


// GLOBAL VARIABLES
long glTimer_Interval = 55712; // dit period in microseconds
String gsMorse;
String gsCallSign = "W7YV";
volatile boolean gbState = false;
String gsLastMessage;
int giSideToneFrequency = 650;
int giPADDLE_DIT_TIME = 75 ; //wpm = 1200 / pdt
// CONSTANTS
const int pinKEY_DIT = 2;
const int pinKEY_DASH = 3;
const int pin_SPEAKER_RED = 4; //2;
const int pin_SPEAKER_BLACK = 5; //3;


void setWordsPerMinute(int wpm) {
  // 62500 = PARIS in 3.37 seconds = 17.804 wpm -- 1112750
  // 74184 = PARIS in 3.99 seconds = 15.04 wpm -- 1115727
  // avergage resultant equation: glTimer_Interval = 1114239 / wpm
  glTimer_Interval = 1114239 / wpm ;

  giPADDLE_DIT_TIME = 1200 / wpm;

}


void setup2() {
  // setup IRQ for keys
  pinMode(pinKEY_DIT, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(pinKEY_DIT), keyStateMachine, CHANGE);
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pinKEY_DIT, INPUT_PULLUP);
  pinMode(pinKEY_DASH, INPUT_PULLUP);
  setup2();


  setWordsPerMinute(18);
  Serial.begin(9600);
  Serial.println(Copyright);

  delay(1000);

} 


volatile int giKeyDitState = 0;
volatile int giKeyDashState = 0;
volatile int giKeyState = 0;
volatile unsigned long glKeyChangeTime = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long glDebounceDelay = 5;    // the debounce time; increase if the output flickers
const int KEY_STRAIGHT = 0;
const int KEY_PADDLE = 1;
const int KEY_PADDLE_REVERSE = 2;


int giKeyMode = KEY_PADDLE;

// Called from loop() to poll mechanical keys connected to the board
void processKeyers() {
  switch (giKeyMode) {
    case KEY_STRAIGHT:
      straightKeyStateMachine();
      break;
    case KEY_PADDLE:
      paddleKeyStateMachine();
      break;
    case KEY_PADDLE_REVERSE:
      paddleKeyStateMachine(); // TODO: NOT IMPLEMENTED
      break;


  }

}
/////////////////////////////////////////////////////////////////
// This section contains the routines for the Iambic Keyer, i.e., a dual
// paddle key that produces a stream of dits and dashes. The routines have the
// following functions:
// paddleKeyDitStateMachine() handles the "dit" side of the Iambic key.
// paddleKeyDashStateMahcine() handles the "dash" side of the Iambic key.
// paddleKeyStateMachine() handles the Iambic Key function.

int paddleKeyDitStateMachine() { // handles only the DIT side of the key
  switch (giKeyDitState) {

    case 0: // nothing
      if (digitalRead(pinKEY_DIT) == LOW)

      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < glDebounceDelay) {
          //delay(1);
        }
        if (digitalRead(pinKEY_DIT) == LOW) {
          giKeyDitState = 1;
        }
      }
      break;

    case 1: // confirmed: dit went down
      glKeyChangeTime = millis();
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
      giKeyDitState = 2; //now wait the dit time
      break;

    case 2: // wait for dit time
      while ((millis() - glKeyChangeTime) < giPADDLE_DIT_TIME) {
        //delay(1);
      }
      giKeyDitState = 3;
      break;
    case 3: // kill the tone and wait for interdit time
      noToneAC2();
      while ((millis() - glKeyChangeTime) < giPADDLE_DIT_TIME * 2) {
        //delay(1);
      }

      giKeyDitState = 4;
      break;

    case 4: // dit is down, interdit is done, has key been released?
      if (digitalRead(pinKEY_DIT) == HIGH)
      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < glDebounceDelay) {
          delay(1);
        }
        if (digitalRead(pinKEY_DIT) == HIGH) {
          noToneAC2();
          giKeyDitState = 0;
        }
      } else {
        // key still down, go back and do more dits
        giKeyDitState = 1;
      }

      break;

  }

  return giKeyDitState;


}
int paddleKeyDashStateMachine() { // Handles only the DASH side of the key



  switch (giKeyDashState) {

    case 0: // nothing
      if (digitalRead(pinKEY_DASH) == LOW)

      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < glDebounceDelay) {
          delay(1);
        }
        if (digitalRead(pinKEY_DASH) == LOW) {
          giKeyDashState = 1;
        }
      }
      break;

    case 1: // confirmed: dash went down
      glKeyChangeTime = millis();
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
      giKeyDashState = 2; //now wait the dash time
      break;

    case 2: // wait for dash time
      while ((millis() - glKeyChangeTime) < (giPADDLE_DIT_TIME * 3)) {
        delay(1);
      }
      giKeyDashState = 3;
      break;
    case 3: // kill the tone and wait for element spacing time
      noToneAC2();
      while ((millis() - glKeyChangeTime) < (giPADDLE_DIT_TIME * 4)) {
        //delay(1);
        if (digitalRead(pinKEY_DASH) == HIGH) {
          //giKeyDashState = 0;
          // break;
        }
      }

      giKeyDashState = 4;
      break;

    case 4: // dash is down, element spacing time is done, has key been released?
      if (digitalRead(pinKEY_DASH) == HIGH)
      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < glDebounceDelay) {
          //delay(1);
        }
        if (digitalRead(pinKEY_DASH) == HIGH) {

          giKeyDashState = 0;
        }
      } else {
        // key still down, go back and do more dits
        giKeyDashState = 1;
      }

      break;

  }
  return giKeyDashState;
}
// paddleKeyStateMachine() is the main routine for handling Iambic Keys. It polls both sides of the
// key. When a key down condition is detected, the routine "locks" into that side until the key is released.
void paddleKeyStateMachine() {

  while (paddleKeyDitStateMachine() != 0) {} ;
  while (paddleKeyDashStateMachine() != 0 ) { };

}
// END OF IAMBIC KEY HANDLING
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// This section contains the routines for the handling a straight key.

void straightKeyStateMachine() {
  switch (giKeyState) {

    case 0: // nothing
      if (digitalRead(pinKEY_DIT) == LOW)

      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < glDebounceDelay) {
          delay(1);
        }
        if (digitalRead(pinKEY_DIT) == LOW) {
          giKeyState = 1;
        }
      }
      break;

    case 1: // dit went down
      glKeyChangeTime = millis();
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
      giKeyState = 2;
      break;
    case 2: // dit is down, wait for dit time,
      if (digitalRead(pinKEY_DIT) == HIGH)
      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < glDebounceDelay) {
          delay(1);
        }
        if (digitalRead(pinKEY_DIT) == HIGH) {
          noToneAC2();
          giKeyState = 0;
        }
      }

      break;

  }
}

// END OF IAMBIC KEY HANDLING
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// stateMachine() accepts a text string containing dots, dashes, and
// space characters and generates Morse code keying from that stream.
// INPUTS:
// gsMorse - a global String containinng dots, dashes, and space characters.
// gbState - the State Variable that toggles between key down and key up conditions.
// Other - see definitions elsewhere in this document.
void stateMachine() {

  if (gbState == true) {
    if (digitalRead(pinKEY_DASH) == LOW) {
      digitalWrite(LED_BUILTIN, HIGH);
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
      //Serial.print("650");
      Timer1.initialize(glTimer_Interval * 3);

    } else if (digitalRead(pinKEY_DIT) == LOW) {
      digitalWrite(LED_BUILTIN, HIGH);
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
      //Serial.print("650");
      Timer1.initialize(glTimer_Interval);

    } else    if (gsMorse.length() != 0) {
      String element ;
      element = gsMorse.substring(0, 1);

      //Serial.print(" E="); Serial.print(element);
      //Serial.print(" MC="); Serial.print(gsMorse); Serial.println(gsMorse.length());


      if (element == " ") {
        Timer1.initialize(glTimer_Interval * 2);
      }
      if (element == ".") {
        digitalWrite(LED_BUILTIN, HIGH);
        toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
        //Serial.print("650");
        Timer1.initialize(glTimer_Interval);
      }
      if (element == "-") {
        digitalWrite(LED_BUILTIN, HIGH);
        toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, giSideToneFrequency);
        //Serial.print("800");
        Timer1.initialize(glTimer_Interval * 3);
      }
      if (gsMorse.length() != 0 ) {
        gsMorse = gsMorse.substring(1);
      }
    } else {
      Timer1.detachInterrupt();
    }

  }


  if (gbState == false) {
    Timer1.initialize(glTimer_Interval);
    noToneAC2();
    digitalWrite(LED_BUILTIN, LOW);
  }
  gbState = !gbState ;
}


String convertToMorse(String strText) {

  String letter, strInput;
  // Define

  // Length (with one extra character for the null terminator)
  int str_len = strText.length() + 1;

  // Prepare the character array (the buffer)
  char char_array[str_len];

  // Copy it over
  strText.toCharArray(char_array, str_len);


  for ( int i = 0; i <= strText.length(); i++) {

    char letter;
    letter = char_array[i];

    if (letter == ' ') {
      strInput = strInput + ' ';
    } else if (letter <= '9') {
      strInput = strInput + numbers[int(letter - 48)]; strInput = strInput + ' ';
      //Serial.println(strInput);
      //break;
    } else if (letter >= 'A') {
      strInput = strInput + letters[int(letter) - 65]; strInput = strInput + ' ';
      //Serial.println(strInput);
      //break;

    }
  }
  return strInput;
}

// END OF TEXT STREAM MORSE CODE HANDLING
/////////////////////////////////////////////////////////////////


String getInputLine(void) {

  String strInput;
  while (Serial.available() > 0) {
    delay(10);
    //Serial.print(Serial.available());
    //toneAC2(2, 3, 650,50);
    char letter = Serial.read(); // read ONE character from the keyboard
    //delay(500);
    //Serial.println(int(letter));
    //noInterrupts();
    if (letter == 13 ) { //look for CR key
      //Serial.println(strInput);
      break;
    }
    strInput = strInput + letter;
  }

  return strInput;
}

void listSettings() {

  // list settings
  Serial.println("$CALLSIGN = " + gsCallSign);
  //Serial.println("$LM = " + gsLastMessage);
  Serial.print("$KM = "); Serial.println(giKeyMode);
  Serial.print("$PDT = "); Serial.println(giPADDLE_DIT_TIME);
  Serial.print("$WPM = "); Serial.println(1114239 / glTimer_Interval);
  Serial.print("$DBD = "); Serial.println(glDebounceDelay);
  Serial.print("$STF = "); Serial.println(giSideToneFrequency);


}

void(* resetFunc) (void) = 0;//declare reset function at address 0


boolean processCommands(String strCmd) {
  strCmd.toUpperCase();
  if (strCmd.substring(0, 9) == "$CALLSIGN" ) {
    Serial.print("Callsign was ") ; Serial.println(gsCallSign);
    gsCallSign = strCmd.substring(9);
    Serial.print("Callsign set to ") ; Serial.println(gsCallSign);
    return true;
  }
  /*
    const int KEY_STRAIGHT = 0;
    const int KEY_PADDLE = 1;
    const int KEY_PADDLE_REVERSE = 2;
  */

  if (strCmd.substring(0, 3) == "$KM") {


    giKeyMode = strCmd.substring(3).toInt();
    Serial.print("Key mode set to "); Serial.println(giKeyMode);

    return true;
  }


  if (strCmd.substring(0, 4) == "$PDT") {


    giPADDLE_DIT_TIME = strCmd.substring(4).toInt();
    Serial.print("Paddle dit time set to "); Serial.println(giPADDLE_DIT_TIME);

    return true;
  }



  if (strCmd.substring(0, 6) == "$RESET") {

    resetFunc(); //call reset
    return true;
  }



  if (strCmd.substring(0, 4) == "$WPM") {

    int iWPM ;
    iWPM = strCmd.substring(4).toInt();
    Serial.print("Keyer speed set to "); Serial.println(iWPM);
    setWordsPerMinute(iWPM);
    return true;
  }

  if (strCmd.substring(0, 4) == "$DBD") {


    glDebounceDelay = strCmd.substring(4).toInt();
    Serial.print("Debounce delay set to "); Serial.println(glDebounceDelay);

    return true;
  }
  if (strCmd.substring(0, 4) == "$STF") {


    giSideToneFrequency = strCmd.substring(4).toInt();
    Serial.print("Sidetone frequency set to "); Serial.println(giSideToneFrequency);

    return true;
  }

  if (strCmd.substring(0, 3) == "$LM" ) {
    if (gsLastMessage.length() == 0) {
      Serial.println("No last message stored.");
      return true;
    }
    Serial.print(gsLastMessage.length());
    strCmd = gsLastMessage ;

  }
  if (strCmd.substring(0, 3) == "$LS" ) {
    listSettings();
    strCmd = "";
    return true;
  }

  if (strCmd.substring(0, 1) == "$" ) {
    // unknown command
    Serial.println("UNKNOWN COMMAND: " + strCmd);
    gsMorse = "........";
    return false;

  }
  // if here, there are no commands so we assume the input is a msg to send
  gsLastMessage = strCmd;
  Serial.println(strCmd);
  String sTemp = convertToMorse(strCmd);
  Serial.println(sTemp);
  gsMorse = sTemp; // post the CW string to the buffer monitored by the IRQ.
  return false ;

}

//////////////////////////////////////////////////////////////
// loop() is the "main" entry point for Arduino code. All roads lead from here.

void loop() {
  processKeyers(); // Poll the key to see what is needed

// TODO: Timers need to be invoked only after it is determined that the input string is intended to be sent as Morse code.

  String strInput;

  strInput = getInputLine(); // fetch a line from the console


  if (strInput.length()) { // if a string exists, start the timers and prepare to generate Morse code.
    Timer1.initialize(glTimer_Interval);
    Timer1.attachInterrupt(stateMachine); // stateMachine to run every ditTime


    if (processCommands(strInput) == false) {

    }

  }
  strInput = "";

  //interrupts();

}

