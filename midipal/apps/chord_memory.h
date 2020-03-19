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
// Chord memory app.

#ifndef MIDIPAL_APPS_CHORD_MEMORY_H_
#define MIDIPAL_APPS_CHORD_MEMORY_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

static const uint8_t kMaxChordNotes = 10;

class ChordMemory {
 public:
  enum Parameter : uint8_t {
    channel_,
    num_notes_,
    chord_data_,
    chord_data_end_ = chord_data_ + kMaxChordNotes,
    COUNT = chord_data_end_
  };

  static uint8_t settings[Parameter::COUNT];

  static void OnInit();
  static void OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size);
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  static void OnNoteAftertouch(uint8_t channel, uint8_t note, uint8_t velocity);
  static uint8_t OnClick();
  
  static const AppInfo app_info_ PROGMEM;
  
 private:
  static void PlayChord(uint8_t type, uint8_t channel, uint8_t note, uint8_t velocity);

  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static inline uint8_t& channel() {
    return ParameterValue(channel_);
  }
  static inline uint8_t& num_notes() {
    return ParameterValue(num_notes_);
  }
  static inline uint8_t* chord_data() {
      return &settings[chord_data_];
  }

  // whether the given channel matches the currently set one
  static inline bool isActiveChannel(uint8_t channel);

  //static uint8_t root_;
  
  DISALLOW_COPY_AND_ASSIGN(ChordMemory);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_CHORD_MEMORY_H_
