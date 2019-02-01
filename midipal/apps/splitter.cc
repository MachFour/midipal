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
// Splitter app.


#include "midipal/apps/splitter.h"

#include "midi/midi_constants.h"
#include "midi/midi.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

const uint8_t Splitter::factory_data[Parameter::COUNT] PROGMEM = {
  0, 48, 0, 1
};

/* static */
uint8_t Splitter::settings[Parameter::COUNT];

/* static */
const AppInfo Splitter::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  nullptr, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
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
  SETTINGS_SPLITTER, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_SPLITTER, // app_name
  true
};  

/* static */
void Splitter::OnInit() {
  Ui::AddPage(STR_RES_INP, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_SPL, UNIT_NOTE, 20, 108);
  Ui::AddPage(STR_RES_LOW, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_UPP, UNIT_INDEX, 0, 15);
}

/* static */
void Splitter::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  const uint8_t type = byteAnd(status, 0xf0);
  const uint8_t channel = byteAnd(status, 0x0f);
  if (channel == input_channel()) {
    switch (type) {
      case MIDI_SYSEX: // and other system messages; just forward
        App::Send(status, data, data_size);
        break;
      case MIDI_NOTE_OFF:
      case MIDI_NOTE_ON:
      case MIDI_POLY_AFTERTOUCH: {
        // Split note on/off/aftertouch messages depending on the note value.
        uint8_t note = data[0];
        auto newChannel = note < split_point() ? lower_channel() : upper_channel();
        App::Send(channelMessage(type, newChannel), data, data_size);
        break;
      }
      default:
        // Pass through the other messages, just rewrite their channel.
        // send messages on both channels only if they're different
        if (upper_channel() != lower_channel()) {
          App::Send(channelMessage(type, upper_channel()), data, data_size);
        }
        App::Send(channelMessage(type, lower_channel()), data, data_size);
        break;
    }
  } else {
    // just forward the message
    App::Send(status, data, data_size);
  }
}

} // namespace apps
} // namespace midipal
