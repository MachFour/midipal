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
// MIDI clock generator app.

#include "midipal/apps/clock_source.h"

#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

static const uint8_t clock_source_factory_data[ClockSource::Parameter::COUNT] PROGMEM = {
  0, 120, 0, 0, 0
};

/* static */
uint8_t ClockSource::settings[Parameter::COUNT];
/* static */
uint8_t ClockSource::num_taps_;
/* static */
uint32_t ClockSource::elapsed_time_;

/* static */
const AppInfo ClockSource::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  nullptr, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
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
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);

  &OnIncrement, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_CLOCK_SOURCE, // settings_offset
  settings, // settings_data
  clock_source_factory_data, // factory_data
  STR_RES_CLOCK, // app_name
  true
};

/* static */
void ClockSource::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  Ui::AddPage(STR_RES_GRV, STR_RES_SWG, 0, 5);
  Ui::AddPage(STR_RES_AMT, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_CONT_, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_TAP, UNIT_INTEGER, 40, 240);
  Clock::Update(bpm(), groove_template(), groove_amount());
  running() = 0;
  num_taps_ = 0;
  if (continuous()) {
    Clock::Start();
  }
}

/* static */
void ClockSource::OnRawByte(uint8_t byte) {
  bool is_realtime = byteAnd(byte, 0xf0u) == 0xf0u;
  bool is_sysex = (byte == 0xf7) || (byte == 0xf0);
  if (!is_realtime || is_sysex) {
    App::SendNow(byte);
  }
}

/* static */
void ClockSource::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  switch (param) {
    case running_:
      if (value) {
        Start();
      } else {
        Stop();
      }
      break;
    case continuous_:
      if (!running()) {
        if (value) {
          Clock::Start();
        } else {
          Clock::Stop();
        }
      }
      break;
    case tap_bpm_:
      // copy to BPM too
      ParameterValue(bpm_) = value;
      break;
    case bpm_:
      // copy to tap_bpm too
      ParameterValue(tap_bpm_) = value;
      break;
    default:
      break;
  }
  // now the actual write
  ParameterValue(param) = value;
  Clock::Update(bpm(), groove_template(), groove_amount());
}

/* static */
uint8_t ClockSource::OnClick() {
  // only need this for tap BPM feature
  if (Ui::page() != tap_bpm_) {
    return 0;
  }
  // tap calculation: find average bpm value
  auto t = Clock::value();
  Clock::Reset();
  if (num_taps_ != 0 && t < 400000L) {
    elapsed_time_ += t;
    auto new_bpm = avrlib::Clip(18750000 * num_taps_ / elapsed_time_, 40_u8, 240_u8);
    SetParameter(bpm_, new_bpm);
  } else {
    num_taps_ = 0;
    elapsed_time_ = 0;
  }
  ++num_taps_;
  return 1;
}

/* static */
uint8_t ClockSource::OnIncrement(int8_t increment) {
  num_taps_ = 0;
  return 0;
}

/* static */
void ClockSource::OnStart() {
  Start();
}

/* static */
void ClockSource::OnStop() {
  Stop();
}

/* static */
void ClockSource::OnContinue() {
  Start();
}

/* static */
void ClockSource::OnClock(uint8_t clock_source) {
  if (clock_source == CLOCK_MODE_INTERNAL) {
    App::SendNow(0xf8);
  }
}

/* static */
void ClockSource::Stop() {
  if (running()) {
    if (!continuous()) {
      Clock::Stop();
    }
    App::SendNow(0xfc);
    running() = 0;
  }
}

/* static */
void ClockSource::Start() {
  if (!running()) {
    if (!continuous()) {
      Clock::Start();
    }
    App::SendNow(0xfa);
    running() = 1;
  }
}

} // namespace apps
} // namespace midipal
