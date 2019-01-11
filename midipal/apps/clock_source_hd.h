// Author: Olivier Gillet (ol.gillet@gmail.com) +
// Contribution from: https://github.com/wackazong/midipal
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

#ifndef MIDIPAL_APPS_CLOCK_SOURCE_HD_H_
#define MIDIPAL_APPS_CLOCK_SOURCE_HD_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class ClockSourceHD {
 public:

  enum Parameter : uint8_t {
    running_,
    bpm_,
    bpm_10th_,
    groove_template_,
    groove_amount_,
    tap_bpm_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnStart();
  static void OnStop();
  static void OnContinue();
  static void OnRawByte(uint8_t byte);
  static uint8_t OnRedraw();
  
  static void SetParameter(uint8_t key, uint8_t value);
  static uint8_t OnClick();
  static uint8_t OnIncrement(int8_t increment);
  static void OnClock(uint8_t clock_source);
  
 private:
  static void Stop();
  static void Start();

  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static uint8_t& running() {
    return ParameterValue(running_);
  }
  static uint8_t& bpm() {
    return ParameterValue(bpm_);
  }
  static uint8_t& bpm_10th() {
    return ParameterValue(bpm_10th_);
  }
  static uint8_t& groove_template() {
    return ParameterValue(groove_template_);
  }
  static uint8_t& groove_amount() {
    return ParameterValue(groove_amount_);
  }
  static uint8_t& tap_bpm() {
    return ParameterValue(tap_bpm_);
  }

  static uint8_t num_taps_;
  static uint32_t elapsed_time_;
  
  DISALLOW_COPY_AND_ASSIGN(ClockSourceHD);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_CLOCK_SOURCE_H_
