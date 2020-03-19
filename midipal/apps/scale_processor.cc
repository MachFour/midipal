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
// Scale processor app.

#include "midipal/apps/scale_processor.h"

#include "avrlib/random.h"
#include "midi/midi_constants.h"

#include "midipal/display.h"

#include "midipal/clock.h"
#include "midipal/event_scheduler.h"
#include "midipal/note_stack.h"
#include "midipal/notes.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

enum VoiceMode {
  VOICE_MODE_OFF,
  VOICE_MODE_MIRROR,
  VOICE_MODE_ALTERNATE,
  VOICE_MODE_TRACK,
  VOICE_MODE_RANDOM,
};
  
using namespace avrlib;

const uint8_t ScaleProcessor::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0, 1, 0, -12, 0
};

/* <static> */
uint8_t ScaleProcessor::settings[Parameter::COUNT];

uint8_t ScaleProcessor::lowest_note_;
uint8_t ScaleProcessor::previous_note_;
uint8_t ScaleProcessor::voice_2_note_;
bool ScaleProcessor::flip_;
/* </static> */

/* static */
const AppInfo ScaleProcessor::app_info_ PROGMEM = {
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
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_SCALE_PROCESSOR, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_SCALE, // app_name
  true
};

/* static */
void ScaleProcessor::OnInit() {
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_ROO, UNIT_SCALE, 0, 24);
  Ui::AddPage(STR_RES_SCL, STR_RES_CHR, 0, 24);
  Ui::AddPage(STR_RES_TRS, UNIT_SIGNED_INTEGER, -24, 24);
  Ui::AddPage(STR_RES_VOI, UNIT_SIGNED_INTEGER, -24, 24);
  Ui::AddPage(STR_RES_HRM, STR_RES_OFF_, 0, 4);
  previous_note_ = 0;
  flip_ = false;
}

inline bool shouldForwardData(uint8_t status, uint8_t channel) {
  return status != noteOffFor(channel) &&
      status != noteOnFor(channel) &&
      status != polyAftertouchFor(channel);
}

/* static */
void ScaleProcessor::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  if (shouldForwardData(status, channel())) {
    App::Send(status, data, data_size);
  }
}

/* static */
void ScaleProcessor::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel == ScaleProcessor::channel()) {
    NoteStack::NoteOn(note, velocity);
    lowest_note_ = FactorizeMidiNote(NoteStack::sorted_note(0).note).note;
    ProcessNoteMessage(MIDI_NOTE_ON, note, velocity);
  }
}

/* static */
void ScaleProcessor::OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel == ScaleProcessor::channel()) {
    NoteStack::NoteOff(note);
    ProcessNoteMessage(MIDI_NOTE_OFF, note, velocity);
  }
}

/* static */
void ScaleProcessor::OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel == ScaleProcessor::channel()) {
    ProcessNoteMessage(MIDI_POLY_AFTERTOUCH, note, velocity);
  }
}

/* static */
void ScaleProcessor::ProcessNoteMessage(uint8_t message, uint8_t note, uint8_t velocity) {
  note = Transpose(note, original());
  const auto messageByte = channelMessage(message, channel());

  if (root() == 24) {
    // Dynamic root mode.
    if (message == MIDI_NOTE_ON) {
      note_map.Put(note, Constraint(note, lowest_note_, scale()));
    }
    NoteMapEntry* entry = note_map.Find(note);
    if (entry) {
      auto value = entry->value;
      App::Send3(messageByte, value, velocity);
      if (voice_1()) {
        App::Send3(messageByte, Transpose(value, voice_1()), velocity);
      }
    }
    return;
  }
  
  // Static root.
  App::Send3(messageByte, Constraint(note, root(), scale()), velocity);
  
  if (voice_1()) {
    App::Send3(messageByte, Constraint(Transpose(note, voice_1()), root(), scale()), velocity);
  }
  if (voice_2()) {
    if (message == MIDI_NOTE_ON) {
      if (previous_note_ == 0) {
        voice_2_note_ = note;
      } else {
        int16_t voice_2_note = voice_2_note_;
        /* note promoted to int16_t automatically */
        int16_t delta = note - previous_note_;
        switch (voice_2()) {
          case VOICE_MODE_MIRROR:
            voice_2_note -= delta;
            break;
          case VOICE_MODE_ALTERNATE:
            flip_ = !flip_;
            voice_2_note += flip_ ? delta : -delta;
            break;
          case VOICE_MODE_TRACK:
            voice_2_note += U8U8MulShift8(Random::GetByte(), 5) - 2;
            break;
          case VOICE_MODE_RANDOM:
            /* note promoted to int16_t automatically */
            voice_2_note = S16(note + U8U8MulShift8(Random::GetByte(), 14) - 7);
            break;
        }
        /* note promoted to int16_t automatically */
        while (voice_2_note > note + 24) {
          voice_2_note -= 12;
        }
        /* note promoted to int16_t automatically */
        while (voice_2_note < note - 24) {
          voice_2_note += 12;
        }
        voice_2_note_ = Clip(voice_2_note, 0_u8, 127_u8);
      }
      previous_note_ = note;
      voice_2_note_ = Constraint(voice_2_note_, root(), scale());
      note_map.Put(note, voice_2_note_);
    }
    
    NoteMapEntry* entry = note_map.Find(note);
    if (entry) {
      App::Send3(messageByte, entry->value, velocity);
      if (message == MIDI_NOTE_OFF) {
        entry->note = 0xff;
      }
    }
  }
}

} // namespace apps
} // namespace midipal
