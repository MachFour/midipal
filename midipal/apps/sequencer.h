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
// Sequencer app.

#ifndef MIDIPAL_APPS_SEQUENCER_H_
#define MIDIPAL_APPS_SEQUENCER_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class Sequencer {
private:
  static constexpr uint8_t kNumBytesPerStep = 4;
  static constexpr uint8_t kMaxSteps = 32;

 public:
  enum Parameter : uint16_t {
    running_,
    clk_mode_,
    bpm_,
    groove_template_,
    groove_amount_,
    clock_division_,
    channel_,
    cc_number_,
    note_track_,
    duration_track_,
    velocity_track_,
    cc_track_,
    num_steps_,
    sequence_data_,
    // XXX this may not fit into uint8_t
    sequence_data_end_ = sequence_data_ + kMaxSteps * kNumBytesPerStep,
    COUNT = sequence_data_end_
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT];
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
  
  static void SetParameter(uint8_t key, uint8_t value);
  static uint8_t CheckPageStatus(uint8_t index);
  

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
  static inline uint8_t& groove_template() {
    return ParameterValue(groove_template_);
  }
  static inline uint8_t& groove_amount() {
    return ParameterValue(groove_amount_);
  }
  static inline uint8_t& clock_division() {
    return ParameterValue(clock_division_);
  }
  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& cc_number() {
    return ParameterValue(cc_number_);
  }
  static inline uint8_t& note_track() {
    return ParameterValue(note_track_);
  }
  static inline uint8_t& duration_track() {
    return ParameterValue(duration_track_);
  }
  static inline uint8_t& velocity_track() {
    return ParameterValue(velocity_track_);
  }
  static inline uint8_t& cc_track() {
    return ParameterValue(cc_track_);
  }
  static inline uint8_t& num_steps() {
    return ParameterValue(num_steps_);
  }
  static inline uint8_t* sequence_step_data(uint8_t step) {
    return &settings[sequence_data_ + step*kNumBytesPerStep];
  }

  static uint8_t midi_clock_prescaler_;
  static uint8_t tick_;
  static uint8_t step_;
  static uint8_t root_note_;
  static uint8_t last_note_;
  
  DISALLOW_COPY_AND_ASSIGN(Sequencer);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_SEQUENCER_H_
