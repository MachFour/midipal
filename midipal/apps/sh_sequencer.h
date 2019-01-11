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
// A sequencer app inspired by the SH-101 sequencer.

#ifndef MIDIPAL_APPS_SH_SEQUENCER_H_
#define MIDIPAL_APPS_SH_SEQUENCER_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{


class ShSequencer {
  static constexpr uint8_t kNumSteps = 100;

public:
  // *_end_ variables mark the end (last index) of multi-byte parameters
  enum Parameter : uint8_t {
    running_,
    recording_,
    clk_mode_,
    bpm_,
    groove_template_,
    groove_amount_,
    clock_division_,
    channel_,
    num_steps_,
    sequence_data_,
    sequence_data_end_ = (sequence_data_ + kNumSteps) - 1,
    slide_data_,
    slide_data_end_ = (slide_data_ + kNumSteps / 8 + 1) - 1,
    accent_data_,
    accent_data_end_ = (accent_data_ + kNumSteps / 8 + 1) - 1,
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
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  
  static void OnContinue();
  static void OnStart();
  static void OnStop();
  static void OnClock(uint8_t clock_mode);
  
  static void OnControlChange(uint8_t channel, uint8_t cc_num, uint8_t value);
  static void OnPitchBend(uint8_t channel, uint16_t value);

  static uint8_t OnClick();
  static uint8_t OnIncrement(int8_t increment);
  static uint8_t OnRedraw();
  
  static void SetParameter(uint8_t key, uint8_t value);
  

 private:
  static void Stop();
  static void Start();
  static void Tick();
  static void SaveAndAdvanceStep();
  static void AddSlideAccent(uint8_t is_accent);

  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static uint8_t& running() {
    return ParameterValue(running_);
  }
  static uint8_t& recording() {
    return ParameterValue(recording_);
  }
  static uint8_t& clk_mode() {
    return ParameterValue(clk_mode_);
  }
  static uint8_t& bpm() {
    return ParameterValue(bpm_);
  }
  static uint8_t& groove_template() {
    return ParameterValue(groove_template_);
  }
  static uint8_t& groove_amount() {
    return ParameterValue(groove_amount_);
  }
  static uint8_t& clock_division() {
    return ParameterValue(clock_division_);
  }
  static uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static uint8_t& num_steps() {
    return ParameterValue(num_steps_);
  }
  static uint8_t* sequence_data() {
    return &settings[sequence_data_];
  }
  static uint8_t* slide_data() {
    return &settings[slide_data_];
  }
  static uint8_t* accent_data() {
    return &settings[accent_data_];
  }
  
  static uint8_t midi_clock_prescaler_;
  static uint8_t tick_;
  static uint8_t step_;
  static uint8_t root_note_;
  static uint8_t last_note_;
  static uint8_t rec_mode_menu_option_;
  static uint8_t pending_note_;
  
  DISALLOW_COPY_AND_ASSIGN(ShSequencer);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_SH_SEQUENCER_H_
