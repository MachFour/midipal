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
// MIDI clock generator app.

#include "midipal/apps/cc_knob.h"

#include "midi/midi.h"

#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t CcKnob::factory_data[Parameter::COUNT] PROGMEM = {
  63, 0, 0, 7, 0, 127
};

/* Check: this is zero-initialised by being in the .bss area? */
uint8_t CcKnob::settings[Parameter::COUNT];


/* static */
const AppInfo CcKnob::app_info_ PROGMEM = {
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
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  &GetParameter, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  CcKnob::Parameter::COUNT, // settings_size
  SETTINGS_CC_KNOB, // settings_offset
  CcKnob::settings, // settings_data
  factory_data, // factory_data
  STR_RES_CC_KNOB, // app_name
  true
};

/* static */
void CcKnob::OnInit() {
  Ui::AddPage(STR_RES_VAL, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_TYP, STR_RES_CC_, 0, 1);
  Ui::AddPage(STR_RES_NUM, UNIT_INTEGER, 0, 255);
  Ui::AddPage(STR_RES_MIN, UNIT_INTEGER, 0, 255);
  Ui::AddPage(STR_RES_MAX, UNIT_INTEGER, 0, 255);
}

/* static */
void CcKnob::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  App::Send(status, data, data_size);
}

/* static */
void CcKnob::modifyParamsForCC() {
  // Extended range not supported by CC.
  if (min() > 127) {
    min() = 127;
  }
  if (max() > 127) {
    max() = 127;
  }
  if (number() > 127) {
    number() = 127;
  }
}

/* static */
void CcKnob::SetParameter(uint8_t key, uint8_t value) {
  //uint8_t previous_value = cc_value_;
  uint8_t previous_cc_value = cc_value();
  //static_cast<uint8_t*>(&value_)[key] = value;
  ParameterValue(static_cast<Parameter>(key)) = value;

  if (key == type_ && value == 0) {
    // 0 value means CC, which has a smaller range than NRPN
    modifyParamsForCC();
  }
  cc_value() = Clip(cc_value(), min(), max());
  if (cc_value() != previous_cc_value) {
    // need to send a new CC/NRPN
    // 0xb0 are channel mode flag bits
    const uint8_t channelMsg = byteOr(0xb0, channel());
    if (type() == 0) {
      App::Send3(channelMsg, U7(number()), U7(cc_value()));
    } else {
      App::Send3(channelMsg, midi::kNrpnMsb, MSB8(number()));
      App::Send3(channelMsg, midi::kNrpnLsb, U7(number()));
      App::Send3(channelMsg, midi::kDataEntryMsb, MSB8(cc_value()));
      App::Send3(channelMsg, midi::kDataEntryLsb, U7(cc_value()));
    }
  }
}

/* static */
// TODO this may not be needed; I think clipping here is redundant
uint8_t CcKnob::GetParameter(uint8_t key) {
  if (key == Parameter::cc_value_) {
    return Clip(cc_value(), min(), max());
  } else {
    return ParameterValue(static_cast<Parameter>(key));
  }
}

} // namespace apps
} // namespace midipal
