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
// Global clock.

#include <avrlib/bitops.h>
#include "midipal/clock.h"

#include "midipal/resources.h"

namespace midipal {

/* <static> */
bool Clock::running_;
uint32_t Clock::clock_;
uint16_t Clock::intervals_[kNumStepsInGroovePattern];
uint16_t Clock::interval_;
uint8_t Clock::tick_count_;
uint8_t Clock::step_count_;
/* </static> */

// TODO document what this number means?
static constexpr uint32_t clock_constant = 781250L;

static inline uint16_t calculate_swing(int32_t base_tick_duration,
      uint8_t groove_template, uint8_t groove_amount, uint8_t pattern_index) {
  int32_t swing_direction = ResourcesManager::Lookup<int16_t, uint8_t>(
        LUT_RES_GROOVE_SWING + groove_template, pattern_index);
  swing_direction *= base_tick_duration;
  swing_direction *= groove_amount;
  return U16(static_cast<uint32_t>(swing_direction) >> 16u);
}

/* static */
void Clock::Update(uint16_t bpm, uint8_t groove_template, uint8_t groove_amount, uint8_t bpm_10th) {
  auto bpm_times_10 = static_cast<uint32_t>(bpm) * 10 + bpm_10th;
  int32_t base_tick_duration = clock_constant*10/bpm_times_10 - 1;
  for (uint8_t i = 0; i < kNumStepsInGroovePattern; ++i) {
    auto swing = calculate_swing(base_tick_duration, groove_template, groove_amount, i);
    intervals_[i] = U16(base_tick_duration + swing);
  }
}

/* static */
void Clock::UpdateFractional(uint16_t bpm, uint8_t multiplier, uint8_t divider, uint8_t groove_template, uint8_t groove_amount) {
  auto bpm_uint32 = static_cast<uint32_t>(bpm);
  int32_t base_tick_duration = (clock_constant * divider) / (bpm_uint32 * multiplier) - 1;
  for (uint8_t i = 0; i < kNumStepsInGroovePattern; ++i) {
    auto swing = calculate_swing(base_tick_duration, groove_template, groove_amount, i);
    intervals_[i] = U16(base_tick_duration + swing);
  }
}

}  // namespace midipal
