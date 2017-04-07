


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


/////////////////////////////////////////////////////////////
// CODE PRACTICE ROUTINES
// getRandomString() returns a randomly selected string from this lookup table.
// input: cResultArray[] - a character array large enough to hold the largest string in the list
// Returns cResultArray[] with the chosen string

const char Words1[] PROGMEM  = {"go am me on by to up so it no of as he if an us or in is at my we do be and man him out not but can who \
has may was one she all you how any its say are now two for men her had the our his been some then like well made when have only your \
work over such time were with into very what then more will they come that from must said them this upon great \
about other shall every these first their could which would there before should little people \
<AA> <AB> ABT ADR AGN ANT BN CFM <CL> CQ ES FB GA GM GE GN HI HR MSG NCS NR OM OP PSE PWR R RCD RX \
RTTY SKED SSB T TFC TX UR WA WB WDS WKD XYL YL 73 88 <SK> <KN> ARRL \
QRA QRI QRJ QRK QRL QRM QRN QRO QRP QRQ QRS QRT QRU QRV QRW QRX QRZ QSA QSB QSD QSG QSK QSL QSM QSN \
QSO QSP QSU QSV QSW QSY QSZ QTA QTB QTC QTH QTX QUB QUC QUD 0 1 2 3 4 5 6 7 8 9 ? , . <ar> <bt> <bk> <as> RST 5NN 599 \
TU TNX SRI BTU CUL WX " 
                               };

void getRandomString(char * cResultArray) {

  int nSize = sizeof(Words1);
  int iStart, iEnd;
  int iIndex = random(0, nSize - 2) ;// <<<< HACK HERE TO FORCE PROPER END OF ARRAY HANDLING // make a random byte pointer into the array



  for (int i = iIndex; i >= 0 ; i-- ) { // find the nearest space below the pointer
    char c = pgm_read_byte_near(Words1 + i);
    if (c == ' ') { // pointing at a space
      iStart = i + 1; // point it to the first chr of next word

      break;
    }
    if (i == 0) {
      iStart = 0; break;
    }
  }
  for (int i = iStart; i <= nSize; i++) { // walk up the string looking for a space
    char c = pgm_read_byte_near(Words1 + i);
    if (c == ' ') { // pointing at next space
      iEnd = i - 1;
      break;
    }
  }
  //Copy the word into the cResultArray
  memcpy_P(cResultArray, Words1 + iStart, iEnd - iStart + 1);

}



/*

  QRA  What is the name of your station? The name of my station is ___.
  QRB How far are you from my station? I am ____ km from you station
  QRD Where are you bound and where are you coming from? I am bound ___ from ___.
  QRG Will you tell me my exact frequency? Your exact frequency is ___ kHz.
  QRH Does my frequency vary? Your frequency varies.
  QRI How is the tone of my transmission? The tone of your transmission is ___ (1-Good, 2-Variable, 3-Bad.)
  QRJ Are you receiving me badly? I cannot receive you, your signal is too weak.
  QRK What is the intelligibility of my signals? The intelligibility of your signals is ___ (1-Bad, 2-Poor, 3-Fair, 4-Good, 5-Excellent.)
  QRL Are you busy? I am busy, please do not interfere
  QRM Is my transmission being interfered with? Your transmission is being interfered with ___ (1-Nil, 2-Slightly, 3-Moderately, 4-Severly, 5-Extremely.)
  QRN Are you troubled by static? I am troubled by static ___ (1-5 as under QRM.)
  QRO Shall I increase power? Increase power.
  QRP Shall I decrease power? Decrease power.
  QRQ Shall I send faster? Send faster (___ WPM.)
  QRR Are you ready for automatic operation? I am ready for automatic operation. Send at ___ WPM.
  QRS Shall I send more slowly? Send more slowly (___ WPM.)
  QRT Shall I stop sending? Stop sending.
  QRU Have you anything for me? I have nothing for you.
  QRV Are you ready? I am ready.
  QRW Shall I inform ___ that you are calling? Please inform ___ that I am calling.
  QRX When will you call me again? I will call you again at ___ hours.
  QRY What is my turn? Your turn is numbered ___.
  QRZ Who is calling me? You are being called by ___.
  QSA What is the strength of my signals? The strength of your signals is ___ (1-Scarcely perceptible, 2-Weak, 3-Fairly Good, 4-Good, 5-Very Good.)
  QSB Are my signals fading? Your signals are fading.
  QSD Is my keying defective? Your keying is defective.
  QSG Shall I send ___ messages at a time? Send ___ messages at a time.
  QSJ What is the charge to be collected per word to ___ including your international telegraph charge? The charge to be collected per word is ___ including my international telegraph charge.
  QSK Can you hear me between you signals and if so can I break in on your transmission? I can hear you between my signals, break in on my transmission.
  QSL Can you acknowledge receipt? I am acknowledging receipt.
  QSM Shall I repeate the last message which I sent you? Repeat the last message.
  QSN Did you hear me on ___ kHz? I did hear you on ___ kHz.
  QSO Can you communicate with ___ direct or by relay? I can communicate with ___ direct (or by relay through ___.)
  QSP Will you relay to ___? I will relay to ___.
  QSQ Have you a doctor on board? (or is ___ on board?) I have a doctor on board (or ___ is on board.)
  QSU Shall I send or reply on this frequency? Send a series of Vs on this frequency.
  QSV Shall I send a series of Vs on this frequency? Send a series of Vs on this frequency.
  QSW Will you send on this frequency? I am going to send on this frequency.
  QSY Shall I change to another frequency? Change to another frequency.
  QSZ Shall I send each word or group more than once? Send each word or group twice (or ___ times.)
  QTA Shall I cancel message number ___? Cancel message number ___.
  QTB Do you agree with my counting of words? I do not agree with your counting of words. I will repeat the first letter or digit of each word or group.
  QTC How many messages have you to send? I have ___ messages for you.
  QTE What is my true bearing from you? Your true bearing from me is ___ degrees.
  QTG Will you send two dashes of 10 seconds each followed by your call sign? I am going to send two dashes of 10 seconds each followed by my call sign.
  QTH What is your location? My location is ___.
  QTI What is your true track? My true track is ___ degrees.
  QTJ What is your speed? My speed is ___ km/h.
  QTL What is your true heading? My true heading is ___ degrees.
  QTN At what time did you depart from ___? I departed from ___ at ___ hours.
  QTO Have you left dock (or port)? I have left dock (or port).
  QTP Are you going to enter dock (or port)? I am goin gto enter dock (or port.)
  QTQ Can you communicate with my station by meains of the International Code of Signals? I am going to communicate with your staion by means of the International Code of Signals.
  QTR What is the correct time? The time is ___.
  QTS Will you send your call sign for ___ minutes so that your frequency can be measured? I will send my call sign for ___ minutes so that my frequency may be measured.
  QTU What are the hours during which your station is open? My station is open from ___ hours to ___ hours.
  QTV Shall I stand guard for you on the frequency of ___ kHz? Stand guard for me on the frequency of ___ kHz.
  QTX Will you keep your station open for further communication with me? I will keep my station open for further communication with you.
  QUA Have you news of ___? I have news of ___.
  QUB Can you give me information concering visibility, height of cluds, direction and velocity of ground wind at ___? Here is the information you requested...
  QUC What is the number of the last message you received from me? The number of the last message I received from you is ___.
  QUD Have you received the urgency signal sent by ___? I have received the urgency signal sent by ___.
  QUF Have you received the distress signal sent by ___? I have received the distress signal sent by ___.
  QUG Will you be forced to land? I am forced to land immediately.
  QUH Will you give me the present barometric pressure? The present barometric pressure is ___ (units).

*/


