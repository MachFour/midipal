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
// Setup app.

#include "midipal/apps/app_selector.h"

#include "avrlib/string.h"
#include "avrlib/watchdog_timer.h"

#include "midi/midi.h"
#include "midi/midi_constants.h"

#include "midipal/display.h"
#include "midipal/sysex_handler.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps {

using namespace avrlib;

/* static */
uint8_t AppSelector::settings[Parameter::COUNT];
/* static */
uint8_t AppSelector::selected_item_;


/* static */
const AppInfo AppSelector::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  nullptr, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
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
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  nullptr, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);
  &OnIncrement, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  &OnRedraw, // uint8_t (*OnRedraw)();
  nullptr, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  1, // settings_size
  SETTINGS_APP_SELECTOR, // settings_offset
  settings, // settings_data
  nullptr, // factory_data
  0, // app_name
  true
};

/* static */
void AppSelector::OnInit() {
  selected_item_ = active_app();
  if (selected_item_ == 0) {
    selected_item_ = 1;
  }
}

/* static */
void AppSelector::OnRawByte(uint8_t byte) {
  App::SendNow(byte);
}

/* static */
uint8_t AppSelector::OnClick() {
  if (selected_item_ == App::num_apps()) {
    // Note nuke.
    for (uint8_t channel = 0; channel < 16; ++channel) {
      for (uint8_t note = 0; note < 128; ++note) {
        App::Send3(noteOffFor(channel), note, 0);
      }
    }
    return 1;
  } else if (selected_item_ == App::num_apps() + 1) {
    // Backup
    sysex_handler.SendBlock(nullptr, 0);
  } else if (selected_item_ == App::num_apps() + 2) {
    // Factory reset.
    for (uint8_t i = 1; i < App::num_apps(); ++i) {
      App::Launch(i);
      App::ResetToFactorySettings();
    }
    App::Launch(0);
    active_app() = 0;
  } else {
    active_app() = selected_item_;
  }
  App::SaveSettings();
  SystemReset(100);
  return 1;
}

/* static */
uint8_t AppSelector::OnIncrement(int8_t increment) {
  selected_item_ += increment;
  if (selected_item_ < 1) {
    selected_item_ = 1;
  } else if (selected_item_ > App::num_apps() + 3_u8) {
    selected_item_ = App::num_apps() + 3_u8;
  }
  return 1;
}

/* static */
uint8_t AppSelector::OnRedraw() {
  Ui::Clear();
  if (selected_item_ >= App::num_apps()) {
    ResourcesManager::LoadStringResource(
        STR_RES_NOTENUKE + selected_item_ - App::num_apps(),
        &line_buffer[0], 8);
  } else {
    App::Launch(selected_item_);
    ResourcesManager::LoadStringResource(App::app_name(), &line_buffer[0], 8);
  }
  App::Launch(0);
  AlignLeft(&line_buffer[0], 8);
  Ui::RefreshScreen();
  return 1;
}

} // namespace apps
} // namespace midipal
