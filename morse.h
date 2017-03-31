/*
const struct morse_character {
  char morse[6];
  char letter;
  
};
morse_characters
struct KeyerSetupData {
  int iSideToneFrequency;
  byte bKeyMode;
  byte bDebounceDelay;
  int iBeaconIntervalSeconds;
  byte byWPM;
  char callsign[12];
  char sBeaconMessage[60];
};

KeyerSetupData gaKeyerData = {
  650, // side tone
  1, // key mode
  5, // debounce delay
  0, // beacon interval
  13, // wpm
  "NOCALLSIGN",
  "test"
};

*/
const char * letters [26] = {
  ".-",     // A
  "-...",   // B
  "-.-.",   // C
  "-..",    // D
  ".",      // E
  "..-.",   // F
  "--.",    // G
  "....",   // H
  "..",     // I
  ".---",   // J
  "-.-",    // K
  ".-..",   // L
  "--",     // M
  "-.",     // N
  "---",    // O
  ".--.",   // P
  "--.-",   // Q
  ".-.",    // R
  "...",    // S
  "-",      // T
  "..-",    // U
  "...-",   // V
  ".--",    // W
  "-..-",   // X
  "-.--",   // Y
  "--.."    // Z
};

const char * numbers [10] =
{
  "-----",  // 0
  ".----",  // 1
  "..---",  // 2
  "...--",  // 3
  "....-",  // 4
  ".....",  // 5
  "-....",  // 6
  "--...",  // 7
  "---..",  // 8
  "----.",  // 9
};



