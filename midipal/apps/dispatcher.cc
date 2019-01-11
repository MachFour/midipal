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
// Dispatcher app.

#include "midipal/apps/dispatcher.h"

#include "avrlib/random.h"

#include "midi/midi.h"

#include "midipal/notes.h"
#include "midipal/ui.h"
#include "midipal/voice_allocator.h"

namespace midipal {
namespace apps {

using namespace avrlib;

/* static */
const uint8_t Dispatcher::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0, 0, 3, 1
};

/* static */
uint8_t Dispatcher::settings[Parameter::COUNT];

/* static */
uint8_t Dispatcher::counter_;

/* static */
const AppInfo Dispatcher::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  &OnNoteOff, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  &OnNoteAftertouch, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnAftertouch)(uint8_t, uint8_t);
  nullptr, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnProgramChange)(uint8_t, uint8_t);
  nullptr, // void (*OnPitchBend)(uint8_t, uint16_t);
  nullptr, // void (*OnSysExByte)(uint8_t);
  nullptr, // void (*OnClock)();
  nullptr, // void (*OnStart)();
  nullptr, // void (*OnContinue)();
  nullptr, // void (*OnStop)();
  nullptr, // bool *(CheckChannel)(uint8_t);
  nullptr, // void (*OnRawByte)(uint8_t);
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_DISPATCHER, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_DISPATCH, // app_name
  true
};

/* static */
void Dispatcher::OnInit() {
  VoiceAllocator::Init();
  Ui::AddPage(STR_RES_INP, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_MOD, STR_RES_CYC, 0, 4);
  Ui::AddPage(STR_RES_OUT, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_NUM, UNIT_INTEGER, 1, 16);
  Ui::AddPage(STR_RES_POL, UNIT_INTEGER, 1, 8);
  counter_ = 0;
}

/* static */
uint8_t Dispatcher::map_channel(uint8_t index) {
  if (polyphony_voices() == 0) {
    polyphony_voices() = 1;
  }
  uint8_t channel = 0;
  while (index >= polyphony_voices()) {
    ++channel;
    index -= polyphony_voices();
  }
  return byteAnd(base_channel() + channel, 0xf);
}

/* static */
void Dispatcher::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  uint8_t type = byteAnd(status, 0xf0);
  uint8_t channel = byteAnd(status, 0x0f);
  if (type == 0xf0 || channel != input_channel()) {
    // Pass-through real time messages and messages on other channels.
    App::Send(status, data, data_size);
  } else if (type != 0x80 && type != 0x90 && type != 0xa0) {
    // Forward the global messages to all channels.
    for (uint8_t i = 0; i < num_voices(); i += polyphony_voices()) {
      App::Send(byteOr(type, map_channel(i)), data, data_size);
    }
  }
  // That's it, the note specific messages have dedicated handlers.
}

/* static */
void Dispatcher::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel != input_channel()) {
    return;
  }
  switch (mode()) {
    case DISPATCHER_CYCLIC:
      ++counter_;
      if (counter_ >= num_voices()) {
        counter_ = 0;
      }
      note_map.Put(note, counter_);
      break;

    case DISPATCHER_POLYPHONIC_ALLOCATOR:
      voice_allocator.set_size(num_voices());
      note_map.Put(note, voice_allocator.NoteOn(note));
      break;
      
    case DISPATCHER_RANDOM:
      note_map.Put(note, Random::GetByte() % num_voices());
      break;
    
    case DISPATCHER_STACK:
      note_map.Put(note, 0xff);
      break;
      
    case DISPATCHER_VELOCITY:
      note_map.Put(note, U8U8MulShift8(velocity << 1, num_voices()));
      break;
  }
  SendMessage(0x90, channel, note, velocity);
}

/* static */
void Dispatcher::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel != input_channel()) {
    return;
  }
  VoiceAllocator::NoteOff(note);
  SendMessage(0x80, channel, note, velocity);
}

/* static */
void Dispatcher::OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel == input_channel()) {
    SendMessage(0xa0, channel, note, velocity);
  }
}

/* static */
void Dispatcher::SendMessage(uint8_t message, uint8_t channel, uint8_t note, uint8_t velocity) {
  NoteMapEntry* entry = note_map.Find(note);
  if (entry) {
    if (entry-> value == 0xff) {
      for (uint8_t i = 0; i < num_voices(); i += polyphony_voices()) {
        App::Send3(byteOr(message, map_channel(i)), note, velocity);
      }
    } else {
      App::Send3(byteOr(message, map_channel(entry->value)), note, velocity);
    }
    if (message == 0x80) {
      entry->note = 0xff;
    }
  }
}

}  // namespace apps
}  // namespace midipal
