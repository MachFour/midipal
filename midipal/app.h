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

#ifndef MIDIPAL_APP_H_
#define MIDIPAL_APP_H_

#include "avrlib/base.h"

#include "midipal/resources.h"

namespace midipal {

enum EepromSetting : uint16_t {
  // Settings must be defined here. Please leave some space between them
  // to allow for future improvements of apps.
  SETTINGS_POLY_SEQUENCER = 24,
  SETTINGS_MONITOR = 0,
  SETTINGS_0XFE_FILTER = 8,
  SETTINGS_SYNC_LATCH = 10,
  SETTINGS_CLOCK_SOURCE = 16,
  SETTINGS_CC_KNOB = 24,
  SETTINGS_TANPURA = 48,
  SETTINGS_DRUM_PATTERN_GENERATOR = 64,
  SETTINGS_CONTROLLER = 80,
  SETTINGS_SPLITTER = 128,
  SETTINGS_RANDOMIZER = 136,
  SETTINGS_CHORD_MEMORY = 144,
  SETTINGS_DISPATCHER = 176,
  SETTINGS_COMBINER = 192,
  SETTINGS_ARPEGGIATOR = 208,
  SETTINGS_DELAY = 256,
  SETTINGS_SCALE_PROCESSOR = 272,
  SETTINGS_SEQUENCER = 288,
  SETTINGS_LFO = 512,
  SETTINGS_FILTER = 544,
  SETTINGS_CLOCK_DIVIDER = 576,
  SETTINGS_GENERIC_FILTER_PROGRAM = 580,
  SETTINGS_GENERIC_FILTER_SETTINGS = 640,
  SETTINGS_GENERIC_FILTER_SETTINGS_DUMP_AREA = 896,
  SETTINGS_APP_SELECTOR = 1008,
  SETTINGS_SYSTEM_SETTINGS = 1012
};

enum ClockMode {
  CLOCK_MODE_INTERNAL,
  CLOCK_MODE_EXTERNAL,
  CLOCK_MODE_NOTE,
};

struct AppInfo {
  void (*OnInit)();
  void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  void (*OnAftertouch)(uint8_t, uint8_t);
  void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  void (*OnProgramChange)(uint8_t, uint8_t);
  void (*OnPitchBend)(uint8_t, uint16_t);
  void (*OnSysExByte)(uint8_t);
  void (*OnClock)(uint8_t);
  void (*OnStart)();
  void (*OnContinue)();
  void (*OnStop)();
  bool (*CheckChannel)(uint8_t);
  void (*OnRawByte)(uint8_t);
  void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  uint8_t (*OnIncrement)(int8_t);
  uint8_t (*OnClick)();
  uint8_t (*OnPot)(uint8_t, uint8_t);
  uint8_t (*OnRedraw)();

  void (*SetParameter)(uint8_t, uint8_t);
  uint8_t (*GetParameter)(uint8_t);
  uint8_t (*CheckPageStatus)(uint8_t);

  // TODO uint8_t settings_size;
  uint16_t settings_size;
  uint16_t settings_offset;
  uint8_t* settings_data;
  // stored in program memory
  const uint8_t* factory_data;
  uint8_t app_name;

  // Whether the app can respond to a clock message within 50µs. Set this to
  // false if your app can generate a big bunch of MIDI messages upon the
  // reception of a single clock tick (eg: anything that may generate
  // overlapping notes, polyphony...). Set to true if your app generates only
  // a couple of MIDI messages or does nothing on clock ticks.
  bool realtime_clock_handling;
};

class App {
 public:
  static void Init();

  // Can be used to save/load settings in EEPROM.
  static void SaveSetting(uint16_t key);
  static void SaveSettings();
  static void LoadSettings();
  static void ResetToFactorySettings();
  static void Launch(uint8_t app_index);

  static void OnInit() {
    if (app_info_.OnInit) {
      app_info_.OnInit();
    }
  }
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (app_info_.OnNoteOn) {
      app_info_.OnNoteOn(channel, note, velocity);
    }
  }
  static void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (app_info_.OnNoteOff) {
      app_info_.OnNoteOff(channel, note, velocity);
    }
  }
  static void OnAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (app_info_.OnNoteAftertouch) {
      app_info_.OnNoteAftertouch(channel, note, velocity);
    }
  }
  static void OnAftertouch(uint8_t channel, uint8_t velocity) {
    if (app_info_.OnAftertouch) {
      app_info_.OnAftertouch(channel, velocity);
    }
  }
  static void OnControlChange(uint8_t channel, uint8_t cc_num, uint8_t value) {
    RemoteControl(channel, cc_num, value);
    if (app_info_.OnControlChange) {
      app_info_.OnControlChange(channel, cc_num, value);
    }
  }
  static void OnProgramChange(uint8_t channel, uint8_t program) {
    if (app_info_.OnProgramChange) {
      app_info_.OnProgramChange(channel, program);
    }
  }
  static void OnPitchBend(uint8_t channel, uint16_t pitch_bend) {
    if (app_info_.OnPitchBend) {
      app_info_.OnPitchBend(channel, pitch_bend);
    }
  }
  static void OnSysExByte(uint8_t sysex_byte) {
    if (app_info_.OnSysExByte) {
      app_info_.OnSysExByte(sysex_byte);
    } else if (!app_info_.OnRawByte) {
      // Forwarding will not be handled by OnRawByte, nor by OnRawMidiData,
      // so we do it explicitly here.
      SendNow(sysex_byte);
    }
  }

  static void OnClock(uint8_t clock_mode) {
    if (app_info_.OnClock) {
      app_info_.OnClock(clock_mode);
    }
  }
  static void OnStart() {
    if (app_info_.OnStart) {
      app_info_.OnStart();
    }
  }
  static void OnContinue() {
    if (app_info_.OnContinue) {
      app_info_.OnContinue();
    }
  }
  static void OnStop() {
    if (app_info_.OnStop) {
      app_info_.OnStop();
    }
  }

  static bool CheckChannel(uint8_t channel) {
    if (app_info_.CheckChannel) {
      return app_info_.CheckChannel(channel);
    } else {
      return true;
    }
  }

  static void OnRawByte(uint8_t byte) {
   if (app_info_.OnRawByte) {
      app_info_.OnRawByte(byte);
    }
  }
  static void OnRawMidiData(
     uint8_t status,
     uint8_t* data,
     uint8_t data_size,
     uint8_t accepted_channel) {
    if (app_info_.OnRawMidiData) {
      app_info_.OnRawMidiData(status, data, data_size, accepted_channel);
    }
  }

  // Event handlers for UI.
  static uint8_t OnIncrement(int8_t increment) {
    if (app_info_.OnIncrement) {
      return app_info_.OnIncrement(increment);
    } else {
      return 0;
    }
  }

  static uint8_t OnClick() {
    if (app_info_.OnClick) {
      return app_info_.OnClick();
    } else {
      return 0;
    }
  }
  static uint8_t OnPot(uint8_t pot, uint8_t value) {
    if (app_info_.OnPot) {
      return app_info_.OnPot(pot, value);
    } else {
      return 0;
    }
  }
  static uint8_t OnRedraw() {
    if (app_info_.OnRedraw) {
      return app_info_.OnRedraw();
    } else {
      return 0;
    }
  }

  // Parameter and page checking.
  static void SetParameter(uint8_t key, uint8_t value) {
    if (key >= num_parameters()) {
      // out of bounds, do nothing
      return;
    } else if (app_info_.SetParameter) {
      app_info_.SetParameter(key, value);
    } else {
      settings_data()[key] = value;
    }
  }
  static uint8_t GetParameter(uint8_t key) {
    if (key >= num_parameters()) {
      // out of bounds, try to return something loud
      return -1;
    } else if (app_info_.GetParameter) {
      return app_info_.GetParameter(key);
    } else {
      return settings_data()[key];
    }
  }

  static uint8_t CheckPageStatus(uint8_t index) {
    if (app_info_.CheckPageStatus) {
      return app_info_.CheckPageStatus(index);
    } else {
      return 1;
    }
  }

  // Access to settings data structure
  static inline uint8_t num_parameters() {
    // TODO record number parameters distinctly from size
    // OR each parameter stores its own size
    return static_cast<uint8_t>(settings_size());
  }
  static inline uint16_t settings_size() { return app_info_.settings_size; }
  static inline uint16_t settings_offset() { return app_info_.settings_offset; }
  static inline uint8_t* settings_data() { return app_info_.settings_data; }
  // in PROGMEM
  static inline const uint8_t* factory_data() { return app_info_.factory_data; }
  static inline uint8_t app_name() { return app_info_.app_name; }
  static inline bool realtime_clock_handling() {
    return app_info_.realtime_clock_handling;
  }

  static void FlushOutputBuffer(uint8_t size);
  static void SendNow(uint8_t byte);
  static void Send(uint8_t status, uint8_t* data, uint8_t size);
  static void Send3(uint8_t a, uint8_t b, uint8_t c);
  static void SendLater(uint8_t note, uint8_t velocity, uint8_t when) {
    SendLater(note, velocity, when, 0);
  }
  static void SendLater(uint8_t note, uint8_t velocity, uint8_t when, uint8_t tag);
  static void SendScheduledNotes(uint8_t channel);
  static void FlushQueue(uint8_t channel);

  static uint8_t num_apps();

  static bool NoteClock(bool on, uint8_t channel, uint8_t note);

 private:
  static void RemoteControl(uint8_t channel, uint8_t cc_num, uint8_t value);

  static AppInfo app_info_;

  DISALLOW_COPY_AND_ASSIGN(App);
};

extern const uint8_t midi_clock_tick_per_step[] PROGMEM;

}  // namespace midipal

#endif // MIDIPAL_APP_H_
