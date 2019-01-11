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
// This app is responsible for configuring which app is active.

#ifndef MIDIPAL_APPS_APP_SELECTOR_H_
#define MIDIPAL_APPS_APP_SELECTOR_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class AppSelector {
 public:

  enum Parameter : uint8_t {
    active_app_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];

  static void OnInit();
  static void OnRawByte(uint8_t byte);
  
  static uint8_t OnClick();
  static uint8_t OnIncrement(int8_t increment);
  static uint8_t OnRedraw();
  
  static const AppInfo app_info_ PROGMEM;

  static inline uint8_t& active_app() {
    return ParameterValue(active_app_);
  };

 private:
  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static uint8_t selected_item_;

  DISALLOW_COPY_AND_ASSIGN(AppSelector);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_APP_SELECTOR_H_
