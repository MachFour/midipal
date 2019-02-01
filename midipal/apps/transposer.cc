// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
//
// Transposer app.

#include "midipal/apps/transposer.h"

#include "midipal/display.h"

#include "midipal/notes.h"
#include "midipal/ui.h"

#include "midi/midi_constants.h"

namespace midipal {
namespace apps{

using namespace avrlib;

/* static */
const uint8_t Transposer::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0
};

/* static */
const AppInfo Transposer::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  &OnNoteOff, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  &OnNoteAftertouch, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnAftertouch)(uint8_t, uint8_t);
  nullptr, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnProgramChange)(uint8_t, uint8_t);
  nullptr, // void (*OnPitchBend)(uint8_t, uint16_t);
  nullptr, // void (*OnSysExByte)(uint8_t);
  nullptr, // void (*OnClock)();
  nullptr, // void (*OnStart)();
  nullptr, // void (*OnContinue)();
  nullptr, // void (*OnStop)();
  nullptr, // bool *(CheckChannel)(uint8_t);
  nullptr, // void (*OnRawByte)(uint8_t);
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_SCALE_PROCESSOR, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_SCALE, // app_name
  true
};

/* static */
void Transposer::OnInit() {
  Ui::AddPage(STR_RES_CHN, UNIT_INTEGER_ALL, 0, 16);
  Ui::AddPage(STR_RES_TRS, UNIT_SIGNED_INTEGER, -24, 24);
}

bool Transposer::isActiveChannel(uint8_t channel) {
  return Transposer::channel() == 0 || Transposer::channel() == channel + 1;
}

uint8_t Transposer::transposedNote(uint8_t note) {
  return Transpose(note, transposition());
}

/* static */
void Transposer::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  const auto type = byteAnd(status, 0xf0);
  const auto channel = byteAnd(status, 0x0f);
  bool is_note = type == MIDI_NOTE_OFF || type == MIDI_NOTE_ON || type == MIDI_POLY_AFTERTOUCH;
  if (!is_note || !isActiveChannel(channel)) {
    App::Send(status, data, data_size);
  }
}

/* static */
void Transposer::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (isActiveChannel(channel)) {
    App::Send3(noteOnFor(channel), transposedNote(note), velocity);
  }
}

/* static */
void Transposer::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (isActiveChannel(channel)) {
    App::Send3(noteOffFor(channel), transposedNote(note), velocity);
  }
}

/* static */
void Transposer::OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (isActiveChannel(channel)) {
    App::Send3(polyAftertouchFor(channel), transposedNote(note), velocity);
  }
}

} // namespace apps
} // namespace midipal
