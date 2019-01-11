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
// Knob controller app.

#ifndef MIDIPAL_APPS_CONTROLLER_H_
#define MIDIPAL_APPS_CONTROLLER_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class Controller {
 public:
  static constexpr int kCCDataSize = 8;

  enum Parameter : uint8_t {
    channel_,
    cc_data_,
    cc_data_last_ = cc_data_ + kCCDataSize - 1, /* the last index */
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnRawMidiData(
     uint8_t status,
     uint8_t* data,
     uint8_t data_size,
     uint8_t accepted_channel);

  static uint8_t OnPot(uint8_t pot, uint8_t value);

 private:

  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static uint8_t& channel() {
    return ParameterValue(channel_);
  }

  static uint8_t* cc_data() {
    return &settings[cc_data_];
  }

  DISALLOW_COPY_AND_ASSIGN(Controller);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_CONTROLLER_H_
