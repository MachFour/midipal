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
// Tanpura app.

#ifndef MIDIPAL_APPS_TANPURA_H_
#define MIDIPAL_APPS_TANPURA_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class Tanpura {
 public:
  enum Parameter : uint8_t {
    running_,
    clk_mode_,
    bpm_,
    clock_division_,
    channel_,
    root_,
    pattern_,
    shift_,
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
  static void OnNoteOn(uint8_t, uint8_t, uint8_t);
  static void OnContinue();
  static void OnStart();
  static void OnStop();
  static void OnClock(uint8_t clock_mode);
  
  static void SetParameter(uint8_t key, uint8_t value);
  

 private:
  static void Stop();
  static void Start();
  static void Tick();

  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static inline uint8_t& running() {
    return ParameterValue(running_);
  }
  static inline uint8_t& clk_mode() {
    return ParameterValue(clk_mode_);
  }
  static inline uint8_t& bpm() {
    return ParameterValue(bpm_);
  }
  static inline uint8_t& clock_division() {
    return ParameterValue(clock_division_);
  }
  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& root() {
    return ParameterValue(root_);
  }
  static inline uint8_t& pattern() {
    return ParameterValue(pattern_);
  }
  static inline uint8_t& shift() {
    return ParameterValue(shift_);
  }

  static uint8_t midi_clock_prescaler_;
  static uint8_t tick_;
  static uint8_t step_;
  
  DISALLOW_COPY_AND_ASSIGN(Tanpura);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_TANPURA_H_
