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

#ifndef MIDIPAL_APPS_CC_KNOB_H_
#define MIDIPAL_APPS_CC_KNOB_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class CcKnob {
public:
  enum Parameter : uint8_t {
    cc_value_,
    channel_,
    type_,
    number_,
    min_,
    max_,
    COUNT
  };

  // holds all the settings in an array to enable pointer arithmetic
  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();

  static void OnRawMidiData(uint8_t status, uint8_t *data, uint8_t data_size, uint8_t accepted_channel);

  // Assume that key < Parameter::COUNT
  static uint8_t GetParameter(uint8_t key);
  static void SetParameter(uint8_t key, uint8_t value);



private:
  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static inline uint8_t& cc_value() {
    return ParameterValue(cc_value_);
  }
  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& type() {
    return ParameterValue(type_);
  }
  static inline uint8_t& number() {
    return ParameterValue(number_);
  }
  static inline uint8_t& min() {
    return ParameterValue(min_);
  }
  static inline uint8_t& max() {
    return ParameterValue(max_);
  }

  static void modifyParamsForCC();

  DISALLOW_COPY_AND_ASSIGN(CcKnob);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_CC_KNOB_H_
