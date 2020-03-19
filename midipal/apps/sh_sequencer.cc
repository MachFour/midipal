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

#include "avrlib/bitops.h"
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
uint8_t ShSequencer::playback_step_;
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
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);

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
  STR_RES_SH_SEQ, // app_name
  true
};

/* static */
void ShSequencer::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_REC, STR_RES_OFF, 0, 1);
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  // trigger side effects
  SetParameter(bpm_, bpm());
  SetParameter(clock_division_, clock_division());
  Clock::Start();
  running() = 0;
  recording() = 0;
}

/* static */
void ShSequencer::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  // Forward everything except note on for the selected channel.
  if (status != noteOffFor(channel()) && status != noteOnFor(channel())) {
    App::Send(status, data, data_size);
  }
}

/* static */
void ShSequencer::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  ParameterValue(param) = value;
  switch (param) {
    case running_:
      if (value) {
        Start();
      } else {
        Stop();
      }
      break;
    case recording_:
      Stop();
      memset(sequence_data(), 60, sequence_data_end_ - sequence_data_ + 1);
      memset(slide_data(), 0, slide_data_end_ - slide_data_ + 1);
      memset(accent_data(), 0, accent_data_end_ - accent_data_ + 1);
      recording() = 1;
      rec_mode_menu_option_ = 0;
      recorded_steps() = 0;
      break;
    case bpm_:
    case groove_template_:
    case groove_amount_:
      Clock::Update(bpm(), groove_template(), groove_amount());
      break;
    case clock_division_:
      midi_clock_prescaler_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
            midi_clock_tick_per_step, clock_division());
      break;
    default:
      break;
  }
}

/* static */
uint8_t ShSequencer::OnRedraw() {
  if (recording()) {
    memset(line_buffer, 0, kLcdWidth);
    line_buffer[0] = static_cast<char>(0xa5);
    UnsafeItoa(recorded_steps(), 2, &line_buffer[1]);
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
    // addition causes promotion to int16_t
    rec_mode_menu_option_ = Clip(rec_mode_menu_option_ + increment, 0_u8, 2_u8);
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
        sequence_data()[recorded_steps()] = 0xff_u8 - rec_mode_menu_option_;
        SaveAndAdvanceStep();
        if (recorded_steps() == kNumSteps) {
          recording() = 0;
        }
        break;
      case 2:
        recording() = 0;
        Ui::set_page(0);  // Go back to "run" page.
        break;
      default:
        break;
    }
    Ui::RefreshScreen();
    return 1;
  } else {
    return 0;
  }
}

/* static */
void ShSequencer::OnStart() {
  if (!isClockModeInternal()) {
    Start();
  }
}

/* static */
void ShSequencer::OnStop() {
  if (!isClockModeInternal()) {
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
    if (isClockModeInternal()) {
      App::SendNow(MIDI_SYS_CLK_TICK);
    }
    Tick();
  }
}

void ShSequencer::RecordSlideOrAccent(uint8_t *data_ptr) {
  /* slide and accent data is bit-packed */
  const uint8_t byte_index = recorded_steps() / 8_u8;
  const uint8_t bit_index = recorded_steps() % 8_u8;
  data_ptr[byte_index] |= bitFlag8Lookup(bit_index);
}

/* static */
void ShSequencer::OnPitchBend(uint8_t channel, uint16_t value) {
  if (channel == ShSequencer::channel() && recording()) {
    if ((value > 8192 + 2048 || value < 8192 - 2048)) {
      RecordSlideOrAccent(slide_data());
    }
  }
}

/* static */
void ShSequencer::OnControlChange(uint8_t channel, uint8_t controller, uint8_t value) {
  if (channel == ShSequencer::channel() && recording()) {
    if (controller == midi::kModulationWheelMsb && value > 0x40)  {
      RecordSlideOrAccent(accent_data());
    }
  }
}

/* static */
void ShSequencer::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != ShSequencer::channel()) {
    return;
  }
  bool was_running = running();
  bool just_started = false;
  if (recording()) {
    sequence_data()[recorded_steps()] = note;
    SaveAndAdvanceStep();
    if (recorded_steps() == kNumSteps) {
      recording() = 0;
    }
  } else if (isClockModeInternal()) {
    if (running() && note == last_note_) {
      Stop();
    } else {
      Start();
      root_note_ = note;
      just_started = true;
    }
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
    if (isClockModeInternal()) {
      App::SendNow(MIDI_SYS_CLK_STOP);
    }
  }
}

/* static */
void ShSequencer::Start() {
  if (!running()) {
    if (isClockModeInternal()) {
      Clock::Start();
      App::SendNow(MIDI_SYS_CLK_START);
    }
    tick_ = midi_clock_prescaler_ - 1_u8;
    root_note_ = 60;
    last_note_ = 60;
    running() = 1;
    playback_step_ = 0;
    pending_note_ = 0xff;
  }
}

/* static */
void ShSequencer::Tick() {
  ++tick_;
  if (tick_ >= midi_clock_prescaler_) {
    tick_ = 0;
    uint8_t note = sequence_data()[playback_step_];
    if (note == 0xff) {
      // It's a rest
      if (pending_note_ != 0xff) {
        App::Send3(noteOffFor(channel()), pending_note_, 0);
      }
      pending_note_ = 0xff;
    } else if (note == 0xfe) {
      // It's a tie, do nothing.
    } else {
      // accent and slide data is bit-packed
      const auto byte_index = playback_step_ / 8u;
      const auto bit_index = playback_step_ % 8u;
      bool accented = bitTest(accent_data()[byte_index], bit_index);
      bool slid = bitTest(slide_data()[byte_index], bit_index);
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
    if (++playback_step_ >= recorded_steps()) {
      playback_step_ = 0;
    }
  }
}

/* static */
void ShSequencer::SaveAndAdvanceStep() {
  App::SaveSetting(sequence_data_ + recorded_steps());
  if (byteAnd(recorded_steps(), 0x7) == 0) {
    // save slide and accent data
    uint8_t slide_accent_index = recorded_steps() / 8_u8;
    App::SaveSetting(slide_data_ + slide_accent_index);
    App::SaveSetting(accent_data_ + slide_accent_index);
  }
  ++recorded_steps();
  App::SaveSetting(recorded_steps_);
}

} // namespace apps
} // namespace midipal
