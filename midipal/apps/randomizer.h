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
// Randomizer app.

#ifndef MIDIPAL_APPS_RANDOMIZER_H_
#define MIDIPAL_APPS_RANDOMIZER_H_

#include "midipal/app.h"
#include "midipal/note_map.h"

namespace midipal {
namespace apps {

class Randomizer {
public:

  enum Parameter : uint8_t {
    channel_,
    global_amount_,
    note_amount_,
    velocity_amount_,
    cc_amount_0,
    cc_amount_1,
    cc_0,
    cc_1,
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
  static void SendMessage(
      uint8_t message,
      uint8_t channel,
      uint8_t note,
      uint8_t velocity);

  static uint8_t ScaleModulationAmount(uint8_t amount);

  static bool isActiveChannel(uint8_t channel);

  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& global_amount() {
    return ParameterValue(global_amount_);
  }
  static inline uint8_t& note_amount() {
    return ParameterValue(note_amount_);
  }
  static inline uint8_t& velocity_amount() {
    return ParameterValue(velocity_amount_);
  }
  static inline uint8_t* cc_amount() {
    return &settings[cc_amount_0];
  }
  static inline uint8_t* cc() {
    return &settings[cc_0];
  }

  DISALLOW_COPY_AND_ASSIGN(Randomizer);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_RANDOMIZER_H_
