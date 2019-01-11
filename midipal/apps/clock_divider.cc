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
// Clock divider app.

#include "midipal/apps/clock_divider.h"

#include "midipal/ui.h"

namespace midipal {
namespace apps{

static const uint8_t clock_divider_factory_data[ClockDivider::Parameter::COUNT] PROGMEM = {
  1, 0,
};

/* static */
uint8_t ClockDivider::settings[Parameter::COUNT];

/* static */
uint8_t ClockDivider::counter_;

/* static */
uint8_t ClockDivider::start_delay_counter_;

/* static */
bool ClockDivider::running_;


/* static */
const AppInfo ClockDivider::app_info_ PROGMEM = {
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
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_CLOCK_DIVIDER, // settings_offset
  settings, // settings_data
  clock_divider_factory_data, // factory_data
  STR_RES_DIVIDER, // app_name
  true
};  
  
/* static */
void ClockDivider::OnInit() {
  Ui::AddPage(STR_RES_DIV, UNIT_INTEGER, 1, 32);
  Ui::AddPage(STR_RES_DELAY, UNIT_INTEGER, 0, 96);
  counter_ = 0;
  start_delay_counter_ = 0;
  running_ = false;
}

/* static */
void ClockDivider::OnRawByte(uint8_t byte) {
  if (byte == 0xf8) {
    // Send a delayed START if necessary.
    if (start_delay_counter_) {
      --start_delay_counter_;
      if (!start_delay_counter_) {
        App::SendNow(0xfa);
        running_ = true;
      }
    }
    // Process clock message and divide it.
    if (running_) {
      ++counter_;
      if (counter_ >= divider()) {
        App::SendNow(0xf8);
        counter_ = 0;
      }
    }
  } else if (byte == 0xfa) {
    counter_ = divider() - 1_u8;
    start_delay_counter_ = start_delay();
    if (!start_delay_counter_) {
      App::SendNow(0xfa);
      running_ = true;
    }
  } else {
    if (byte == 0xfc) {
      running_ = false;
    }
    App::SendNow(byte);
  }
}

} // namespace apps
} // namespace midipal
