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
// Dispatcher app.

#ifndef MIDIPAL_APPS_DISPATCHER_H_
#define MIDIPAL_APPS_DISPATCHER_H_

#include "midipal/app.h"
#include "midipal/note_map.h"

namespace midipal {
namespace apps{

class Dispatcher {
 public:
  enum Parameter : uint8_t {
    input_channel_,
    mode_,
    base_channel_,
    num_voices_,
    polyphony_voices_,
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
  enum DispatcherMode {
    DISPATCHER_CYCLIC,
    DISPATCHER_POLYPHONIC_ALLOCATOR,
    DISPATCHER_RANDOM,
    DISPATCHER_STACK,
    DISPATCHER_VELOCITY
  };

  static void SendMessage(uint8_t message, uint8_t channel, uint8_t note, uint8_t velocity);
  static uint8_t map_channel(uint8_t index);

  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static inline uint8_t& input_channel() {
    return ParameterValue(input_channel_);
  }
  static inline DispatcherMode mode() {
    return static_cast<DispatcherMode>(ParameterValue(mode_));
  }
  static inline uint8_t& base_channel() {
    return ParameterValue(base_channel_);
  }
  static inline uint8_t& num_voices() {
    return ParameterValue(num_voices_);
  }
  static inline uint8_t& polyphony_voices() {
    return ParameterValue(polyphony_voices_);
  }

  static uint8_t counter_;
  
  DISALLOW_COPY_AND_ASSIGN(Dispatcher);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_DISPATCHER_H_
