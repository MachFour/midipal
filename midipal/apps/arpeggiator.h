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
// Arpeggiator app.

#ifndef MIDIPAL_APPS_ARPEGGIATOR_H_
#define MIDIPAL_APPS_ARPEGGIATOR_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class Arpeggiator {
  static constexpr auto maxPatternLength = 16u;

public:
  enum Parameter : uint8_t {
    clk_mode_,
    bpm_,
    groove_template_,
    groove_amount_,
    channel_,
    direction_,
    num_octaves_,
    pattern_,
    pattern_length_,
    clock_division_,
    duration_,
    latch_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];

  static void OnInit();
  static void OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size);
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnControlChange(uint8_t, uint8_t, uint8_t);

  static void OnContinue();
  static void OnStart();
  static void OnStop();
  static void OnClock(uint8_t clock_mode);

  static void SetParameter(uint8_t key, uint8_t value);
  
  static const AppInfo app_info_ PROGMEM;
  
 protected:
  static void Tick();
  static void Start();
  static void StartArpeggio();
  static void StepArpeggio();
  static void SendNote(uint8_t note, uint8_t velocity);
  

private:

  enum ArpDirection : uint8_t {
    ARPEGGIO_DIRECTION_UP,
    ARPEGGIO_DIRECTION_DOWN,
    ARPEGGIO_DIRECTION_UP_DOWN,
    ARPEGGIO_DIRECTION_RANDOM,
    ARPEGGIO_DIRECTION_AS_PLAYED,
    ARPEGGIO_DIRECTION_CHORD
  };

  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
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
  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& direction() {
    return ParameterValue(direction_);
  }
  static inline ArpDirection directionEnum() {
    return static_cast<ArpDirection>(direction());
  }

  static inline uint8_t& num_octaves() {
    return ParameterValue(num_octaves_);
  }
  static inline uint8_t& pattern() {
    return ParameterValue(pattern_);
  }
  static inline uint8_t& pattern_length() {
    return ParameterValue(pattern_length_);
  }
  static inline uint8_t& clock_division() {
    return ParameterValue(clock_division_);
  }
  static inline uint8_t& duration() {
    return ParameterValue(duration_);
  }
  static inline uint8_t& latch() {
    return ParameterValue(latch_);
  }
  
  static uint8_t midi_clock_prescaler_;
  
  static uint8_t tick_;
  static uint8_t idle_ticks_;
  static uint8_t pattern_step;
  static int8_t current_direction_;
  static int8_t current_octave_;
  static int8_t current_step_;
  
  static uint8_t ignore_note_off_messages_;
  static bool recording_;
  static bool running_;

  DISALLOW_COPY_AND_ASSIGN(Arpeggiator);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_ARPEGGIATOR_H_
