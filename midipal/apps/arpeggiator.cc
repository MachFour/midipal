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

#include "midipal/apps/arpeggiator.h"

#include "avrlib/random.h"
#include "midi/midi.h"
#include "midi/midi_constants.h"

#include "midipal/display.h"

#include "midipal/clock.h"
#include "midipal/event_scheduler.h"
#include "midipal/note_stack.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;
using namespace midi;

const uint8_t arpeggiator_factory_data[Arpeggiator::Parameter::COUNT] PROGMEM = {
  0, 120, 0, 0, 0, 0, 1, 0, 16, 12, 14, 0
};

/* <static> */
uint8_t Arpeggiator::settings[Parameter::COUNT];

bool Arpeggiator::running_;
uint8_t Arpeggiator::midi_clock_prescaler_;
uint8_t Arpeggiator::tick_;
uint8_t Arpeggiator::idle_ticks_;
uint8_t Arpeggiator::pattern_step;
int8_t Arpeggiator::current_direction_;
int8_t Arpeggiator::current_octave_;
int8_t Arpeggiator::current_step_;
uint8_t Arpeggiator::ignore_note_off_messages_;
bool Arpeggiator::recording_;
/* </static> */

/* static */
const AppInfo Arpeggiator::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  &OnNoteOff, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnAftertouch)(uint8_t, uint8_t);
  &OnControlChange, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
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
  SETTINGS_ARPEGGIATOR, // settings_offset
  settings, // settings_data
  arpeggiator_factory_data, // factory_data
  STR_RES_ARPEGGIO, // app_name
  false
};

/* static */
void Arpeggiator::OnInit() {
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_DIR, STR_RES_UP, 0, 5);
  Ui::AddPage(STR_RES_OCT, UNIT_INTEGER, 1, 4);
  Ui::AddPage(STR_RES_PTN, UNIT_INDEX, 0, 21);
  Ui::AddPage(STR_RES_LEN, UNIT_INTEGER, 1, 16);
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_DUR, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_LAT, STR_RES_OFF, 0, 1);
  
  Clock::Update(bpm(), groove_template(), groove_amount());
  SetParameter(clock_division_, clock_division());  // Force an update of the prescaler.
  Clock::Start();
  idle_ticks_ = 96;
  running_ = false;
  ignore_note_off_messages_ = 0;
  recording_ = false;
}

/* static */
void Arpeggiator::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  // Forward everything except note on for the selected channel.
  if (status != noteOffFor(channel()) && status != noteOnFor(channel()) &&
      (status != controlChangeFor(channel()) || data[0] != kHoldPedal)) {
    App::Send(status, data, data_size);
  }
}

/* static */
void Arpeggiator::OnContinue() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running_ = true;
  }
}

/* static */
void Arpeggiator::OnStart() {
  if (clk_mode() != CLOCK_MODE_INTERNAL && !running_) {
    Start();
  }
}

/* static */
void Arpeggiator::OnStop() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running_ = false;
    App::FlushQueue(channel());
  }
}

/* static */
void Arpeggiator::OnClock(uint8_t clock_mode) {
  if (clk_mode() == clock_mode && running_) {
    if (clock_mode == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_TICK);
    }
    Tick();
  }
}

/* static */
void Arpeggiator::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != Arpeggiator::channel()) {
    return;
  }
  if (clk_mode() != CLOCK_MODE_EXTERNAL) {
    if (idle_ticks_ >= 96) {
      Clock::Start();
      Start();
      App::SendNow(MIDI_SYS_CLK_START);
    }
    idle_ticks_ = 0;
  }
  if (latch() && !recording_) {
    NoteStack::Clear();
    recording_ = true;
  }
  NoteStack::NoteOn(note, velocity);
}

/* static */
void Arpeggiator::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(false, channel, note)) ||
      channel != Arpeggiator::channel() || ignore_note_off_messages_) {
    return;
  }
  if (!latch()) {
    NoteStack::NoteOff(note);
  } else {
    if (note == NoteStack::most_recent_note().note) {
      recording_ = false;
    }
  }
}

/* static */
void Arpeggiator::SendNote(uint8_t note, uint8_t velocity) {
  velocity = U7(velocity);
  if (EventScheduler::Remove(note, 0)) {
    App::Send3(noteOffFor(channel()), note, 0);
  }
  // Send a note on and schedule a note off later.
  App::Send3(noteOnFor(channel()), note, velocity);
  App::SendLater(note, 0, ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, duration()) - 1_u8);
}

/* static */
void Arpeggiator::Tick() {
  ++tick_;
  
  if (NoteStack::size()) {
    idle_ticks_ = 0;
  }
  ++idle_ticks_;
  if (idle_ticks_ >= 96) {
    idle_ticks_ = 96;
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      running_ = false;
      App::FlushQueue(channel());
      App::SendNow(MIDI_SYS_CLK_STOP);
    }
  }
  
  App::SendScheduledNotes(channel());
  
  if (tick_ >= midi_clock_prescaler_) {
    tick_ = 0;
    auto pattern = ResourcesManager::Lookup<uint16_t, uint8_t>(
        lut_res_arpeggiator_patterns, Arpeggiator::pattern());
    bool has_arpeggiator_note = bitTest(pattern, pattern_step);
    if (NoteStack::size() && has_arpeggiator_note) {
      if (directionEnum() != ARPEGGIO_DIRECTION_CHORD) {
        StepArpeggio();
      
        const NoteEntry* arpeggio_note = &NoteStack::sorted_note(current_step_);
        if (directionEnum() == ARPEGGIO_DIRECTION_AS_PLAYED) {
          arpeggio_note = &NoteStack::played_note(current_step_);
        }
        uint8_t note = arpeggio_note->note;
        uint8_t velocity = arpeggio_note->velocity;
        note += 12 * current_octave_;
        while (note > 127) {
          note -= 12;
        }
        SendNote(note, velocity);
      } else {
        for (uint8_t i = 0; i < NoteStack::size(); ++i) {
          const NoteEntry* chord_note = &NoteStack::sorted_note(i);
          SendNote(chord_note->note, chord_note->velocity);
        }
      }
    }
    pattern_step++;
    if (pattern_step == pattern_length() || pattern_step == maxPatternLength) {
      // reached end of pattern
      pattern_step = 0;
    }
  }
}

/* static */
void Arpeggiator::Start() {
  running_ = true;
  pattern_step = 0;
  tick_ = midi_clock_prescaler_ - 1_u8;
  current_direction_ = (directionEnum() == ARPEGGIO_DIRECTION_DOWN ? S8(-1) : S8(1));
  current_octave_ = 127;
  recording_ = false;
}

/* static */
void Arpeggiator::StartArpeggio() {
  if (current_direction_ == 1) {
    current_octave_ = 0;
    current_step_ = 0;
  } else {
    current_step_ = NoteStack::size() - S8(1);
    current_octave_ = num_octaves() - 1_u8;
  }
}

/* static */
void Arpeggiator::OnControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
  if (channel != Arpeggiator::channel() || cc != kHoldPedal) {
    return;
  }
  if (ignore_note_off_messages_ && !value) {
    // Pedal was released, kill all pending arpeggios.
    NoteStack::Clear();
  }
  ignore_note_off_messages_ = value;
}

/* static */
void Arpeggiator::StepArpeggio() {
  if (current_octave_ == 127) {
    StartArpeggio();
    return;
  }
  
  uint8_t num_notes = NoteStack::size();
  if (directionEnum() == ARPEGGIO_DIRECTION_RANDOM) {
    uint8_t random_byte = Random::GetByte();
    current_octave_ = byteAnd(random_byte, 0xf);
    current_step_ = byteAnd(random_byte, 0xf0) >> 4u;
    while (current_octave_ >= num_octaves()) {
      current_octave_ -= num_octaves();
    }
    while (current_step_ >= num_notes) {
      current_step_ -= num_notes;
    }
  } else {
    current_step_ += current_direction_;
    uint8_t change_octave = 0;
    if (current_step_ >= num_notes) {
      current_step_ = 0;
      change_octave = 1;
    } else if (current_step_ < 0) {
      current_step_ = num_notes - 1_u8;
      change_octave = 1;
    }
    if (change_octave) {
      current_octave_ += current_direction_;
      if (current_octave_ >= num_octaves() || current_octave_ < 0) {
        if (directionEnum() == ARPEGGIO_DIRECTION_UP_DOWN) {
          current_direction_ = -current_direction_;
          StartArpeggio();
          if (num_notes > 1 || num_octaves() > 1) {
            StepArpeggio();
          }
        } else {
          StartArpeggio();
        }
      }
    }
  }
}

/* static */
// assume that key < Parameter::COUNT
void Arpeggiator::SetParameter(uint8_t key, uint8_t value) {
  const auto param = static_cast<Parameter>(key);
  ParameterValue(param) = value;
  switch (param) {
    case bpm_:
    case groove_template_:
    case groove_amount_:
      Clock::Update(bpm(), groove_template(), groove_amount());
      break;
    case clock_division_:
      midi_clock_prescaler_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
            midi_clock_tick_per_step, clock_division());
      break;
    case direction_:
      // When changing the arpeggio direction, reset the pattern.
      current_direction_ = (directionEnum() == ARPEGGIO_DIRECTION_DOWN ? S8(-1) : S8(1));
      StartArpeggio();
      break;
    case latch_:
      // When disabling latch mode, clear the note stack.
      if (!value) {
        NoteStack::Clear();
        recording_ = false;
      }
      break;
    default:
      break;
  }
}

} // namespace apps
} // namespace midipal
