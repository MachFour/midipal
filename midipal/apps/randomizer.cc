// Copyright 2011 Olivier Gillet.
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
// Randomizer app.

#include "midipal/apps/randomizer.h"

#include "avrlib/random.h"
#include "midi/midi_constants.h"

#include "midi/midi.h"

#include "midipal/notes.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t Randomizer::factory_data[Parameter::COUNT] PROGMEM = {
  0, 127, 12, 32, 0, 0, 7, 10
};

/* static */
uint8_t Randomizer::settings[Parameter::COUNT];

/* static */
const AppInfo Randomizer::app_info_ PROGMEM = {
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
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_RANDOMIZER, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_RANDOMIZ, // app_name
  true
};

/* static */
void Randomizer::OnInit() {
  Ui::AddPage(STR_RES_CHN, UNIT_INTEGER_ALL, 0, 16);
  Ui::AddPage(STR_RES_AMT, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_NOT, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_VEL, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_CC1, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_CC2, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES__C1, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES__C2, UNIT_INTEGER, 0, 127);
}

inline bool shouldForwardData(uint8_t status) {
  uint8_t type = byteAnd(status, 0xf0);
  return !(type == MIDI_NOTE_OFF || type == MIDI_NOTE_ON
      || type == MIDI_POLY_AFTERTOUCH);
}

/* static */
void Randomizer::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  if (shouldForwardData(status)) {
    App::Send(status, data, data_size);
  }
}

/* static */
uint8_t Randomizer::ScaleModulationAmount(uint8_t amount) {
  if (Random::GetByte() < (global_amount() << 1u)) {
    return amount << 1u;
  } else {
    return 0;
  }
}

static uint8_t randomU7() {
  return U7(Random::GetByte());
}

bool Randomizer::isActiveChannel(uint8_t channel) {
  // channel() == 0 means all channels are active
  return Randomizer::channel() == 0 || Randomizer::channel() == channel + 1;

}

/* static */
void Randomizer::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isActiveChannel(channel)) {
    App::Send3(noteOnFor(channel), note, velocity);
  } else {
    // Send random CCs before the note
    for (uint8_t i = 0; i < 2; ++i) {
      if (cc_amount()[i] && global_amount()) {
        uint8_t value = U8Mix(63, randomU7(), ScaleModulationAmount(cc_amount()[i]));
        App::Send3(controlChangeFor(channel), cc()[i], value);
      }
    }
    if (velocity_amount()) {
      velocity = U8Mix(velocity, randomU7(), ScaleModulationAmount(velocity_amount()));
    }
    uint8_t new_note = note;
    if (note_amount()) {
      new_note = U8Mix(note, randomU7(), ScaleModulationAmount(note_amount()));
    }

    note_map.Put(note, new_note);

    App::Send3(noteOnFor(channel), new_note, velocity);
  }
}

/* static */
void Randomizer::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  SendMessage(MIDI_NOTE_OFF, channel, note, velocity);
}

/* static */
void Randomizer::OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  SendMessage(MIDI_POLY_AFTERTOUCH, channel, note, velocity);
}

/* static */
void Randomizer::SendMessage(uint8_t message, uint8_t channel, uint8_t note, uint8_t velocity) {
  const auto messageOnChannel = byteOr(message, channel);
  if (!isActiveChannel(channel)) {
    App::Send3(messageOnChannel, note, velocity);
  } else {
    NoteMapEntry* entry = note_map.Find(note);
    if (entry) {
      App::Send3(messageOnChannel, entry->value, velocity);
      if (message == MIDI_NOTE_OFF) {
        entry->note = 0xff;
      }
    }
  }  
}

} // namespace apps
} // namespace midipal
