



/////////////////////////////////////////////////////////////////
// Called from loop() to poll mechanical keys connected to the board



/////////////////////////////////////////////////////////////////
// This section contains the routines for the Iambic Keyer, i.e., a dual
// paddle key that produces a stream of dits and dashes. The routines have the
// following functions:
// paddleKeyDitStateMachine() handles the "dit" side of the Iambic key.
// paddleKeyDashStateMahcine() handles the "dash" side of the Iambic key.
// paddleKeyStateMachine() handles the Iambic Key function.




int paddleKeyDitStateMachine() { // handles only the DIT side of the key
  switch (giKeyDitState) {

    case 0: // look for key down
      if (digitalRead(giPinKEY_DIT) == LOW)

      {
        // key down so debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < gaKeyerData.bDebounceDelay) {
          //delay(1);
        }
        // if still down, call it good
        if (digitalRead(giPinKEY_DIT) == LOW) {
          giKeyDitState = 1;
        }
      }
      break;

    case 1: // confirmed: dit went down, so light up the speaker
      glKeyChangeTime = millis();
      digitalWrite(LED_BUILTIN, HIGH);
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, gaKeyerData.iSideToneFrequency);
      giKeyDitState = 2; //now wait the dit time
      break;

    case 2: // wait for dit time
      while ((millis() - glKeyChangeTime) < giPADDLE_DIT_TIME) {
        //wait for key dit time, the kill the tone
      }
      giKeyDitState = 3;
      break;

    case 3: // kill the tone and wait for interdit time
      digitalWrite(LED_BUILTIN, LOW);
      noToneAC2(); // kill the tone
      while ((millis() - glKeyChangeTime) < giPADDLE_DIT_TIME * 2) {
        // wait one element time (same as dit time)
      }

      giKeyDitState = 4;
      break;

    case 4: // dit is down, interdit is done, has key been released?

      if (digitalRead(giPinKEY_DIT) == HIGH) // poll the key
      {
        // TODO: Think about this! Should we bail before dit time is done?
        // debounce
        glKeyChangeTime = millis();
        while ((millis() - glKeyChangeTime) < gaKeyerData.bDebounceDelay) {

        }
        if (digitalRead(giPinKEY_DIT) == HIGH) {
          // OOPS, key is up. get out of here
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
      if (digitalRead(giPinKEY_DASH) == LOW)

      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < gaKeyerData.bDebounceDelay) {
          delay(1);
        }
        if (digitalRead(giPinKEY_DASH) == LOW) {
          giKeyDashState = 1;
        }
      }
      break;

    case 1: // confirmed: dash went down
      glKeyChangeTime = millis();
      digitalWrite(LED_BUILTIN, HIGH);
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, gaKeyerData.iSideToneFrequency);
      giKeyDashState = 2; //now wait the dash time
      break;

    case 2: // wait for dash time
      while ((millis() - glKeyChangeTime) < (giPADDLE_DIT_TIME * 3)) {
        delay(1);
      }
      giKeyDashState = 3;
      break;
    case 3: // kill the tone and wait for element spacing time
      digitalWrite(LED_BUILTIN, LOW);
      noToneAC2();
      while ((millis() - glKeyChangeTime) < (giPADDLE_DIT_TIME * 4)) {
        //delay(1);
        if (digitalRead(giPinKEY_DASH) == HIGH) {
          //giKeyDashState = 0;
          // break;
        }
      }

      giKeyDashState = 4;
      break;

    case 4: // dash is down, element spacing time is done, has key been released?
      if (digitalRead(giPinKEY_DASH) == HIGH)
      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < gaKeyerData.bDebounceDelay) {
          //delay(1);
        }
        if (digitalRead(giPinKEY_DASH) == HIGH) {

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

  while (paddleKeyDitStateMachine() != 0) {
    glLastKeyElementSent = millis(); } // this alerts other routines that a key dit or dash was sent} ;
  while (paddleKeyDashStateMachine() != 0 ) {
    glLastKeyElementSent = millis(); };

}
// END OF IAMBIC KEY HANDLING
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// This section contains the routines for the handling a straight key.

void straightKeyStateMachine() {
  switch (giKeyState) {

    case 0: // nothing
      if (digitalRead(giPinKEY_LEFT) == LOW)

      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < gaKeyerData.bDebounceDelay) {
          delay(1);
        }
        if (digitalRead(giPinKEY_LEFT) == LOW) {
          giKeyState = 1;
        }
      }
      break;

    case 1: // dit went down
      glKeyChangeTime = millis();
      toneAC2(pin_SPEAKER_RED, pin_SPEAKER_BLACK, gaKeyerData.iSideToneFrequency);
      giKeyState = 2;
      break;
    case 2: // dit is down, wait for dit time,
      if (digitalRead(giPinKEY_LEFT) == HIGH)
      {
        // debounce
        glKeyChangeTime = millis();
        //delay(5);
        while ((millis() - glKeyChangeTime) < gaKeyerData.bDebounceDelay) {
          delay(1);
        }
        if (digitalRead(giPinKEY_LEFT) == HIGH) {
          noToneAC2();
          giKeyState = 0;
        }
      }

      break;

  }
}

// END OF STRAIGHT KEY HANDLING
/////////////////////////////////////////////////////////////////


void processKeyers() {
  switch (gaKeyerData.bKeyMode) {
    case KEY_STRAIGHT:
      giPinKEY_DIT = giPinKEY_LEFT;
      giPinKEY_DASH = giPinKEY_RIGHT;
      straightKeyStateMachine();
      break;
    case KEY_PADDLE:
      giPinKEY_DIT = giPinKEY_LEFT;
      giPinKEY_DASH = giPinKEY_RIGHT;
      paddleKeyStateMachine();
      break;
    case KEY_PADDLE_REVERSE:
      giPinKEY_DIT = giPinKEY_RIGHT;
      giPinKEY_DASH = giPinKEY_LEFT;
      paddleKeyStateMachine();
      break;
}

  }

