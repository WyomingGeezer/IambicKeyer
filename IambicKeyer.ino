
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
  ' 03/25/2017 JRH Added LED blink during Morse code, implemented KEY_PADDLE_REVERSE,
  '                fixed console blocking, added ability to send prosigns
  ' 03/26/2017 JRH Added beacon feature, added documentation, rearranged code sections
  ' 03/28/2017 JRH Fixed numerous bugs related to writing the EEPROM and beacon. Implemented
  ' F() macro.
  '
  ' TODO:
  1. Add code for an LCD display
  2. Add code for recording Morse code as sent.
  3. Flesh out documentation, including a schematic of the desired implementation.
  '****************************************************************
    Copyright (C) 2017 JAMES R. HARVEY

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/* GENERAL NOTES
 *  1. F() MACRO: See https://www.baldengineer.com/arduino-f-macro.html as well as
 *  http://playground.arduino.cc/Learning/Memory . This macro refers to "the string
 *  stored in flash memory" and saves dynamic RAM space.
 *  

*/
#define INCLUDE_HAND_KEY




// INCLUDE FILES FROM ARDUINO LIBRARY
#include <EEPROM.h>
// See http://playground.arduino.cc/Code/Timer1
#include <TimerOne.h>
// See https://bitbucket.org/teckel12/arduino-toneac2/wiki/Home
#include <toneAC2.h>
#include <MemoryFree.h>




//INCLUDE FILES FROM JRH LIBRARY

#include "morse.h"
#include "copyright.h"


/*
   GENERAL INFORMATION:

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

/////////////////////////////////////////////////////////////////
// GLOBAL CONSTANTS
const int giPinKEY_LEFT = 2; // PIN 2 of the board connected to dit side of the key
const int giPinKEY_RIGHT = 3; // PIN3 of the board connected to the DASH side of the key

// The push-pull nature of the ToneAC2 requires a 100 ohm resistor in series with the
// speaker attached to the speaker pins
const int pin_SPEAKER_RED = 4; // PIN4, connected to the speaker RED lead
const int pin_SPEAKER_BLACK = 5; // PIN5, connected to the speaker BLACK lead


// flags to define the type of key
const int KEY_STRAIGHT = 0;
const int KEY_PADDLE = 1;
const int KEY_PADDLE_REVERSE = 2;
const unsigned long EEPROM_Signature = 0x195A0B0F;
const int FLAG_ENABLE_DEBUG = 1;
const int FLAG_ENABLE_HEARTBEAT = 2;
const int FLAG_DISABLE_AUDIO = 4;


/////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
long glTimer_Interval = 55712; // dit period in microseconds
volatile String gsMorse; // string of Morse, i.e., "CQ" = "-.-. --.-" (note the space between the characters. This forces a one-dit space.
volatile long glLastKeyElementSent;
volatile boolean gbState = false; // use in StateMachine function
String gsLastMessage; // holds the last message sent by StateMachine
//gaKeyerData.iSideToneFrequency = 650; // CW side tone frequency.
int giPADDLE_DIT_TIME = 75 ; //wpm = 1200 / pdt . Milliseconds for basic dit length.
//String gsBeaconMessage = "";
//long glBeaconIntervalSeconds = 0;

volatile int giKeyDitState = 0;
volatile int giKeyDashState = 0;
volatile int giKeyState = 0;
volatile unsigned long glKeyChangeTime = 0;
//unsigned long glDebounceDelay = 5;    // key debounce time. CAREFULL: if too long, screws up key. If too short, screws up keying.
int giPinKEY_DIT ;
int giPinKEY_DASH;


//int giKeyMode = KEY_PADDLE;// defautl key value. Change with $KM

struct KeyerSetupData {
  unsigned long ulSignature ;
  int iFLAGS ;
  int iSideToneFrequency;
  byte bKeyMode;
  byte bDebounceDelay;
  long lBeaconIntervalSeconds;
  byte byWPM;
  char callsign[12];
  char sBeaconMessage[60];
};

KeyerSetupData gaKeyerData = {
  EEPROM_Signature , // EEPROM signature
  FLAG_ENABLE_HEARTBEAT,
  650, // side tone
  1, // key mode
  5, // debounce delay
  0, // beacon interval
  16, // wpm
  "W7YV",
  "DE W7YV <AR>"
};

/////////////////////////////////////////////////////////////////
void setup() {
  // initialize digital pins


  pinMode(LED_BUILTIN, OUTPUT); // Blinks when morse is sending or when key is down
  pinMode(giPinKEY_LEFT, INPUT_PULLUP);
  pinMode(giPinKEY_RIGHT, INPUT_PULLUP);
  

  // Brag stuff
  Serial.begin(9600);
  Serial.println(F(Copyright));
  Serial.println(F("Type $CR for additional copyright information."));
  Serial.print(F("EEPROM size: ")); Serial.println(EEPROM.length());
  Serial.print(F("Free RAM: ")); Serial.println(freeRam());
  debugMsg(F("Debug is enabled"));

  // read EEPROM signature
  KeyerSetupData dataEEPROM;
  EEPROM.get(0, dataEEPROM);
  if (dataEEPROM.ulSignature != gaKeyerData.ulSignature) {
    EEPROM.put(0, gaKeyerData);
    Serial.println(F("Initializing EEPROM"));
  } else
  { // load EEPROM to memory
    Serial.println(F("Loading EEPROM settings"));
    gaKeyerData = dataEEPROM;
  }
  setWordsPerMinute(gaKeyerData.byWPM);  

  listSettings();

  Serial.println("####################################################");
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
/////////////////////////////////////////////////////////////////
// setWordsPerMinute(wpm) adjusts the timers to produce the desired wpm.

void setWordsPerMinute(int wpm) {

  gaKeyerData.byWPM = wpm ;
  // 62500 = PARIS in 3.37 seconds = 17.804 wpm -- 1112750
  // 74184 = PARIS in 3.99 seconds = 15.04 wpm -- 1115727
  // avergage resultant equation: glTimer_Interval = 1114239 / wpm
  const unsigned long lTimerFactor = 1114239; // *WARNING* Processor dependant
  const int lPaddleTimeFactor = 1200;
  glTimer_Interval = lTimerFactor / wpm ;

  giPADDLE_DIT_TIME = lPaddleTimeFactor / wpm;

}


#if defined(INCLUDE_HAND_KEY)

#include "handkey.h"

#endif

/////////////////////////////////////////////////////////////////
// stateMachine() accepts a text string containing dots, dashes, and
// space characters and generates Morse code keying from that stream.
// INPUTS:
// gsMorse - a global String containinng dots, dashes, and space characters.
// gbState - the State Variable that toggles between key down and key up conditions.
// Other - see definitions elsewhere in this document.
void stateMachine() {

  if (gbState == true) {

    if (gsMorse.length() != 0) {
      String element ;
      element = gsMorse.substring(0, 1);

      if (element == " ") {
        Timer1.initialize(glTimer_Interval * 2);
      }
      if (element == ".") {
        digitalWrite(LED_BUILTIN, HIGH);
        if ((gaKeyerData.iFLAGS & FLAG_DISABLE_AUDIO) == 0) {
          toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, gaKeyerData.iSideToneFrequency);
        }
        //Serial.print("650");
        Timer1.initialize(glTimer_Interval);
      }
      if (element == "-") {
        digitalWrite(LED_BUILTIN, HIGH);
        if ((gaKeyerData.iFLAGS & FLAG_DISABLE_AUDIO) == 0) {
          toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, gaKeyerData.iSideToneFrequency);
        }
        Timer1.initialize(glTimer_Interval * 3);
      }
      if (gsMorse.length() != 0 ) {
        gsMorse = gsMorse.substring(1);
      }
    } else {
      glLastKeyElementSent = millis();
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

void debugMsg(String sMsg) {

  if (gaKeyerData.iFLAGS & FLAG_ENABLE_DEBUG) {
    Serial.println(sMsg);
    Serial.println(freeRam());
  }
}

/////////////////////////////////////////////////////////////////
// convertToMorse() takes a text string and converts it to a string of
// Morse code dots and dashes.
// INPUT: English text string
// OUPUT: Morse code string of dots and dashes
//
// Prosigns are implemented using the <> characters to bracket the letters. For example,
// the prosign <BT> is sent as "-...-"

String convertToMorse(String strText) {
  debugMsg ("Enter convertToMorse(" + strText + ")");

  String letter, sSpacer = " ", strInput, sResult;
  strInput = "";
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

    if (letter == '<') {
      sSpacer = "";
    }

    if (letter == ">" ) {
      strInput = strInput + " ";
      sSpacer = "";

    }


    if (letter == ' ') {
      sSpacer = " ";
      strInput = strInput + " ";
    }

    if ((letter >= 'A') && (letter <= 'Z')) {
      strInput.concat(letters[int(letter) - 65]);
      strInput.concat(sSpacer);
    }

    if ((letter >= '0') && (letter <= '9')) {
      strInput.concat(numbers[int(letter - 48)]);
      strInput.concat(sSpacer);

    }

  }

  return strInput;
}

// END OF TEXT STREAM MORSE CODE HANDLING
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// getInputLine() reads a serial port and accumulates a line of text. When a CR
// is received, the entire line is returned to the calling program.
String gsBuffer = "";

String getInputLine(void) {

  String sResult;

  while (Serial.available() > 0) {
    delay(10);
    //Serial.print(Serial.available());
    //toneAC2(2, 3, 650,50);
    char letter = Serial.read(); // read ONE character from the keyboard
    Serial.print(letter);
    //delay(500);
    //Serial.println(int(letter));
    //noInterrupts();
    if (letter == 13 ) { //look for CR key
      //Serial.println(strInput);
      Serial.println();
      sResult = gsBuffer;
      gsBuffer = "";
      return sResult;
      break;
    }
    gsBuffer = gsBuffer + letter;
  }

  return "";
}


/////////////////////////////////////////////////////////////////
// listSettings simply lists the internal variables along with the syntax
// needed to change the values.
void listSettings() {

  // list settings
  Serial.print(F("$CALLSIGN ")); Serial.println(gaKeyerData.callsign);
  Serial.println("$LM " + gsLastMessage);
  Serial.print("$KM "); Serial.println(gaKeyerData.bKeyMode);
  Serial.print("$PDT "); Serial.println(giPADDLE_DIT_TIME);
  Serial.print("$WPM "); Serial.println(1114239 / glTimer_Interval);
  Serial.print("$DBD "); Serial.println(gaKeyerData.bDebounceDelay);
  Serial.print("$STF "); Serial.println(gaKeyerData.iSideToneFrequency);
  Serial.print("$BI "); Serial.println(gaKeyerData.lBeaconIntervalSeconds);
  Serial.print("$BM "); Serial.println(gaKeyerData.sBeaconMessage);
  Serial.print("$SF "); Serial.println(gaKeyerData.iFLAGS, HEX);
  Serial.print(F("Free Memory: ")); Serial.println(freeRam());




}

void(* resetFunc) (void) = 0;//declare reset function at address 0


////////////////////////////////////////////////////////////////
// EEPROM ROUTINES



void listEEPROM() {
  int address = 0;
  byte value;

  for (int address = 0; address < sizeof(KeyerSetupData) ; address++ ) {
    // read a byte from the current address of the EEPROM
    value = EEPROM.read(address);

    Serial.print(address);
    Serial.print("\t");
    Serial.print(value, HEX);    Serial.print("\t"); Serial.print(value, DEC) ; Serial.print("\t"); Serial.print(char(value));
    Serial.println();


  }
}

void clearEEPROM() {

  int address = 0;
  byte value;

  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}




boolean processCommands(String strCmd) {
  strCmd.toUpperCase();
  if (strCmd.substring(0, 9) == "$CS" ) {
    Serial.print(F("Callsign was ")) ; Serial.println(gaKeyerData.callsign);
    strCmd.substring(9).toCharArray(gaKeyerData.callsign, 10);
    Serial.print(F("Callsign set to ")) ; Serial.println(gaKeyerData.callsign);


    return true;
  }

  if (strCmd.substring(0, 3) == "$SF") {  // readout the EEPROM


    gaKeyerData.iFLAGS = strCmd.substring(4).toInt();
    return true;
  }
  if (strCmd.substring(0, 3) == "$EL") {  // readout the EEPROM


    listEEPROM();
    return true;
  }
  if (strCmd.substring(0, 3) == "$EC") {  // readout the EEPROM


    clearEEPROM();
    return true;
  }

  if (strCmd.substring(0, 3) == "$EW") {  // readout the EEPROM

    EEPROM.put(0, gaKeyerData);
    listEEPROM();
    
    return true;
  }

  if (strCmd.substring(0, 3) == "$ER") {  // readout the EEPROM


    EEPROM.get(0, gaKeyerData);
    return true;
  }
  /*
    const int KEY_STRAIGHT = 0;
    const int KEY_PADDLE = 1;
    const int KEY_PADDLE_REVERSE = 2;
  */

  if (strCmd.substring(0, 3) == "$KM") {


    gaKeyerData.bKeyMode = strCmd.substring(3).toInt();
    Serial.print(F("Key mode set to ")); Serial.println(gaKeyerData.bKeyMode);

    return true;
  }


  if (strCmd.substring(0, 4) == "$PDT") {


    giPADDLE_DIT_TIME = strCmd.substring(4).toInt();
    Serial.print(F("Paddle dit time set to ")); Serial.println(giPADDLE_DIT_TIME);

    return true;
  }

  if (strCmd.substring(0, 3) == "$SM") {

    Serial.print(F("Morse conversion: ")); Serial.println(convertToMorse(strCmd.substring(4)));

    return true;
  }
  if (strCmd.substring(0, 3) == "$FR") {


    Serial.print("Free RAM: "); Serial.println(freeRam());


    return true;
  }
  if (strCmd.substring(0, 3) == "$CR") {


    Serial.println(F("Copyright information PENDING"));
    Serial.println(F("See <http://www.gnu.org/licenses/>"));

    return true;
  }


  if (strCmd.substring(0, 6) == "$RESET") {

    resetFunc(); //call reset
    return true;
  }



  if (strCmd.substring(0, 4) == "$WPM") {

    int iWPM ;
    iWPM = strCmd.substring(4).toInt();
    Serial.print(F("Keyer speed set to ")); Serial.println(iWPM);
    setWordsPerMinute(iWPM);
    return true;
  }

  if (strCmd.substring(0, 4) == "$DBD") {


    gaKeyerData.bDebounceDelay = strCmd.substring(4).toInt();
    Serial.print(F("Debounce delay set to ")); Serial.println(gaKeyerData.bDebounceDelay);

    return true;
  }
  if (strCmd.substring(0, 4) == "$STF") {


    gaKeyerData.iSideToneFrequency = strCmd.substring(4).toInt();
    Serial.print(F("Sidetone frequency set to ")); Serial.println(gaKeyerData.iSideToneFrequency);

    return true;
  }

  if (strCmd.substring(0, 3) == "$LM" ) {
    if (gsLastMessage.length() == 0) {
      Serial.println(F("No last message stored."));
      return true;
    }
    Serial.print(gsLastMessage);
    strCmd = gsLastMessage ;

  }
  if (strCmd.substring(0, 3) == "$LS" ) {
    listSettings();
    strCmd = "";
    return true;
  }
  if (strCmd.substring(0, 3) == "$BM" ) {
    strCmd.substring(4).toCharArray(gaKeyerData.sBeaconMessage, sizeof(gaKeyerData.sBeaconMessage));

    strCmd = "";
    return true;
  }

  if (strCmd.substring(0, 3) == "$BI" ) {
    gaKeyerData.lBeaconIntervalSeconds = strCmd.substring(4).toInt();

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
  //Serial.println(strCmd);
  String sTemp = convertToMorse(strCmd);
  Serial.println(sTemp);
  gsMorse = sTemp; // post the CW string to the buffer monitored by the IRQ.
  return false ;

}
////////////////////////////////////////////////////////////////////////
// Add a leading zero when necessary
// purloined from: http://www.technoblogy.com/list?1BHX
  void Print2 (int n) {
  if (n<10) Serial.print('0');
  Serial.print(n);
}

void printTime() {
  // Demonstrate clock
  unsigned long Now = millis()/1000;
  int Seconds = Now%60;
  int Minutes = (Now/60)%60;
  int Hours = (Now/3600)%24;


  Print2(Hours); Serial.print(':'); 
  Print2(Minutes); Serial.print(':'); 
  Print2(Seconds); Serial.println();
}
///////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
// runBeacon() will send a beacon message at intervals.
// SETTINGS:
// glBeaconIntervalSeconds -- number of seconds between beacons, set using $Bi
// gsBeaconMessage -- the string containing the beacon message, set using $BM

unsigned long glTimeSinceLastBeacon = 0;

void runBeacon() {
  if (gaKeyerData.lBeaconIntervalSeconds == 0) {
    glTimeSinceLastBeacon = 0;
    return;
  }
  if (gaKeyerData.sBeaconMessage[0] == 0) {
    return;
  }




  if ((millis() - glTimeSinceLastBeacon) >= (gaKeyerData.lBeaconIntervalSeconds * 1000)) {
    // time for the beacon
    glTimeSinceLastBeacon = millis();


    gsLastMessage = gaKeyerData.sBeaconMessage;
    //Serial.println(strCmd);
    String sTemp = convertToMorse(gaKeyerData.sBeaconMessage);
    if (sTemp.length() == 0) {
      debugMsg("sTemp was null in runBeacon()");
    }
//int Hours = glTimeSinceLastBeacon / (1000*60*60);
//int Minutes = (glTimeSinceLastBeacon % (1000*60*60)) / (1000*60);
//int Seconds = ((glTimeSinceLastBeacon % (1000*60*60)) % (1000*60)) / 1000;
Serial.print("BEACON: "); Serial.println(gaKeyerData.sBeaconMessage); Serial.println(sTemp);

    Serial.print("Interval: "); Serial.println(gaKeyerData.lBeaconIntervalSeconds);
   Serial.print("Event time (minutes): "); printTime();
    Serial.print("Free RAM: "); Serial.println(freeRam());


    gsMorse = sTemp; // post the CW string to the buffer monitored by the IRQ.
    Timer1.initialize(glTimer_Interval);
    Timer1.attachInterrupt(stateMachine); // stateMachine to run every ditTime


  }


}

void blinkLED(int iTimeMilliseconds) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(iTimeMilliseconds);
  digitalWrite(LED_BUILTIN, LOW);

}
unsigned long giLastHeartBeat = 0;
const int iHeartBeatInterval = 5000;
void runHeartBeat() {
  if ((millis() - glLastKeyElementSent) <= 10000 ) {
    return ;
  }
  if (gaKeyerData.iFLAGS & FLAG_ENABLE_HEARTBEAT) {

    if ((millis() - giLastHeartBeat) >= iHeartBeatInterval) {
      giLastHeartBeat = millis();
      //noInterrupts();
      blinkLED(150);

      //interrupts();
    }

  }



}

//////////////////////////////////////////////////////////////
// loop() is the "main" entry point for Arduino code. All roads lead from here.

void loop() {

  runHeartBeat();

  runBeacon(); // if beacon is setup, run the beacon

#if defined(INCLUDE_HAND_KEY)
  processKeyers(); // Poll the key to see what is needed
#endif


  String strInput;

  strInput = getInputLine(); // fetch a line from the console


  if (strInput.length()) { // if a string exists, start the timers and prepare to generate Morse code.
    Timer1.initialize(glTimer_Interval);
    Timer1.attachInterrupt(stateMachine); // stateMachine to run every ditTime


    if (processCommands(strInput) == false) {

    }

  }
  strInput = "";

}

