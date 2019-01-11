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

#include "midipal/apps/sh_sequencer.h"

#include "avrlib/op.h"
#include "avrlib/string.h"
#include "midi/midi_constants.h"

#include "midi/midi.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t ShSequencer::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0,
  0, 120, 0, 0, 12, 0, 8,
  48, 0xff, 48, 0xff, 60, 0xff, 60, 0xff,
};

/* <static> */
uint8_t ShSequencer::settings[Parameter::COUNT];


uint8_t ShSequencer::midi_clock_prescaler_;
uint8_t ShSequencer::tick_;
uint8_t ShSequencer::step_;
uint8_t ShSequencer::root_note_;
uint8_t ShSequencer::last_note_;
uint8_t ShSequencer::rec_mode_menu_option_;
uint8_t ShSequencer::pending_note_;
/* </static> */

/* static */
const AppInfo ShSequencer::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  &OnNoteOff, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnAftertouch)(uint8_t, uint8_t);
  OnControlChange, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnProgramChange)(uint8_t, uint8_t);
  OnPitchBend, // void (*OnPitchBend)(uint8_t, uint16_t);
  nullptr, // void (*OnSysExByte)(uint8_t);
  &OnClock, // void (*OnClock)();
  &OnStart, // void (*OnStart)();
  &OnContinue, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  nullptr, // bool (*CheckChannel)(uint8_t);
  nullptr, // void (*OnRawByte)(uint8_t);
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);

  &OnIncrement, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  &OnRedraw, // uint8_t (*OnRedraw)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  //9 + kNumSteps + 2 * (kNumSteps / 8 + 1),
  Parameter::COUNT, // settings_size
  SETTINGS_SEQUENCER, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_SEQUENCR, // app_name
  true
};

/* static */
void ShSequencer::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_REC, STR_RES_OFF, 0, 1);
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Clock::Update(bpm(), groove_template(), groove_amount());
  SetParameter(bpm_, bpm());
  Clock::Start();
  running() = 0;
  recording() = 0;
}

/* static */
void ShSequencer::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  // Forward everything except note on for the selected channel.
  if (status != noteOffFor(channel()) &&
      status != noteOnFor(channel())) {
    App::Send(status, data, data_size);
  }
}

/* static */
void ShSequencer::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  if (param == running_) {
    if (value) {
      Start();
    } else {
      Stop();
    }
  } else if (param == recording_) {
    Stop();
    memset(sequence_data(), 60, sequence_data_end_ - sequence_data_ + 1);
    memset(slide_data(), 0, slide_data_end_ - slide_data_ + 1);
    memset(accent_data(), 0, accent_data_end_ - accent_data_ + 1);
    recording() = 1;
    rec_mode_menu_option_ = 0;
    num_steps() = 0;
  }
  ParameterValue(param) = value;
  if (param < groove_amount()) {
    // it's a clock-related parameter
    Clock::Update(bpm(), groove_template(), groove_amount());
  }
  midi_clock_prescaler_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, clock_division());
}

/* static */
uint8_t ShSequencer::OnRedraw() {
  if (recording()) {
    memset(line_buffer, 0, kLcdWidth);
    line_buffer[0] = static_cast<char>(0xa5);
    UnsafeItoa(num_steps(), 2, &line_buffer[1]);
    PadRight(&line_buffer[1], 2, ' ');
    line_buffer[3] = '|';
    
    ResourcesManager::LoadStringResource(STR_RES_REST + rec_mode_menu_option_,
        &line_buffer[4], 4);
    AlignRight(&line_buffer[4], 4);
    Display::Print(0, line_buffer);
    return 1;
  } else {
    return 0;
  }
}

/* static */
uint8_t ShSequencer::OnIncrement(int8_t increment) {
  if (recording()) {
    rec_mode_menu_option_ += increment;
    if (rec_mode_menu_option_ == 0xff) {
      rec_mode_menu_option_ = 0;
    }
    if (rec_mode_menu_option_ >= 2) {
      rec_mode_menu_option_ = 2;
    }
    Ui::RefreshScreen();
    return 1;
  } else {
    return 0;
  }
}

/* static */
uint8_t ShSequencer::OnClick() {
  if (recording()) {
    switch (rec_mode_menu_option_) {
      case 0:
      case 1:
        sequence_data()[num_steps_] = 0xff_u8 - rec_mode_menu_option_;
        SaveAndAdvanceStep();
        if (num_steps() == kNumSteps) {
          recording() = 0;
        }
        break;
      case 2:
        recording() = 0;
        Ui::set_page(0);  // Go back to "run" page.
        break;
    }
  } else {
    return 0;
  }
  Ui::RefreshScreen();
  return 1;
}

/* static */
void ShSequencer::OnStart() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Start();
  }
}

/* static */
void ShSequencer::OnStop() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Stop();
  }
}

/* static */
void ShSequencer::OnContinue() {
  if (!running()) {
    Start();
  }
}

/* static */
void ShSequencer::OnClock(uint8_t clock_source) {
  if (clk_mode() == clock_source && running()) {
    if (clock_source == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_TICK);
    }
    Tick();
  }
}

/* static */
void ShSequencer::AddSlideAccent(uint8_t is_accent) {
  uint8_t accent_slide_index = num_steps()/8_u8;
  uint8_t accent_slide_mask = bitFlag(byteAnd(num_steps(), 0x7));
  if (is_accent) {
    accent_slide_index += kNumSteps / 8 + 1;
  }
  slide_data()[accent_slide_index] |= accent_slide_mask;
}

/* static */
void ShSequencer::OnPitchBend(uint8_t channel, uint16_t value) {
  if (channel == ShSequencer::channel() && recording()) {
    if ((value > 8192 + 2048 || value < 8192 - 2048)) {
      AddSlideAccent(0);
    }
  }
}

/* static */
void ShSequencer::OnControlChange(uint8_t channel, uint8_t controller, uint8_t value) {
  if (channel == ShSequencer::channel() && recording()) {
    if (controller == midi::kModulationWheelMsb && value > 0x40)  {
      AddSlideAccent(1);
    }
  }
}

/* static */
void ShSequencer::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != ShSequencer::channel()) {
    return;
  }
  uint8_t was_running = running();
  uint8_t just_started = 0;
  if (recording()) {
    sequence_data()[num_steps_] = note;
    SaveAndAdvanceStep();
    if (num_steps() == kNumSteps) {
      recording() = 0;
    }
  } else if (running() && clk_mode() == CLOCK_MODE_INTERNAL && note == last_note_) {
    Stop();
  } else if (clk_mode() == CLOCK_MODE_INTERNAL) {
    Start();
    root_note_ = note;
    just_started = 1;
  }
  
  if (!was_running && !just_started) {
    App::Send3(noteOnFor(channel), note, velocity);
  }
  if (running() && !recording()) {
    last_note_ = note;
  }
}

/* static */
void ShSequencer::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(false, channel, note)) ||
      channel != ShSequencer::channel()) {
    return;
  }
  
  if (!running()) {
    App::Send3(noteOffFor(channel), note, velocity);
  }
}

/* static */
void ShSequencer::Stop() {
  if (running()) {
    running() = 0;
    if (pending_note_ != 0xff) {
      App::Send3(noteOffFor(channel()), pending_note_, 0);
    }
    pending_note_ = 0xff;
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_STOP);
    }
  }
}

/* static */
void ShSequencer::Start() {
  if (!running()) {
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      Clock::Start();
      App::SendNow(MIDI_SYS_CLK_START);
    }
    tick_ = midi_clock_prescaler_ - 1_u8;
    root_note_ = 60;
    last_note_ = 60;
    running() = 1;
    step_ = 0;
    pending_note_ = 0xff;
  }
}

/* static */
void ShSequencer::Tick() {
  ++tick_;
  if (tick_ >= midi_clock_prescaler_) {
    tick_ = 0;
    uint8_t note = sequence_data()[step_];
    if (note == 0xff) {
      // It's a rest
      if (pending_note_ != 0xff) {
        App::Send3(noteOffFor(channel()), pending_note_, 0);
      }
      pending_note_ = 0xff;
    } else if (note == 0xfe) {
      // It's a tie, do nothing.
    } else {
      uint8_t accent_slide_index = step_ / 8_u8;
      uint8_t accent_slide_mask = bitFlag(byteAnd(step_, 0x7));
      uint8_t accented = accent_data()[accent_slide_index] & accent_slide_mask;
      uint8_t slid = slide_data()[accent_slide_index] & accent_slide_mask;
      note = U7(note);
      // note is promotied to int16_t for addition
      note = Clip(note + last_note_ - root_note_, 0_u8, 127_u8);
      
      if (pending_note_ != 0xff && !slid) {
        App::Send3(noteOffFor(channel()), pending_note_, 0);
      }
      App::Send3(noteOnFor(channel()), note, accented ? 127_u8 : 64_u8);
      if (pending_note_ != 0xff && slid) {
        App::Send3(noteOffFor(channel()), pending_note_, 0);
      }
      pending_note_ = note;
    }
    ++step_;
    if (step_ >= num_steps()) {
      step_ = 0;
    }
  }
}

/* static */
void ShSequencer::SaveAndAdvanceStep() {
  App::SaveSetting(sequence_data_ + num_steps());
  if (byteAnd(num_steps(), 0x7) == 0) {
    // save slide and accent data
    uint8_t slide_accent_index = num_steps() / 8_u8;
    App::SaveSetting(slide_data_ + slide_accent_index);
    App::SaveSetting(accent_data_ + slide_accent_index);
  }
  ++num_steps();
  App::SaveSetting(num_steps_);
}

} // namespace apps
} // namespace midipal
