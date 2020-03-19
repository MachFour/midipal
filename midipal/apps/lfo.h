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
// CC LFO app.

#ifndef MIDIPAL_APPS_LFO_H_
#define MIDIPAL_APPS_LFO_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

enum LfoSyncMode {
  LFO_SYNC_FREE_RUNNING,
  LFO_SYNC_NOTE_ON,
  LFO_SYNC_START
};

struct LfoData {
  uint8_t cc_number;
  uint8_t amount;
  uint8_t center_value;
  uint8_t waveform;
  uint8_t cycle_duration;
  uint8_t sync_mode;
};


class Lfo {
 public:
  static const uint8_t kNumLfos = 4;

  enum Parameter : uint8_t {
    running_,
    clk_mode_,
    bpm_,
    groove_template_,
    groove_amount_,
    clock_division_,
    channel_,
    lfo_data_,
    lfo_data_end_ = lfo_data_ + kNumLfos*sizeof(LfoData) - 1, /* last byte */
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size);
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);

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

  static inline LfoData* lfo_data() {
    return reinterpret_cast<LfoData*>(&settings[lfo_data_]);
  }

  static uint16_t phase_[kNumLfos];
  static uint16_t phase_increment_[kNumLfos];
  
  static uint8_t tick_;
  static uint8_t midi_clock_prescaler_;
  
  DISALLOW_COPY_AND_ASSIGN(Lfo);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_LFO_H_
