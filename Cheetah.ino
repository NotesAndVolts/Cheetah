// ************************************************************************
// CHEETAH - Midi keyboard helper
// For Arduino NANO
// by Notes and Volts http://www.notesandvolts.com
//
// ** Requires Arduino MIDI Library v4.2 or later **
// ************************************************************************
// Version Alpha 0.2.1
// ************************************************************************

#include <Bounce2.h>
#include <MIDI.h>

#define LED 13
#define MAJ_PIN 4
#define MIN_PIN 3
#define DOM_PIN 2

// Instantiate Bounce object
Bounce Maj = Bounce();
Bounce Min = Bounce();
Bounce Dom = Bounce();

MIDI_CREATE_DEFAULT_INSTANCE();

//Globals
const byte numLicks = 3; //Number of Licks in array
byte currentLick = 0;
byte notePos = 0;
byte lastLick = 255;
int pedalValue = HIGH;
byte keysDown = 0;
byte lowNote = 255;

byte scaleType = 1;

//Licks - end Lick with 255
byte minLicks[numLicks][17] = {
  {15, 12, 10, 7, 12, 10, 7, 5, 10, 7, 5, 3, 7, 5, 3, 0, 255},
  {0, 5, 3, 7, 5, 10, 7, 12, 255},
  {0, 3, 5, 7, 10, 12, 7, 10, 5, 7, 3, 5, 0, 3, 255}
};

byte majLicks[numLicks][17] = {
  {0, 2, 3, 4, 7, 9, 7, 12, 255},
  {12, 9, 7, 4, 3, 2, 0, 2, 0, 255},
  {14, 15, 16, 19, 16, 15, 14, 12, 9, 7, 12, 255}
};

byte domLicks[numLicks][17] = {
  {7, 10, 14, 17, 14, 15, 16, 12, 14, 10, 9, 7, 255},
  {12, 9, 11, 9, 10, 2, 5, 9, 255},
  {16, 12, 10, 7, 14, 10, 7, 5, 4, 12, 255}
};



void setup() {
  pinMode(LED, OUTPUT);
  Maj.attach(MAJ_PIN, INPUT_PULLUP);
  Maj.interval(5);
  Min.attach(MIN_PIN, INPUT_PULLUP);
  Min.interval(5);
  Dom.attach(DOM_PIN, INPUT_PULLUP);
  Dom.interval(5);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(HandleNoteOn);
  MIDI.setHandleNoteOff(HandleNoteOff);
  MIDI.setHandlePitchBend(HandlePitchBend);
  MIDI.setHandleControlChange(HandleCC);
}

void loop() {
  MIDI.read();
  buttonUpdate();
}

void buttonUpdate() {
  int buttonValue = 0;

  Maj.update();
  if ( Maj.changed() ) {
    // THE STATE OF THE INPUT CHANGED
    buttonValue = Maj.read();

    // DO SOMETHING WITH THE VALUE
    if (buttonValue == LOW) {
      digitalWrite(LED, HIGH );
      noteBuffer(255, true);
      scaleType = 2;
      pedalValue = LOW;
    }
    else {
      digitalWrite(LED, LOW );
      MIDI.sendNoteOff(lastLick, 0, 1);//
      pedalValue = HIGH;
    }
  }

  Min.update();
  if ( Min.changed() ) {
    // THE STATE OF THE INPUT CHANGED
    buttonValue = Min.read();

    // DO SOMETHING WITH THE VALUE
    if (buttonValue == LOW) {
      digitalWrite(LED, HIGH );
      noteBuffer(255, true);
      scaleType = 1;
      pedalValue = LOW;
    }
    else {
      digitalWrite(LED, LOW );
      MIDI.sendNoteOff(lastLick, 0, 1);//
      pedalValue = HIGH;
    }
  }

  Dom.update();
  if ( Dom.changed() ) {
    // THE STATE OF THE INPUT CHANGED
    buttonValue = Dom.read();

    // DO SOMETHING WITH THE VALUE
    if (buttonValue == LOW) {
      digitalWrite(LED, HIGH );
      noteBuffer(255, true);
      scaleType = 3;
      pedalValue = LOW;
    }
    else {
      digitalWrite(LED, LOW );
      MIDI.sendNoteOff(lastLick, 0, 1);//
      pedalValue = HIGH;
    }
  }

}//End buttonUpdate

void HandleNoteOn(byte channel, byte pitch, byte velocity) {
  if (pedalValue == HIGH) {
    if (noteBuffer(pitch, true)) MIDI.sendNoteOn(pitch, velocity, channel);
    return;
  }

  keysDown++;
  if (pitch < lowNote) lowNote = pitch;
  MIDI.sendNoteOff(lastLick, 0, channel);
  switch (scaleType) {
    case 1:
      lastLick = lowNote + minLicks[currentLick][notePos];
      MIDI.sendNoteOn(lastLick, velocity, channel);
      notePos = notePos + 1;
      if (minLicks[currentLick][notePos] == 255) {
        notePos = 0;
        currentLick++;
        if ((currentLick) >= numLicks) currentLick = 0;
      }
      break;
    case 2:
      lastLick = lowNote + majLicks[currentLick][notePos];
      MIDI.sendNoteOn(lastLick, velocity, channel);
      notePos = notePos + 1;
      if (majLicks[currentLick][notePos] == 255) {
        notePos = 0;
        currentLick++;
        if ((currentLick) >= numLicks) currentLick = 0;
      }
      break;
    case 3:
      lastLick = lowNote + domLicks[currentLick][notePos];
      MIDI.sendNoteOn(lastLick, velocity, channel);
      notePos = notePos + 1;
      if (domLicks[currentLick][notePos] == 255) {
        notePos = 0;
        currentLick++;
        if ((currentLick) >= numLicks) currentLick = 0;
      }
      break;
  }
}

void HandleNoteOff(byte channel, byte pitch, byte velocity) {
  if (pedalValue == HIGH) {
    noteBuffer(pitch, false);
    MIDI.sendNoteOff(pitch, velocity, channel);
    return;
  }

  if (keysDown >= 1) keysDown--;
  if (keysDown == 0) {
    MIDI.sendNoteOff(lastLick, velocity, channel);
    lowNote = 255;
  }
}

void HandlePitchBend(byte channel,  int bend) {
  MIDI.sendPitchBend(bend, channel);
}

void HandleCC (byte channel, byte number, byte value) {
  MIDI.sendControlChange(number, value, channel);
}

bool noteBuffer(byte note, bool noteIn) {
  //note = note number
  //noteIn: TRUE for NOTE IN, FALSE for NOTE OUT
  //If note = 255 - purge buffer and send note OFFs
  //Function returns FALSE if Buffer is FULL
  const byte bufferSize = 10;
  static byte howFull = 0;
  static byte noteBuff[bufferSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // Purge and send OFFs
  if (note == 255) {
    for (int i = 0; i < bufferSize; i++) {
      if (noteBuff[i] > 0) {
        MIDI.sendNoteOff(noteBuff[i], 0, 1);
        noteBuff[i] = 0;
      }
    }
    howFull = 0;
    MIDI.sendSysEx(bufferSize, noteBuff, false);
    return true;
  }

  // Note IN
  if (noteIn == true && howFull < bufferSize) {
    for (int i = 0; i < bufferSize; i++) {
      if (noteBuff[i] == 0) {
        noteBuff[i] = note;
        howFull++;
        MIDI.sendSysEx(bufferSize, noteBuff, false);
        return true;
      }
    }
  }

  // Note OUT
  else if (noteIn == false) {
    for (int i = 0; i < bufferSize; i++) {
      if (noteBuff[i] == note) {
        noteBuff[i] = 0;
        howFull--;
        MIDI.sendSysEx(bufferSize, noteBuff, false);
        return true;
      }
    }
  }

  // Buffer FULL
  else if (howFull >= bufferSize) {
    return false;
  }
} //end noteBuffer
