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
//
// Contribution from: https://github.com/wackazong/midipal

#include "midipal/apps/clock_source_hd.h"

#include "avrlib/op.h"
#include "avrlib/string.h"
#include "midi/midi_constants.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{
  
using namespace avrlib;

const uint8_t ClockSourceHD::factory_data[Parameter::COUNT] PROGMEM = {
  0, 120, 0, 0, 0
};

uint8_t ClockSourceHD::settings[Parameter::COUNT];

/* static */
uint8_t ClockSourceHD::num_taps_;

/* static */
uint32_t ClockSourceHD::elapsed_time_;

/* static */
const AppInfo ClockSourceHD::app_info_ PROGMEM = {
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
  &OnRedraw, // uint8_t (*OnRedraw)();
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
void ClockSourceHD::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  // TODO: decide whether they should be brought back.
  // Ui::AddPage(STR_RES_GRV, STR_RES_SWG, 0, 5);
  // Ui::AddPage(STR_RES_AMT, UNIT_INTEGER, 0, 127);
  // Ui::AddPage(STR_RES_TAP, UNIT_INTEGER, 40, 240);
    Clock::Update(bpm(), groove_template(), groove_amount(), bpm_10th());
  running() = 0;
  num_taps_ = 0;
}

/* static */
void ClockSourceHD::OnRawByte(uint8_t byte) {
  bool is_realtime = byteAnd(byte, 0xf0) == 0xf0;
  bool is_sysex = (byte == 0xf7) || (byte == 0xf0);
  if (!is_realtime || is_sysex) {
    App::SendNow(byte);
  }
}

/* static */
void ClockSourceHD::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  if (param == running_) {
    if (value) {
      Start();
    } else {
      Stop();
    }
  // mirror BPM and tap bpm
  } else if (key == tap_bpm_) {
    ParameterValue(bpm_) = value;
  } else if (key == bpm_) {
    ParameterValue(tap_bpm_) = value;
  }
  ParameterValue(param) = value;
    Clock::Update(bpm(), groove_template(), groove_amount(), bpm_10th());
}


/* static */
uint8_t ClockSourceHD::OnClick() {
  // TODO: make it work.
  if (Ui::page() == 4) {
    uint32_t t = Clock::value();
    Clock::Reset();
    if (num_taps_ > 0 && t < 100000L) {
      elapsed_time_ += t;
      // TODO ???
      auto something = 60 * 78125L * num_taps_ / elapsed_time_;
      uint8_t new_bpm = avrlib::Clip(something, 40_u8, 240_u8);
      SetParameter(bpm_, new_bpm);
    } else {
      num_taps_ = 0;
      elapsed_time_ = 0;
    }
    ++num_taps_;
    return 1;
  } else {
    return 0;
  }
}

/* static */
uint8_t ClockSourceHD::OnIncrement(int8_t increment) {
  num_taps_ = 0;
  //here we can put manual handling of the bpm value and then return 1
  if (Ui::page() == 1 && Ui::editing() == 1) {
    //first handle update of bpm and bpm10th
    if (increment > 0) {
      if (bpm_10th() == 9) {
        bpm() += 1;
        bpm_10th() = 0;
      } else {
        bpm_10th() += 1;
      }
    } else {
      if (bpm_10th() == 0) {
        bpm() -= 1;
        bpm_10th() = 9;
      } else {
        bpm_10th() -= 1;
      }
    }

    //check for bpm being too low
    if (bpm() == 39) {
      bpm() = 40;
      bpm_10th() = 0;
    }
    //check for bpm being too high
    if (bpm() == 240 && bpm_10th() != 0) {
      bpm_10th() = 0;
    }
    //update the clock
    Clock::Update(bpm(), groove_template(), groove_amount(), bpm_10th());
    return 1;
  } else {
    return 0;
  }
}

/* static */
uint8_t ClockSourceHD::OnRedraw() {
  if (Ui::page() == 1) {
    memset(line_buffer, ' ', kLcdWidth);
    UnsafeItoa(bpm(), 3, &line_buffer[2]);
    PadRight(&line_buffer[2], 3, ' ');
    line_buffer[5] = '.';
    line_buffer[6] = '0' + bpm_10th();
    if (Ui::editing() == 1) {
      line_buffer[1] = '[';
      line_buffer[7] = ']';
    }
    Display::Print(0, line_buffer);
    return 1;
  } else {
    return 0;
  }
}

/* static */
void ClockSourceHD::OnStart() {
  Start();
}

/* static */
void ClockSourceHD::OnStop() {
  Stop();
}

/* static */
void ClockSourceHD::OnContinue() {
  Start();
}

/* static */
void ClockSourceHD::OnClock(uint8_t clock_source) {
  if (clock_source == CLOCK_MODE_INTERNAL) {
    App::SendNow(MIDI_SYS_CLK_TICK);
  }
}

/* static */
void ClockSourceHD::Stop() {
  if (running()) {
    Clock::Stop();
    App::SendNow(MIDI_SYS_CLK_STOP);
    running() = 0;
  }
}

/* static */
void ClockSourceHD::Start() {
  if (!running()) {
    Clock::Start();
    App::SendNow(MIDI_SYS_CLK_START);
    running() = 1;
  }
}

} // namespace apps
} // namespace midipal
