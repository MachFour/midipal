// Copyright 2012 Peter Kvitek.
//
// Author: Peter Kvitek (pete@kvitek.com)
// Based on app code by Olivier Gillet (ol.gillet@gmail.com)
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
// Settings app.

#include "midipal/apps/settings.h"

#include "midi/midi.h"

#include "midipal/ui.h"

namespace midipal {
namespace apps{

static const uint8_t settings_factory_data[Settings::Parameter::COUNT] PROGMEM = {
  0, 0, 16, 84, 12,
};

/* static */
uint8_t Settings::settings[Parameter::COUNT] = {0};

/* static */
const AppInfo Settings::app_info_ PROGMEM = {
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
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
#ifdef MIDIBUD_FIRMWARE
  nullptr, // uint8_t (*OnSwitch)(uint8_t);
#else
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
#endif
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_SYSTEM_SETTINGS, // settings_offset
  settings, // settings_data
  settings_factory_data, // factory_data
  STR_RES_SETTINGS, // app_name
  false // realtime_clock_handling
};

/* static */
void Settings::OnInit() {
  Ui::AddPage(STR_RES_XFE, STR_RES_LET, 0, 1);
  Ui::AddPage(STR_RES_CCC, UNIT_INTEGER, 0, 16);
  Ui::AddPage(STR_RES_CLC, UNIT_INTEGER, 1, 16);
  Ui::AddPage(STR_RES_CLN, UNIT_NOTE, 24, 108);
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
}

/* static */
void Settings::OnRawByte(uint8_t byte) {
  App::SendNow(byte);
}

} // namespace apps
} // namespace midipal
