// Copyright 2012 Olivier Gillet.
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
// Clock divider app.

#include "midipal/apps/sync_latch.h"

#include "midi/midi_constants.h"
#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t SyncLatch::factory_data[Parameter::COUNT] PROGMEM = {
  4, 8
};


/* static */
uint8_t SyncLatch::settings[Parameter::COUNT];

/* static */
const AppInfo SyncLatch::app_info_ PROGMEM = {
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
  nullptr, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  nullptr, // bool *(CheckChannel)(uint8_t);
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  &OnRedraw, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_SYNC_LATCH, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_SYNCLTCH, // app_name
  true
};  
  
/* static */
void SyncLatch::OnInit() {
  Ui::AddPage(STR_RES_NUM, UNIT_INTEGER, 1, 32);
  Ui::AddPage(STR_RES_DEN, STR_RES_2_1, 8, 16);
  // Add two dummy pages. They won't be shown on screen since Redraw is
  // overridden, but this allows the default navigation handling to be used.
  Ui::AddPage(0, UNIT_INTEGER, 1, 1);
  Ui::AddPage(0, UNIT_INTEGER, 1, 1);
  state() = 0;
}

/* static */
void SyncLatch::OnRawByte(uint8_t byte) {
  App::SendNow(byte);
}

/* static */
void SyncLatch::OnStart() {
  beat_counter() = 0;
  step_counter() = 0;
  state() = STATE_RUNNING;
}

/* static */
void SyncLatch::OnStop() {
  state() = 0;
}

/* static */
void SyncLatch::OnClock(uint8_t clock_mode) {
  if (clock_mode != CLOCK_MODE_EXTERNAL) {
    return;
  }
  ++step_counter();
  uint8_t num_ticks_per_beat = ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, beat_division());
  if (step_counter() >= num_ticks_per_beat) {
    step_counter() = 0;
    ++beat_counter();
  }
  if (beat_counter() >= num_beats()) {
    beat_counter() = 0;
  }
  if (byteAnd(state(), STATE_ARMED) && step_counter() == 0 && beat_counter() == 0) {
    if (byteAnd(state(), STATE_RUNNING)) {
      App::SendNow(MIDI_SYS_CLK_STOP);
      state() = 0;
    } else {
      App::SendNow(0xfa);
      state() = STATE_RUNNING;
    }
  }
}

/* static */
uint8_t SyncLatch::OnClick() {
  switch (Ui::page()) {
    case 2:
      state() |= STATE_ARMED;
      return 1;
    case 3:
      beat_counter() = 0;
      step_counter() = 0;
      return 1;
    default:
      return 0;
  }
}

/* static */
uint8_t SyncLatch::OnRedraw() {
  switch (Ui::page()) {
    case 2:
      memset(line_buffer, ' ', kLcdWidth);
      UnsafeItoa(beat_counter() + 1, 2, &line_buffer[1]);
      PadRight(&line_buffer[1], 2, '0');
      UnsafeItoa(step_counter(), 2, &line_buffer[4]);
      PadRight(&line_buffer[4], 2, '0');
      line_buffer[3] = ':';
      if (byteAnd(state(), STATE_ARMED)) {
        line_buffer[0] = '[';
        line_buffer[6] = ']';
      }
      line_buffer[7] = byteAnd(state(), STATE_RUNNING) ? '>' : static_cast<char>(0xa5);
      Display::Print(0, line_buffer);
      return 1;
    case 3:
      Ui::PrintString(STR_RES_RESET);
      return 1;
    default:
      return 0;
  }
}

} // namespace apps
} // namespace midipal
