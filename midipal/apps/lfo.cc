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

#include "midipal/apps/lfo.h"

#include "avrlib/op.h"
#include "avrlib/random.h"

#include "midipal/clock.h"
#include "midipal/note_stack.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t Lfo::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0, 120, 0, 0, 16, 0,
  
  7, 63, 63, 0, 2, 0,
  10, 0, 63, 0, 4, 0,
  74, 0, 63, 0, 2, 0,
  71, 0, 63, 0, 4, 0,
};

/* <static> */
uint8_t Lfo::settings[Parameter::COUNT];

uint16_t Lfo::phase_[kNumLfos];
uint16_t Lfo::phase_increment_[kNumLfos];

uint8_t Lfo::tick_;
uint8_t Lfo::midi_clock_prescaler_;
/* </static> */

/* static */
const AppInfo Lfo::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  &OnNoteOff, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnAftertouch)(uint8_t, uint8_t);
  nullptr, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnProgramChange)(uint8_t, uint8_t);
  nullptr, // void (*OnPitchBend)(uint8_t, uint16_t);
  nullptr, // void (*OnSysExByte)(uint8_t);
  &OnClock, // void (*OnClock)();
  &OnStart, // void (*OnStart)();
  &OnContinue, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  nullptr, // bool *(CheckChannel)(uint8_t);
  nullptr, // void (*OnRawByte)(uint8_t);
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);

  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_LFO, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_CC_LFO, // app_name
  true
};

/* static */
void Lfo::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_RES, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  
  for (uint8_t i = 0; i < kNumLfos; ++i) {
    Ui::AddPage(STR_RES_CC1 + i, UNIT_INTEGER, 0, 127);
    Ui::AddPage(STR_RES_AM1 + i, UNIT_SIGNED_INTEGER, -63, 63);
    Ui::AddPage(STR_RES_CE1 + i, UNIT_INTEGER, 0, 127);
    Ui::AddPage(STR_RES_WF1 + i, STR_RES_TRI, 0, 17);
    Ui::AddPage(STR_RES_RT1 + i, STR_RES_4_1, 0, 18);
    Ui::AddPage(STR_RES_SY1 + i, STR_RES_FRE, 0, 2);
  }

  Clock::Update(bpm(), groove_template(), groove_amount());
  SetParameter(bpm_, bpm());
  Clock::Start();
  running() = 0;
}

/* static */
void Lfo::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  if (status != byteOr(0x80, channel()) && status != byteOr(0x90, channel())) {
    App::Send(status, data, data_size);
  } else {
    if (clk_mode() != CLOCK_MODE_NOTE ||
        !App::NoteClock(false, byteAnd(status, 0xf), data[0])) {
      App::Send(status, data, data_size);
    }
  }
}

/* static */
void Lfo::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  if (param == running_) {
    if (value) {
      Start();
    } else {
      Stop();
    }
  }
  ParameterValue(param) = value;
  if (param < groove_amount_) {
    Clock::Update(bpm(), groove_template(), groove_amount());
  }
  midi_clock_prescaler_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, clock_division());
  uint16_t factor = midi_clock_prescaler_;
  for (uint8_t i = 0; i < kNumLfos; ++i) {
    phase_increment_[i] = ResourcesManager::Lookup<uint16_t, uint8_t>(
      lut_res_increments, lfo_data()[i].cycle_duration) * factor;
  }
}

/* static */
void Lfo::OnStart() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Start();
  }
}

/* static */
void Lfo::OnStop() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Stop();
  }
}

/* static */
void Lfo::OnContinue() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running() = 1;
  }
}

/* static */
void Lfo::OnClock(uint8_t clock_source) {
  if (clk_mode() == clock_source && running()) {
    if (clock_source == CLOCK_MODE_INTERNAL) {
      App::SendNow(0xf8);
    }
    Tick();
  }
}

/* static */
void Lfo::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != Lfo::channel()) {
    return;
  }
  for (uint8_t i = 0; i < kNumLfos; ++i) {
    if (lfo_data()[i].sync_mode == LFO_SYNC_NOTE_ON ||
        (lfo_data()[i].sync_mode == LFO_SYNC_START && NoteStack::size() == 0)) {
      phase_[i] = 0;
    }
  }
  NoteStack::NoteOn(note, velocity);
}

/* static */
void Lfo::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(false, channel, note)) ||
      channel != Lfo::channel()) {
    return;
  }
  NoteStack::NoteOff(note);
}

/* static */
void Lfo::Stop() {
  if (running()) {
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      App::SendNow(0xfc);
    }
    running() = 0;
  }
}

/* static */
void Lfo::Start() {
  if (!running()) {
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      Clock::Start();
      App::SendNow(0xfa);
    }
    tick_ = midi_clock_prescaler_ - 1_u8;
    running() = 1;
    for (uint8_t i = 0; i < kNumLfos; ++i) {
      phase_[i] = 0;
    }
  }
}

/* static */
void Lfo::Tick() {
  ++tick_;
  if (tick_ >= midi_clock_prescaler_) {
    tick_ = 0;
    for (uint8_t i = 0; i < kNumLfos; ++i) {
      uint8_t value = 0;
      bool skip = false;
      if (lfo_data()[i].waveform == 17) {
        // random
        if (phase_[i] < phase_increment_[i]) {
          value = Random::GetByte();
        } else {
          skip = true;
        }
      } else {
        uint16_t offset = U8U8Mul(lfo_data()[i].waveform, 129);
        value = InterpolateSample(wav_res_lfo_waveforms + offset, phase_[i] >> 1u);
      }
      phase_[i] += phase_increment_[i];
      if (lfo_data()[i].amount && !skip) {
        auto scaled_value = static_cast<int16_t>(lfo_data()[i].center_value) +
            S8S8MulShift8(lfo_data()[i].amount << 1u, value - 128_u8);
        auto clipped_value = Clip(scaled_value, 0_u8, 127_u8);
        if (lfo_data()[i].cc_number >= 127) {
          App::Send3(byteOr(0xe0, channel()), 0, clipped_value);
        } else {
          App::Send3(byteOr(0xb0, channel()), lfo_data()[i].cc_number, clipped_value);
        }
      }
    }
  }
}

} // namespace apps
} // namespace midipal
