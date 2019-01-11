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
// Delay app.

#include "midipal/apps/delay.h"
#include "midi/midi_constants.h"

#include "midipal/display.h"

#include "midipal/clock.h"
#include "midipal/event_scheduler.h"
#include "midipal/note_stack.h"
#include "midipal/notes.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{
  
using namespace avrlib;

static const uint8_t velocity_factor_table[16] PROGMEM = {
  31, 60, 87, 112, 134, 155, 174, 191, 206, 219, 230, 239, 246, 251, 253, 255
};

const uint8_t Delay::factory_data[Delay::Parameter::COUNT] PROGMEM = {
  0, 120, 0, 0, 0, 8, 4, 10, 0, 0
};

/* Static vars */
/* Check: this is zero-initialised by being in the .bss area? */
uint8_t Delay::settings[Parameter::COUNT];
uint8_t Delay::running_;
uint8_t Delay::velocity_factor_reverse_log_;

/* static */
const AppInfo Delay::app_info_ PROGMEM = {
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
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);

  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_DELAY, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_DELAY, // app_name
  false
};

/* static */
void Delay::OnInit() {
  Ui::AddClockPages();
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_DELAY, STR_RES_2_1, 0, 12);
  Ui::AddPage(STR_RES_REP, UNIT_INTEGER, 0, 32);
  Ui::AddPage(STR_RES_VEL, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_TRS, UNIT_SIGNED_INTEGER, -24, 24);
  Ui::AddPage(STR_RES_DPL, UNIT_SIGNED_INTEGER, -63, 63);
  
  Clock::Update(bpm(), groove_template(), groove_amount());
  SetParameter(delay_, delay());  // Force an update of the prescaler.
  Clock::Start();
  running_ = 0;
}

/* static */
void Delay::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  if (status != noteOffFor(channel()) && status != noteOnFor(channel())) {
    App::Send(status, data, data_size);
  } else if (clk_mode() != CLOCK_MODE_NOTE ||
        !App::NoteClock(false, byteAnd(status, 0xf), data[0])) {
    App::Send(status, data, data_size);
  }
}

/* static */
void Delay::OnContinue() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running_ = 1;
  }
}

/* static */
void Delay::OnStart() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running_ = 1;
  }
}

/* static */
void Delay::OnStop() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running_ = 0;
  }
}

/* static */
void Delay::OnClock(uint8_t clock_source) {
  if (clk_mode() == clock_source && (running_ || clock_source == CLOCK_MODE_INTERNAL)) {
    if (clock_source == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_TICK);
    }
    SendEchoes();
  }
}

/* static */
void Delay::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(true, channel, note)) ||
      channel != Delay::channel()) {
    return;
  }
  ScheduleEchoes(note, velocity, num_taps());
}

/* static */
void Delay::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if ((clk_mode() == CLOCK_MODE_NOTE && App::NoteClock(false, channel, note)) ||
      channel != Delay::channel()) {
    return;
  }
  ScheduleEchoes(note, 0, num_taps());
}

/* static */
void Delay::SendEchoes() {
  uint8_t current = event_scheduler.root();
  while (current) {
    const SchedulerEntry& entry = event_scheduler.entry(current);
    if (entry.when) {
      break;
    }
    if (entry.note != kZombieSlot) {
      if (entry.velocity == 0) {
        App::Send3(noteOffFor(channel()), entry.note, 0);
      } else {
        App::Send3(noteOnFor(channel()), entry.note, entry.velocity);
      }
      ScheduleEchoes(entry.note, entry.velocity, entry.tag);
    }
    current = entry.next;
  }
  event_scheduler.Tick();
}

/* static */
void Delay::ScheduleEchoes(uint8_t note, uint8_t velocity, uint8_t num_taps) {
  if (num_taps == 0) {
    return;
  }
  int8_t doppler_iterations = Delay::num_taps() - num_taps;
  if (doppler_iterations < 0) {
    doppler_iterations = 0;
  }
  int16_t delay = ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, Delay::delay());
  for (uint8_t i = 0; i < doppler_iterations && doppler(); ++i) {
    delay += S16S8MulShift8(delay, doppler() << 1u);
  }

  uint8_t min_velocity_value = velocity > 0 ? 1_u8 : 0_u8;
  velocity = U8U8MulShift8(velocity, velocity_factor_reverse_log_);
  if (velocity < min_velocity_value) {
    velocity = min_velocity_value;
  }
  note = Transpose(note, transposition());
  App::SendLater(note, velocity, Clip(delay, 1_u8, 255_u8), num_taps - 1_u8);
  if (event_scheduler.overflow()) {
    Display::set_status('!');
  }
}

/* static */
// assumes key < Parameter::COUNT
void Delay::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  ParameterValue(param) = value;
  if (key <= Parameter::groove_amount_) {
    // it's a clock-related parameter
    Clock::Update(bpm(), groove_template(), groove_amount());
  }
  if (key == Parameter::clk_mode_ && value == 0) {
    // stop clock
    App::SendNow(MIDI_SYS_CLK_STOP);
  }
  velocity_factor_reverse_log_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
      velocity_factor_table, velocity_factor());
}

} // namespace apps
} // namespace midipal
