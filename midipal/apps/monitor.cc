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
// MIDI monitor app.

#include "midipal/apps/monitor.h"

#include "avrlib/string.h"

#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

static const uint8_t monitor_factory_data[Monitor::Parameter::COUNT] PROGMEM = { 0 };

/* static */
uint8_t Monitor::settings[Parameter::COUNT];
/* static */
uint8_t Monitor::idle_counter_;

/* static */
const AppInfo Monitor::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  &OnNoteOff, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  &OnNoteAftertouch, // void (*OnNoteAftertouch)(uint8_t, uint8_t);
  &OnAftertouch, // void (*OnAftertouch)(uint8_t, uint8_t, uint8_t);
  &OnControlChange, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  &OnProgramChange, // void (*OnProgramChange)(uint8_t, uint8_t);
  &OnPitchBend, // void (*OnPitchBend)(uint8_t, uint16_t);
  &OnSysExByte, // void (*OnSysExByte)(uint8_t);
  &OnClock, // void (*OnClock)();
  &OnStart, // void (*OnStart)();
  &OnContinue, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  &CheckChannel, // bool *(CheckChannel)(uint8_t);
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)();
  &OnRedraw, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)();
  Parameter::COUNT, // settings_size
  SETTINGS_MONITOR, // settings_offset
  settings, // settings_data
  monitor_factory_data, // factory_data
  STR_RES_MONITOR, // app_name
  true
};

/* static */
void Monitor::OnInit() {
  Lcd::SetCustomCharMapRes(chr_res_digits_10, 7, 1);
  Ui::Clear();
  Ui::AddPage(STR_RES_CHN, UNIT_INTEGER_ALL, 0, 16);
}

/* static */
void Monitor::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  // 01234567
  // 1 C#7 7f
  Ui::Clear();
  Ui::PrintChannel(&line_buffer[0], channel);
  Ui::PrintNote(&line_buffer[2], note);
  if (velocity == 0xff) {
    line_buffer[6] = '-';
    line_buffer[7] = '-';
  } else {
    Ui::PrintHex(&line_buffer[6], velocity);
  }
  Ui::RefreshScreen();
}

/* static */
void Monitor::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  OnNoteOn(channel, note, 0xff);
}

/* static */
void Monitor::OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  OnNoteOn(channel, note, byteOr(0xa0, velocity >> 4u));
}

/* static */
void Monitor::OnAftertouch(uint8_t channel, uint8_t velocity) {
  Ui::Clear();
  Ui::PrintChannel(&line_buffer[0], channel);
  line_buffer[2] = 'a';
  line_buffer[3] = 'f';
  line_buffer[4] = 't';
  Ui::PrintHex(&line_buffer[6], velocity);
  Ui::RefreshScreen();
}

/* static */
void Monitor::OnControlChange( uint8_t channel, uint8_t cc_num, uint8_t value) {
  // 01234567
  // 1 C45 67
  if (cc_num >= 0x78) {
    PrintString(channel, U8(STR_RES_SNDOFF + cc_num - 0x78));
  } else {
    Ui::PrintChannel(&line_buffer[0], channel);
    line_buffer[2] = '#';
    Ui::PrintHex(&line_buffer[3], cc_num);
    Ui::PrintHex(&line_buffer[6], value);
    Ui::RefreshScreen();
  }
}

/* static */
void Monitor::OnProgramChange(uint8_t channel, uint8_t program) {
  Ui::Clear();
  Ui::PrintChannel(&line_buffer[0], channel);
  line_buffer[2] = 'p';
  line_buffer[3] = 'g';
  line_buffer[4] = 'm';
  Ui::PrintHex(&line_buffer[6], program);
  Ui::RefreshScreen();
}

/* static */
void Monitor::OnPitchBend(uint8_t channel, uint16_t pitch_bend) {
  Ui::Clear();
  Ui::PrintChannel(&line_buffer[0], channel);
  line_buffer[2] = 'b';
  Ui::PrintHex(&line_buffer[4], highByte(pitch_bend));
  Ui::PrintHex(&line_buffer[6], lowByte(pitch_bend));
  Ui::RefreshScreen();
}

/* static */
void Monitor::PrintString(uint8_t channel, uint8_t res_id) {
  if (Ui::editing()) {
    return;
  }
  Ui::Clear();
  Ui::PrintChannel(&line_buffer[0], channel);
  ResourcesManager::LoadStringResource(res_id, &line_buffer[2], 6);
  AlignRight(&line_buffer[2], 6);
  Ui::RefreshScreen();
}

/* static */
void Monitor::OnSysExByte(uint8_t sysex_byte) {
  if (sysex_byte == 0xf0) {
    PrintString(0xff, STR_RES_SYSX__);
  } else if (sysex_byte == 0xf7) {
    PrintString(0xff, STR_RES___SYSX);
  } else {
    PrintString(0xff, STR_RES__SYSX_);
  }
}

/* static */
void Monitor::OnClock(uint8_t clock_mode) {
  if (Ui::editing()) {
    return;
  }
  if (clock_mode == CLOCK_MODE_EXTERNAL) {
    line_buffer[1] = static_cast<char>(0xa5);
    Ui::RefreshScreen();
  }
}

/* static */
void Monitor::OnStart() {
  PrintString(0xff, STR_RES_START);
}

/* static */
void Monitor::OnContinue() {
  PrintString(0xff, STR_RES_CONT_);
}

/* static */
void Monitor::OnStop() {
  PrintString(0xff, STR_RES_STOP);
}

/* static */
bool Monitor::CheckChannel(uint8_t channel) {
  // channel is 0-indexed but monitored channel is 1-indexed
  return !Ui::editing() && ((monitored_channel() == 0) || (channel + 1 == monitored_channel()));
}

/* static */
void Monitor::OnRawByte(uint8_t byte) {
  if (byte != 0xfe && byte != 0xf8) {
    idle_counter_ = 0;
  }
  if (byte == 0xff) {
    PrintString(0xff, STR_RES_RESET);
  }
  App::SendNow(byte);
}

/* static */
uint8_t Monitor::OnClick() {
  Ui::Clear();
  Ui::RefreshScreen();
  return 0;
}

/* static */
uint8_t Monitor::OnRedraw() {
  if (!Ui::editing()) {
    return 1;  // Prevent the default screen redraw handler to be called.
  } else {
    return 0;
  }
}

/* static */
void Monitor::OnIdle() {
  if (idle_counter_ < 60) {
    ++idle_counter_;
    if (idle_counter_ == 60) {
      Ui::Clear();
      Ui::RefreshScreen();
    }
  }
}

} // namespace apps
} // namespace midipal
