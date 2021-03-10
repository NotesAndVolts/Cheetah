// ************************************************************************
// CHEETAH - Midi keyboard helper
// For Arduino NANO
// by Notes and Volts http://www.notesandvolts.com
//
// ** Requires Arduino MIDI Library v4.2 or later **
// ************************************************************************
// Version Alpha 0.1 - Buttons
// ************************************************************************

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

//Globals
const byte numLicks = 3; //Number of Licks in array
byte currentLick = 0;
byte notePos = 0;
byte lastLick = 255;
int pedalValue = 0;
byte keysDown = 0;
byte lowNote = 255;

//Licks - end Lick with 255
byte licks[numLicks][17] = {
  {15, 12, 10, 7, 12, 10, 7, 5, 10, 7, 5, 3, 7, 5, 3, 0, 255},
  {0, 5, 3, 7, 5, 10, 7, 12, 255},
  {0, 3, 5, 7, 10, 12, 7, 10, 5, 7, 3, 5, 0, 3, 255}
};



void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(HandleNoteOn);
  MIDI.setHandleNoteOff(HandleNoteOff);
  MIDI.setHandlePitchBend(HandlePitchBend);
  MIDI.setHandleControlChange(HandleCC);
}

void loop() {
  MIDI.read();
}

void HandleNoteOn(byte channel, byte pitch, byte velocity) {
  keysDown++;
  if (pitch < lowNote) lowNote = pitch;
  MIDI.sendNoteOff(lastLick, 0, channel);
  lastLick = lowNote + licks[currentLick][notePos];
  MIDI.sendNoteOn(lastLick, velocity, channel);
  notePos = notePos + 1;
  if (licks[currentLick][notePos] == 255) {
    notePos = 0;
    currentLick++;
    if ((currentLick) >= numLicks) currentLick = 0;
  }
}

void HandleNoteOff(byte channel, byte pitch, byte velocity) {
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

