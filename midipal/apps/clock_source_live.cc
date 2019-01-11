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

#include "midipal/apps/clock_source_live.h"

#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps {

const uint8_t ClockSourceLive::factory_data[Parameter::COUNT] PROGMEM = {
  0, 120, 1, 1, 0, 24
};

/* static */
uint8_t ClockSourceLive::settings[Parameter::COUNT];

/* static */
uint8_t ClockSourceLive::num_ticks_;

/* static */
uint32_t ClockSourceLive::elapsed_time_;

/* static */
const AppInfo ClockSourceLive::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
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
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);

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
  factory_data, // factory_data
  STR_RES_CLOCK, // app_name
  true
};

/* static */
void ClockSourceLive::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  Ui::AddPage(STR_RES_NUM, UNIT_INTEGER, 1, 16);
  Ui::AddPage(STR_RES_DIV, UNIT_INTEGER, 1, 16);
  Ui::AddPage(STR_RES_START, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_NOT, UNIT_NOTE, 0, 111);
  Ui::AddPage(STR_RES_TAP, UNIT_INTEGER, 40, 240);
  SetParameter(1, bpm());
  running() = 0;
  num_taps_ = 0;
}

static const uint8_t ratios[] PROGMEM = {
  0x13, 0x12, 0x23, 0x34, 0x11, 0x54, 0x43, 0x32, 0x53, 0x74, 0x21, 0x94,
  0x73, 0x52, 0x00, 0x00, 0x11
};

/* static */
void ClockSourceLive::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (note >= control_note() && note < control_note() + sizeof(ratios)) {
    uint8_t ratio = pgm_read_byte(ratios + note - control_note());
    if (ratio) {
      multiplier() = avrlib::U8ShiftRight4(ratio);
      SetParameter(3, byteAnd(ratio, 0x0f_u8));
      if (send_start()) {
        App::SendNow(0xfa);
      }
    }
  }
}

/* static */
void ClockSourceLive::OnRawByte(uint8_t byte) {
  bool is_realtime = byteAnd(byte, 0xf0) == 0xf0;
  bool is_sysex = (byte == 0xf7) || (byte == 0xf0);
  if (!is_realtime || is_sysex) {
    App::SendNow(byte);
  }
  if (byte == 0xfa) {
    num_ticks_ = 0;
  }
  if (byte == 0xf8) {
    ++num_ticks_;
    if (num_ticks_ == 192) {
      SetParameter(1, avrlib::Clip(18750000 * 8 / Clock::value(), 10_u8, 255_u8));
      num_ticks_ = 0;
      Clock::Reset();
    }
  }
}

/* static */
void ClockSourceLive::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  switch (param) {
    case running_:
      if (value) {
        Start();
      } else {
        Stop();
      }
      break;
    case tap_bpm_:
      // mirror the change to BPM parameter
      ParameterValue(bpm_) = value;
      break;
    case bpm_:
      // mirror the change to tap_bpm parameter
      ParameterValue(tap_bpm_) = value;
      break;
    default:
      break;
  }
  ParameterValue(param) = value;
  Clock::UpdateFractional(bpm(), multiplier(), divider(), 0, 0);
}

/* static */
uint8_t ClockSourceLive::OnClick() {
  if (Ui::page() == 6) {
    TapTempo();
    return 1;
  } else {
    return 0;
  }
}

/* static */
void ClockSourceLive::TapTempo() {
  uint32_t t = Clock::value();
  Clock::Reset();
  if (num_taps_ > 0 && t < 400000L) {
    elapsed_time_ += t;
    auto new_bpm = avrlib::Clip(18750000 * num_taps_ / elapsed_time_, 40_u8, 240_u8);
    SetParameter(bpm_, new_bpm);
  } else {
    num_taps_ = 0;
    elapsed_time_ = 0;
  }
  ++num_taps_;
}

/* static */
uint8_t ClockSourceLive::OnIncrement(int8_t increment) {
  num_taps_ = 0;
  return 0;
}

/* static */
void ClockSourceLive::OnStart() {
  Start();
}

/* static */
void ClockSourceLive::OnStop() {
  Stop();
}

/* static */
void ClockSourceLive::OnContinue() {
  Start();
}

/* static */
void ClockSourceLive::OnClock(uint8_t clock_source) {
  if (clock_source == CLOCK_MODE_INTERNAL) {
    App::SendNow(0xf8);
  }
}

/* static */
void ClockSourceLive::Stop() {
  if (running()) {
    Clock::Stop();
    App::SendNow(0xfc);
    running() = 0;
  }
}

/* static */
void ClockSourceLive::Start() {
  if (!running()) {
    Clock::Start();
    App::SendNow(0xfa);
    running() = 1;
  }
}

} // namespace apps
} // namespace midipal
