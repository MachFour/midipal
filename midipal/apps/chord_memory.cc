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
// Chord memory app.

#include "midipal/apps/chord_memory.h"

#include "midi/midi.h"

#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t chord_memory_factory_data[ChordMemory::Parameter::COUNT] PROGMEM = {
  0, 4, 60, 63, 67, 70, 48, 48, 48, 48, 48, 48,
};

/* static */
uint8_t ChordMemory::settings[Parameter::COUNT];

/* static */
//uint8_t ChordMemory::root_;

/* static */
const AppInfo ChordMemory::app_info_ PROGMEM = {
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
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_CHORD_MEMORY, // settings_offset
  settings, // settings_data
  chord_memory_factory_data, // factory_data
  STR_RES_CHORDMEM, // app_name
  true
};

/* static */
void ChordMemory::OnInit() {
  Ui::AddPage(STR_RES_CHN, UNIT_INTEGER_ALL, 0, 16);
}

/* static */
void ChordMemory::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  uint8_t type = byteAnd(status, 0xf0);
  if (type != 0x80 && type != 0x90 && type != 0xa0) {
    App::Send(status, data, data_size);
  }
}

/* static */
uint8_t ChordMemory::OnClick() {
  if (!Ui::editing()) {
    num_notes() = 0;
  } else {
    App::SaveSettings();
  }
  return 0;
}

/* static */
void ChordMemory::PlayChord(uint8_t type, uint8_t channel, uint8_t note, uint8_t velocity) {
  if (num_notes() == 0) {
    App::Send3(type | channel, note, velocity);
  } else {
    for (uint8_t i = 0; i < num_notes(); ++i) {
      int16_t n = note;
      n += chord_data()[i] - chord_data()[0];
      while (n < 0) {
        n += 12;
      }
      while (n > 127) {
        n -= 12;
      }
      App::Send3(byteOr(type, channel), U8(n), velocity);
    }
  }
}

/* static */
inline bool ChordMemory::isActiveChannel(uint8_t channel) {
  // channel() == 0 represents 'all channels active'
  // thus actual channel() values starting from 1 correspond to channels.
  // The argument channel is 0-based
  return ChordMemory::channel() == 0 || ChordMemory::channel() == channel + 1;
}

/* static */
void ChordMemory::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isActiveChannel(channel)) {
    App::Send3(byteOr(0x90, channel), note, velocity);
  } else {
    // Record mode.
    if (Ui::editing()) {
      chord_data()[num_notes()++] = note;
      // Rotate buffer of recorded notes to avoid overflow ; but do not
      // touch the first note.
      if (num_notes() == kMaxChordNotes) {
        for (uint8_t i = 1; i < kMaxChordNotes - 1; ++i) {
          chord_data()[i] = chord_data()[i + 1];
        }
        num_notes() = kMaxChordNotes - 1;
      }
      App::Send3(byteOr(0x90, channel), note, velocity);
    } else {
      PlayChord(0x90, channel, note, velocity);
    }
  }
}

/* static */
void ChordMemory::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isActiveChannel(channel) || Ui::editing()) {
    App::Send3(byteOr(0x80, channel), note, velocity);
  } else {
    PlayChord(0x80, channel, note, velocity);
  }
}

/* static */
void ChordMemory::OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isActiveChannel(channel) || Ui::editing()) {
    App::Send3(byteOr(0xa0, channel), note, velocity);
  } else {
    PlayChord(0xa0, channel, note, velocity);
  }
}

} // namespace apps
} // namespace midipal
