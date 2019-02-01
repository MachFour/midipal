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

#include "midipal/apps/poly_sequencer.h"

#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midi/midi_constants.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t PolySequencer::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0,
  0, 
  0,
  120,
  0,
  0,
  12,
  0,
  1
};

/* <static> */
uint8_t PolySequencer::settings[Parameter::COUNT];

uint8_t PolySequencer::midi_clock_prescaler_;
uint8_t PolySequencer::tick_;
uint8_t PolySequencer::step_;
uint8_t PolySequencer::edit_step_;
uint8_t PolySequencer::root_note_;
uint8_t PolySequencer::last_note_;
uint8_t PolySequencer::previous_rec_note_;
uint8_t PolySequencer::rec_mode_menu_option_;
uint8_t PolySequencer::pending_notes_[kNumTracks];
uint8_t PolySequencer::pending_notes_transposed_[kNumTracks];
/* </static> */

/* static */
const AppInfo PolySequencer::app_info_ PROGMEM = {
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

  &OnIncrement, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  &OnRedraw, // uint8_t (*OnRedraw)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_POLY_SEQUENCER, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_POLYSEQ, // app_name
  false
};

/* static */
void PolySequencer::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_REC, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_DUB, STR_RES_OFF, 0, 1);
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Clock::Update(bpm(), groove_template(), groove_amount());
  SetParameter(bpm_, bpm());
  Clock::Start();
  running() = 0;
  recording() = 0;
  overdubbing() = 0;
}

/* static */
void PolySequencer::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  // Forward everything except note on for the selected channel.
  if (status != noteOffFor(channel()) && status != noteOnFor(channel())) {
    App::Send(status, data, data_size);
  }
}

/* static */
void PolySequencer::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  if (param == running_) {
    if (value) {
      Start();
    } else {
      Stop();
    }
  } else if (key == recording_) {
    edit_step_ = 0;
    recording() = 1;
    rec_mode_menu_option_ = 0;
    overdubbing() = 0;
    num_steps() = 0;
    previous_rec_note_ = 0xff;
  } else if (key == overdubbing_) {
    edit_step_ = 0;
    recording() = 0;
    overdubbing() = 1;
    rec_mode_menu_option_ = 0;
    previous_rec_note_ = 0xff;
  }
  ParameterValue(param) = value;
  if (key <= groove_amount_) {
    // it's a clock parameter
    Clock::Update(bpm(), groove_template(), groove_amount());
  }
  midi_clock_prescaler_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, clock_division());
}

/* static */
uint8_t PolySequencer::OnRedraw() {
  if (recording() || overdubbing()) {
    memset(line_buffer, 0, kLcdWidth);
    UnsafeItoa(edit_step_, 3, &line_buffer[0]);
    PadRight(&line_buffer[0], 3, ' ');
    line_buffer[3] = static_cast<char>(0xa5);
    ResourcesManager::LoadStringResource(
        STR_RES_REST + rec_mode_menu_option_, &line_buffer[4], 4);
    AlignRight(&line_buffer[4], 4);
    Display::Print(0, line_buffer);
    return 1;
  } else {
    return 0;
  }
}

/* static */
uint8_t PolySequencer::OnIncrement(int8_t increment) {
  if (recording() || overdubbing()) {
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
uint8_t PolySequencer::OnClick() {
  if (recording() || overdubbing()) {
    switch (rec_mode_menu_option_) {
      case 0:
      case 1:
        Record(0xff_u8 - rec_mode_menu_option_);
        break;
      case 2:
        recording() = 0;
        overdubbing() = 0;
        break;
    }
    Ui::RefreshScreen();
    return 1;
  } else {
    return 0;
  }
}

/* static */
void PolySequencer::OnStart() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Start();
  }
}

/* static */
void PolySequencer::OnStop() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Stop();
  }
}

/* static */
void PolySequencer::OnContinue() {
  if (!running()) {
    Start();
  }
}

/* static */
void PolySequencer::OnClock(uint8_t clock_source) {
  if (clk_mode() == clock_source && running()) {
    if (clock_source == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_TICK);
    }
    Tick();
  }
}

/* static */
void PolySequencer::Record(uint8_t note) {
  uint8_t step_data = note;
  if (note == 0xfe && previous_rec_note_ != 0xff) {
    step_data = byteOr(0x80, previous_rec_note_);
  } else if (note == 0xff) {
    step_data = 0xff;
  } else {
    previous_rec_note_ = note;
  }
  if (recording()) {
    for (uint8_t i = 0; i < kNumTracks; ++i) {
      auto offset = i * kNumSteps + edit_step_;
      sequence_data()[offset] = 0xff;
      App::SaveSetting(sequence_data_ + offset);
    }
  }
  if (step_data != 0xff) {
    // Find a free slot to record the note.
    bool slot_found = false;
    for (uint8_t i = 0; i < kNumTracks; ++i) {
      auto offset = i * kNumSteps + edit_step_;
      if (sequence_data()[offset] == 0xff) {
        sequence_data()[offset] = step_data;
        App::SaveSetting(sequence_data_ + offset);
        slot_found = true;
        break;
      }
    }
    if (!slot_found) {
      sequence_data()[edit_step_] = step_data;
      App::SaveSetting(sequence_data_ + edit_step_);
    }
  }
  
  edit_step_++;
  if (edit_step_ >= kNumSteps) {
    // Auto-overdub if max number of steps is reached.
    edit_step_ = 0;
  }
  if (overdubbing() && edit_step_ >= num_steps()) {
    edit_step_ = 0;
  }
  if (recording()) {
    num_steps() = edit_step_;
    App::SaveSetting(num_steps());
  }
}

/* static */
void PolySequencer::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != PolySequencer::channel()) {
    return;
  }
  bool was_running = running();
  bool just_started = false;
  if (recording() || overdubbing()) {
    Record(note);
  } else if (running() && clk_mode() == CLOCK_MODE_INTERNAL && note == last_note_) {
    Stop();
  } else if (clk_mode() == CLOCK_MODE_INTERNAL) {
    Start();
    root_note_ = note;
    just_started = true;
  }
  
  if (!was_running && !just_started) {
    App::Send3(noteOnFor(PolySequencer::channel()), note, velocity);
  }
  if (running() && !recording() && !overdubbing()) {
    last_note_ = note;
  }
}

/* static */
void PolySequencer::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(false, channel, note)) ||
      channel != PolySequencer::channel()) {
    return;
  }
  
  if (!running()) {
    App::Send3(noteOffFor(channel), note, velocity);
  }
}

/* static */
void PolySequencer::Stop() {
  if (!running()) {
    return;
  }
  running() = 0;
  // Stop pending notes.
  for (uint8_t i = 0; i < kNumTracks; ++i) {
    if (pending_notes_[i] != 0xff) {
      App::Send3(noteOffFor(channel()), pending_notes_transposed_[i], 0);
    }
  }
  if (clk_mode() == CLOCK_MODE_INTERNAL) {
    App::SendNow(MIDI_SYS_CLK_STOP);
  }
}

/* static */
void PolySequencer::Start() {
  if (running()) {
    return;
  }
  if (clk_mode() == CLOCK_MODE_INTERNAL) {
    Clock::Start();
    App::SendNow(MIDI_SYS_CLK_START);
  }
  tick_ = midi_clock_prescaler_ - 1_u8;
  root_note_ = 60;
  last_note_ = 60;
  running() = 1;
  step_ = 0;
  memset(pending_notes_, 0xff, kNumTracks);
}

/* static */
void PolySequencer::Tick() {
  ++tick_;
  if (tick_ >= midi_clock_prescaler_) {
    tick_ = 0;
    
    // Send note off.
    for (uint8_t i = 0; i < kNumTracks; ++i) {
      if (pending_notes_[i] != 0xff) {
        uint8_t is_tied = 0;
        for (uint8_t j = 0; j < kNumTracks; ++j) {
          auto offset = j * kNumSteps + step_;
          if (sequence_data()[offset] == byteOr(pending_notes_[i], 0x80)) {
            is_tied = 1;
            break;
          }
        }
        if (!is_tied) {
          App::Send3(noteOffFor(channel()), pending_notes_transposed_[i], 0);
          pending_notes_[i] = 0xff;
        }
      }
    }
    
    // Send note on.
    for (uint8_t i = 0; i < kNumTracks; ++i) {
      auto offset = i * kNumSteps + step_;
      uint8_t note = sequence_data()[offset];
      if (note < 0x80) {
        pending_notes_[i] = note;
        note += last_note_ - root_note_;
        pending_notes_transposed_[i] = note;
        App::Send3(noteOnFor(channel()), note, 100);
      }
    }
    ++step_;
    if (step_ >= num_steps()) {
      step_ = 0;
    }
  }
}

} // namespace apps
} // namespace midipal
