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
// Scale processor app.

#ifndef MIDIPAL_APPS_SCALE_PROCESSOR_H_
#define MIDIPAL_APPS_SCALE_PROCESSOR_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class ScaleProcessor {
 public:

  enum Parameter : uint8_t {
    channel_,
    root_,
    scale_,
    original_,
    voice_1_,
    voice_2_,
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


 protected:
  static void ProcessNoteMessage(
      uint8_t message,
      uint8_t note,
      uint8_t velocity);

  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& root() {
    return ParameterValue(root_);
  }
  static inline uint8_t& scale() {
    return ParameterValue(scale_);
  }
  static inline uint8_t& original() {
    return ParameterValue(original_);
  }
  static inline uint8_t& voice_1() {
    return ParameterValue(voice_1_);
  }
  static inline uint8_t& voice_2() {
    return ParameterValue(voice_2_);
  }

  static uint8_t lowest_note_;
  static uint8_t previous_note_;
  static uint8_t voice_2_note_;
  static bool flip_;

  DISALLOW_COPY_AND_ASSIGN(ScaleProcessor);
};

} // namespace apps
} // namespace midipal

#endif  // MIDIPAL_APPS_SCALE_PROCESSOR_H_
