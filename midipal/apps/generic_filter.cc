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
// Generic filter app.

#include <avr/eeprom.h>

#include "midipal/apps/generic_filter.h"

#include "avrlib/random.h"
#include "avrlib/op.h"

#include "midi/midi.h"

#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t GenericFilter::factory_data[Parameter::COUNT] PROGMEM = { 0 };

/* static */
uint8_t GenericFilter::settings[Parameter::COUNT];

/* static */
Modifier GenericFilter::modifiers_[kNumModifiers];

/* static */
const AppInfo GenericFilter::app_info_ PROGMEM = {
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
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_GENERIC_FILTER_PROGRAM, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_USER_PRG, // app_name
  true
};  

/* static */
void GenericFilter::OnInit() {
  Ui::AddPage(STR_RES_PRG, UNIT_INDEX, 0, 3);
  SetParameter(active_program_, active_program());
}

inline void GenericFilter::loadProgram(uint8_t num) {
  constexpr auto program_size = sizeof(modifiers_);
  auto program_addr = SETTINGS_GENERIC_FILTER_SETTINGS + num * program_size;
  eeprom_read_block(modifiers_, reinterpret_cast<void *>(program_addr), program_size);
}

/* static */
void GenericFilter::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  ParameterValue(param) = value;
  // right now this is always true
  if (param == active_program_) {
    loadProgram(value);
  }
}

/* static */
void GenericFilter::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  bool destroyed = false;
  bool forwarded = false;
  for (uint8_t i = 0; i < kNumModifiers; ++i) {
    const Modifier& modifier = modifiers_[i];
    if (modifier.accept(status, data[0], data[1])) {
      destroyed |= modifier.destroys();
      forwarded |= modifier.forwards();
    }
  }
  
  if (destroyed) {
    return;
  }
  
  if (forwarded) {
    App::Send(status, data, data_size);
  }
  
  // The rules are now applied one by one.
  for (uint8_t i = 0; i < kNumModifiers; ++i) {
    const Modifier& modifier = modifiers_[i];
    if (modifier.accept(status, data[0], data[1])) {
      // Apply a transformation on the channel.
      uint8_t channel = status;
      if (modifier.remaps_channel()) {
        channel = modifier.action;
      } else {
        channel += modifier.action;
      }
      if (modifier.promotes_to_cc()) {
        status = byteOr(0xb0, byteAnd(channel, 0x0f));
        data_size = 2;
      } else {
        auto type_bits = byteAnd(status, 0xf0);
        auto channel_bits = byteAnd(channel, 0x0f);
        status = byteOr(type_bits, channel_bits);
      }
      
      uint8_t new_data[2];
      // Apply a transformation on the values
      for (uint8_t j = 0; j < 2; ++j) {
        ValueTransformation t = modifier.value_transformation[j];
        uint8_t original_value = data[t.swap_source() ? (1 - j) : j];
        int16_t value = original_value;
        switch (byteAnd(t.operation, 0x0f)) {
          case VALUE_OEPRATION_ADD:
            value += t.argument[0];
            break;

          case VALUE_OPERATION_SUBTRACT:
            value -= t.argument[0];
            break;

          case VALUE_OPERATION_INVERT:
            value = 127_u8 - value;
            break;

          case VALUE_OPERATION_SET_TO:
            value = t.argument[0];
            break;
          
          case VALUE_OPERATION_SET_TO_RANDOM:
            value = U7(Random::GetByte());
            // Fall through
          case VALUE_OPERATION_MAP_TO_RANGE:
            if (t.argument[0] < t.argument[1]) {
              value = U8U8MulShift8(value << 1, t.argument[1] - t.argument[0]);
              value += t.argument[0];
            } else {
              value = U8U8MulShift8(value << 1, t.argument[0] - t.argument[1]);
              value = t.argument[1] - value;
            }
            break;

          case VALUE_OPERATION_ADD_RANDOM:
            value += U8U8MulShift8(Random::GetByte(), t.argument[1] - t.argument[0]);
            value += t.argument[0];
            break;
        }
        if (t.preserves_zero() && original_value == 0) {
          value = 0;
        }
        if (t.wrap()) {
          new_data[j] = byteAnd(value, 0x7f);
        } else {
          new_data[j] = Clip(value, 0x00_u8, 0x7f_u8);
        }
      }
      data[0] = new_data[0];
      data[1] = new_data[1];
    }
  }
  
  App::Send(status, data, data_size);
}

} // namespace apps
} // namespace midipal
