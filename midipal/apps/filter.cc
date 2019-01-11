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
// Channel filter app.

#include "midipal/apps/filter.h"

#include "midi/midi.h"

#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps {

/* static */
const uint8_t Filter::factory_data[Parameter::COUNT] PROGMEM = {
  1, 0, 0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

/* static */
uint8_t Filter::settings[Parameter::COUNT];

/* static */
const AppInfo Filter::app_info_ PROGMEM = {
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
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_FILTER, // settings_offset
  settings,
  factory_data, // factory_data
  STR_RES_CHNFILTR, // app_name
  true
};  

/* static */
void Filter::OnInit() {
  Lcd::SetCustomCharMapRes(chr_res_digits_10, 7, 1);
  Ui::AddRepeatedPage(UNIT_CHANNEL, STR_RES_OFF, 0, 1, 16);
}

/* static */
void Filter::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  auto isSystemCommonMessage = byteAnd(status, 0xf0) == 0xf0;
  // note: if (isSystemCommonMessage), the channel bits
  // don't really represent a channel
  auto channel = byteAnd(status, 0x0f);
  if (isSystemCommonMessage || channel_enabled(channel)) {
    App::Send(status, data, data_size);
  }
}

}  // namespace apps
}  // namespace midipal
