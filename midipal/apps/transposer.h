// Copyright 2012 Olivier Gillet.
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
// Transposer app.

#ifndef MIDIPAL_APPS_TRANSPOSER_H_
#define MIDIPAL_APPS_TRANSPOSER_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class Transposer {
public:
  enum Parameter : uint8_t {
    channel_,
    transposition_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size);
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity);

private:
  static bool isActiveChannel(uint8_t channel);
  static uint8_t transposedNote(uint8_t note);

  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static uint8_t& channel() {
    return ParameterValue(channel_);
  };
  static int8_t& transposition() {
    return reinterpret_cast<int8_t&>(ParameterValue(transposition_));
  };
  
  DISALLOW_COPY_AND_ASSIGN(Transposer);
};

} // namespace apps
} // namespace midipal

#endif  // MIDIPAL_APPS_TRANSPOSER_H_
