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

#include <midi/midi_constants.h>
#include "midipal/apps/sequencer.h"

#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t Sequencer::factory_data[Parameter::COUNT] PROGMEM = {
  0,   // Running_,
  CLOCK_MODE_INTERNAL,
  120, // Bpm_,
  0,   // Groove_template
  0,   // Groove_amount_,
  12,  // Clock_division_
  0,   // Channel_,
  74,  // Cc_number_,
  1,   // Note_track_,
  1,   // Duration_track_
  1,   // Velocity_track_
  1,   // Cc_track_,
  8,   // Num_steps_,
  48, 12, 15, 127,
  48, 12,  0, 127,
  60, 12, 15, 112,
  60, 12,  0, 112,
  48, 12, 15, 96,
  48, 12,  0, 96,
  60, 12, 15, 80,
  60, 12,  0, 80,
  48, 12, 15, 64,
  48, 12,  0, 64,
  60, 12, 15, 48,
  60, 12,  0, 48,
  48, 12, 15, 32,
  48, 12,  0, 32,
  60, 12, 15, 16,
  60, 12,  0, 16,
  48, 12, 15, 0,
  48, 12,  0, 0,
  60, 12, 15, 16,
  60, 12,  0, 16,
  48, 12, 15, 32,
  48, 12,  0, 32,
  60, 12, 15, 48,
  60, 12,  0, 48,
  48, 12, 15, 64,
  48, 12,  0, 64,
  60, 12, 15, 80,
  60, 12,  0, 80,
  48, 12, 15, 96,
  48, 12,  0, 96,
  60, 12, 15, 112,
  60, 12,  0, 112,
};

/* <static> */
uint8_t Sequencer::settings[Parameter::COUNT];

uint8_t Sequencer::midi_clock_prescaler_;
uint8_t Sequencer::tick_;
uint8_t Sequencer::playback_step_;
uint8_t Sequencer::root_note_;
uint8_t Sequencer::last_note_;
/* </static> */

/* static */
const AppInfo Sequencer::app_info_ PROGMEM = {
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
  &CheckPageStatus, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_SEQUENCER, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_SEQUENCR, // app_name
  false
};

/* static */
void Sequencer::OnInit() {
  Lcd::SetCustomCharMapRes(chr_res_sequencer_icons, 4, 1);
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_CC_, UNIT_INTEGER, 0, 127);
  Ui::AddPage(STR_RES_NOT, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_DUR, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_VEL, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_CC_LFO, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_STP, UNIT_INTEGER, 1, 32);
  Ui::AddRepeatedPage(STR_RES_1, UNIT_NOTE, 0, 127, 32);
  Ui::AddRepeatedPage(STR_RES_2, STR_RES_2_1, 0, 16, 32);
  Ui::AddRepeatedPage(STR_RES_3, UNIT_INTEGER, 0, 15, 32);
  Ui::AddRepeatedPage(STR_RES_4, UNIT_INTEGER, 0, 127, 32);
  // need to explicitly initialise these due to the side effects
  SetParameter(bpm_, bpm());
  SetParameter(clock_division_, clock_division());
  Clock::Start();
  running() = 0;
}

/* static */
void Sequencer::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  // Forward everything except note on for the selected channel.
  if (status != noteOffFor(channel()) && status != noteOnFor(channel())) {
    App::Send(status, data, data_size);
  }
}

/* static */
void Sequencer::SetParameter(uint8_t key, uint8_t value) {
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
    case bpm_:
    case groove_amount_:
    case groove_template_:
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
void Sequencer::OnStart() {
  if (!usingInternalClock()) {
    Start();
  }
}

/* static */
void Sequencer::OnStop() {
  if (!usingInternalClock()) {
    Stop();
  }
}

/* static */
void Sequencer::OnContinue() {
  if (!usingInternalClock()) {
    running() = 1;
  }
}

/* static */
void Sequencer::OnClock(uint8_t clock_source) {
  if (clk_mode() == clock_source && running()) {
    if (clock_source == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_TICK);
    }
    Tick();
  }
}

/* static */
void Sequencer::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != Sequencer::channel()) {
    return;
  }
  
  if (!note_track()) {
    App::Send3(noteOnFor(channel), note, velocity);
  }

  // Step recording.
  if (Ui::editing() && Ui::page() >= sequence_data_) {
    if (!running()) {
      App::Send3(noteOnFor(channel), note, velocity);
    }
    // edit the sequence
    // only 256/BYTES_PER_STEP steps supported
    uint8_t step = (Ui::page() - sequence_data_)/BYTES_PER_STEP;
    uint8_t* record_data = step_data(step);
    record_data[NOTE] = note;
    if (velocity_track()) {
      record_data[VELOCITY] = velocity/8_u8;
    }
    // skip to next page of same type, if possible
    uint8_t next_page = Ui::page() + BYTES_PER_STEP;
    if (CheckPageStatus(next_page) == PAGE_GOOD) {
      Ui::set_page(next_page);
    }
    return;
  }

  if (usingInternalClock()) {
    if (running() && note == last_note_ && note_track()) {
      Stop();
    } else if (!running()) {
      Start();
      root_note_ = note;
    }
  }
  last_note_ = note;
}

/* static */
void Sequencer::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(false, channel, note)) ||
      channel != Sequencer::channel()) {
    return;
  }
  
  if (!note_track() || !running()) {
    App::Send3(noteOffFor(channel), note, velocity);
  }
}

/* static */
void Sequencer::Stop() {
  if (running()) {
    // Flush the note off messages in the queue.
    App::FlushQueue(channel());
    // To be on the safe side, send an all notes off message.
    App::Send3(controlChangeFor(channel()), 123, 0);
    if (usingInternalClock()) {
      App::SendNow(MIDI_SYS_CLK_STOP);
    }
    running() = 0;
    root_note_ = 0;
    last_note_ = 0;
  }
}

/* static */
void Sequencer::Start() {
  if (!running()) {
    if (usingInternalClock()) {
      Clock::Start();
      App::SendNow(MIDI_SYS_CLK_START);
    }
    if (root_note_ == 0 || last_note_ == 0) {
      root_note_ = 60;
      last_note_ = 60;
    }
    // set tick_ so a note will be played during next Tick() call
    tick_ = midi_clock_prescaler_ - 1_u8;
    playback_step_ = 0;
    running() = 1;
  }
}

/* static */
void Sequencer::Tick() {
  App::SendScheduledNotes(channel());

  if (++tick_ >= midi_clock_prescaler_) {
    tick_ = 0;
    auto playback_data = step_data(playback_step_);
    // read step data as appropriate
    auto note = playback_data[NOTE];
    uint8_t duration;
    if (duration_track()) {
      duration = ResourcesManager::Lookup<uint8_t, uint8_t>(midi_clock_tick_per_step, playback_data[1]);
    } else {
      duration = midi_clock_prescaler_;
    }
    uint8_t velocity;
    if (velocity_track()) {
      velocity = playback_data[VELOCITY] * 8_u8;
    } else {
      velocity = kDefaultVelocity;
    }
    if (cc_track()) {
      // If a CC sequence is programmed, send a CC.
      auto cc = playback_data[CC];
      App::Send3(controlChangeFor(channel()), U7(cc_number()), U7(cc));
    }
    // If a note is programmed, send it.
    if (note_track() && velocity != 0) {
      // addition causes promotion to will be promoted to int16_t
      note = Clip(note + last_note_ - root_note_, 0_u8, 127_u8);
      App::Send3(noteOnFor(channel()), note, velocity);
      App::SendLater(note, 0, duration - 1_u8);
    }
    if (++playback_step_ >= num_steps()) {
      playback_step_ = 0;
    }
  }
}

/* static */
uint8_t Sequencer::CheckPageStatus(uint8_t index) {
  if (index < sequence_data_) {
    return PAGE_GOOD;
  }
  // for sequence pages, we need to compute whether they're valid
  // based on the currently enabled tracks
  index -= sequence_data_;
  
  uint8_t step_index = index / BYTES_PER_STEP;
  uint8_t step_offset = index % BYTES_PER_STEP;

  // We cannot go beyond the number of steps defined.
  // BYTES_PER_STEP = 4 --> four kinds of note data
  if (step_index >= num_steps()) {
    return PAGE_LAST;
  }

  switch (step_offset) {
    case NOTE:
      return note_track() ? PAGE_GOOD : PAGE_BAD;
    case DURATION:
      return duration_track() ? PAGE_GOOD : PAGE_BAD;
    case VELOCITY:
      return velocity_track() ? PAGE_GOOD : PAGE_BAD;
    case CC:
      return cc_track() ? PAGE_GOOD : PAGE_BAD;
    default:
      return PAGE_BAD;
  }
}

} // namespace apps
} // namespace midipal
