#include <MemoryFree.h>



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


// INCLUDE FILES FROM ARDUINO LIBRARY
#include <EEPROM.h>
// See http://playground.arduino.cc/Code/Timer1
#include <TimerOne.h>
// See https://bitbucket.org/teckel12/arduino-toneac2/wiki/Home
#include <toneAC2.h>


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
  13, // wpm
  "W7YV",
  "DE W7YV <AR>"
};

/////////////////////////////////////////////////////////////////
void setup() {
  // initialize digital pins


  pinMode(LED_BUILTIN, OUTPUT); // Blinks when morse is sending or when key is down
  pinMode(giPinKEY_LEFT, INPUT_PULLUP);
  pinMode(giPinKEY_RIGHT, INPUT_PULLUP);
  setWordsPerMinute(18);  // set default TODO: implement EEPROM storage of "last data"

  // Brag stuff
  Serial.begin(9600);
  Serial.println(Copyright);
  Serial.println("Type $CR for additional copyright information.");
  Serial.print("EEPROM size: "); Serial.println(EEPROM.length());
  Serial.print("Free RAM: "); Serial.println(freeRam());
  debugMsg("Debug is enabled");

  // read EEPROM signature
  KeyerSetupData dataEEPROM;
  EEPROM.get(0, dataEEPROM);
  if (dataEEPROM.ulSignature != gaKeyerData.ulSignature) {
    EEPROM.put(0, gaKeyerData);
    Serial.println("Initializing EEPROM");
  } else
  { // load EEPROM to memory
    Serial.println("Loading EEPROM settings");
    gaKeyerData = dataEEPROM;
  }
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
  // 62500 = PARIS in 3.37 seconds = 17.804 wpm -- 1112750
  // 74184 = PARIS in 3.99 seconds = 15.04 wpm -- 1115727
  // avergage resultant equation: glTimer_Interval = 1114239 / wpm
  const unsigned long lTimerFactor = 1114239; // *WARNING* Processor dependant
  const int lPaddleTimeFactor = 1200;
  glTimer_Interval = lTimerFactor / wpm ;

  giPADDLE_DIT_TIME = lPaddleTimeFactor / wpm;

}
#include "handkey.h"
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
  const int FLAG_ENABLE_DEBUG = 1;
  const int FLAG_ENABLE_HEARTBEAT = 2;
  if (gaKeyerData.iFLAGS & FLAG_ENABLE_HEARTBEAT) {
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
  Serial.print("$CALLSIGN "); Serial.println(gaKeyerData.callsign);
  Serial.println("$LM " + gsLastMessage);
  Serial.print("$KM "); Serial.println(gaKeyerData.bKeyMode);
  Serial.print("$PDT "); Serial.println(giPADDLE_DIT_TIME);
  Serial.print("$WPM "); Serial.println(1114239 / glTimer_Interval);
  Serial.print("$DBD "); Serial.println(gaKeyerData.bDebounceDelay);
  Serial.print("$STF "); Serial.println(gaKeyerData.iSideToneFrequency);
  Serial.print("$BI "); Serial.println(gaKeyerData.lBeaconIntervalSeconds);
  Serial.print("$BM "); Serial.println(gaKeyerData.sBeaconMessage);
  Serial.print("$SF "); Serial.println(gaKeyerData.iFLAGS, HEX);
  Serial.print("Free Memory: "); Serial.println(freeRam());




}

void(* resetFunc) (void) = 0;//declare reset function at address 0


////////////////////////////////////////////////////////////////
// EEPROM ROUTINES



/*

  unsigned long eeprom_crc(void) {
  //    Written by Christopher Andrews.
  //    CRC algorithm generated by pycrc, MIT licence ( https://github.com/tpircher/pycrc ).
  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = 0 ; index < EEPROM.length()  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
  }
*/
void listEEPROM() {
  int address = 0;
  byte value;

  for (int address = 0; address < 1024 ; address++ ) {
    // read a byte from the current address of the EEPROM
    value = EEPROM.read(address);

    Serial.print(address);
    Serial.print("\t");
    Serial.print(value, HEX);
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
    Serial.print("Callsign was ") ; Serial.println(gaKeyerData.callsign);
    strCmd.substring(9).toCharArray(gaKeyerData.callsign, 10);
    Serial.print("Callsign set to ") ; Serial.println(gaKeyerData.callsign);


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
    Serial.print("Key mode set to "); Serial.println(gaKeyerData.bKeyMode);

    return true;
  }


  if (strCmd.substring(0, 4) == "$PDT") {


    giPADDLE_DIT_TIME = strCmd.substring(4).toInt();
    Serial.print("Paddle dit time set to "); Serial.println(giPADDLE_DIT_TIME);

    return true;
  }

  if (strCmd.substring(0, 3) == "$SM") {

    Serial.print("Morse conversion: "); Serial.println(convertToMorse(strCmd.substring(4)));

    return true;
  }
  if (strCmd.substring(0, 3) == "$FR") {


    Serial.print("Free RAM: "); Serial.println(freeRam());


    return true;
  }
  if (strCmd.substring(0, 3) == "$CR") {


    Serial.println("Copyright information PENDING");
    Serial.println("See <http://www.gnu.org/licenses/>");

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


    gaKeyerData.bDebounceDelay = strCmd.substring(4).toInt();
    Serial.print("Debounce delay set to "); Serial.println(gaKeyerData.bDebounceDelay);

    return true;
  }
  if (strCmd.substring(0, 4) == "$STF") {


    gaKeyerData.iSideToneFrequency = strCmd.substring(4).toInt();
    Serial.print("Sidetone frequency set to "); Serial.println(gaKeyerData.iSideToneFrequency);

    return true;
  }

  if (strCmd.substring(0, 3) == "$LM" ) {
    if (gsLastMessage.length() == 0) {
      Serial.println("No last message stored.");
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



//////////////////////////////////////////////////////////////
// runBeacon() will send a beacon message at intervals.
// SETTINGS:
// glBeaconIntervalSeconds -- number of seconds between beacons, set using $Bi
// gsBeaconMessage -- the string containing the beacon message, set using $BM

long glTimeSinceLastBeacon = 0;

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
    Serial.print("BEACON: "); Serial.println(gaKeyerData.sBeaconMessage); Serial.println(sTemp);
    Serial.print("Interval: "); Serial.println(gaKeyerData.lBeaconIntervalSeconds, DEC);
    Serial.print("Event time: "); Serial.println(glTimeSinceLastBeacon, HEX);
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

  processKeyers(); // Poll the key to see what is needed


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

