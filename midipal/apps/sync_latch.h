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
// Sync counter app.

#ifndef MIDIPAL_APPS_SYNC_LATCH_H_
#define MIDIPAL_APPS_SYNC_LATCH_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{


class SyncLatch {
 public:
  static constexpr uint8_t STATE_RUNNING = 1;
  static constexpr uint8_t STATE_ARMED = 2;

  enum Parameter : uint8_t {
    num_beats_,
    beat_division_,
    beat_counter_,
    step_counter_,
    state_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnRawByte(uint8_t byte);

  static void OnStart();
  static void OnStop();
  static void OnClock(uint8_t clock_mode);

  static uint8_t OnClick();
  static uint8_t OnRedraw();
  
 private:
  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static inline uint8_t& num_beats() {
    return ParameterValue(num_beats_);
  }
  static inline uint8_t& beat_division() {
    return ParameterValue(beat_division_);
  }
  static inline uint8_t& beat_counter() {
    return ParameterValue(beat_counter_);
  }
  static inline uint8_t& step_counter() {
    return ParameterValue(step_counter_);
  }
  static inline uint8_t& state() {
    return ParameterValue(state_);
  }
  
  DISALLOW_COPY_AND_ASSIGN(SyncLatch);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_SYNC_LATCH_H_
