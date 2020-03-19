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

#ifndef MIDIPAL_APPS_FILTER_H_
#define MIDIPAL_APPS_FILTER_H_

#include "midipal/app.h"

namespace midipal {
namespace apps {

class Filter {
 public:
  // one parameter for each MIDI channel
  enum Parameter : uint8_t {
    ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7,
    ch8, ch9, cha, chb, chc, chd, che, chf,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size);
 

 private:
  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static inline bool channel_enabled(uint8_t which) {
    return settings[ch0 + which];
  }
  
  DISALLOW_COPY_AND_ASSIGN(Filter);
};

}  // namespace apps
}  // namespace midipal

#endif // MIDIPAL_APPS_FILTER_H_
