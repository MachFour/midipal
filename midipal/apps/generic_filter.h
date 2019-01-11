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
// Generic filter app.

#ifndef MIDIPAL_APPS_GENERIC_FILTER_H_
#define MIDIPAL_APPS_GENERIC_FILTER_H_

#include <avrlib/bitops.h>
#include "midipal/app.h"

namespace midipal {
namespace apps{

static const uint8_t kNumModifiers = 4;

enum ValueOperation {
  VALUE_OPERATION_NOP,
  VALUE_OEPRATION_ADD,
  VALUE_OPERATION_SUBTRACT,
  VALUE_OPERATION_INVERT,
  VALUE_OPERATION_SET_TO,
  VALUE_OPERATION_MAP_TO_RANGE,
  VALUE_OPERATION_SET_TO_RANDOM,
  VALUE_OPERATION_ADD_RANDOM,
};

struct Range {
  int8_t min;
  int8_t max;
};

struct ValueTransformation {
  uint8_t operation;
  int8_t argument[2];
  
  inline uint8_t preserves_zero() const {
    return byteAnd(operation, 0x80);
  }
  
  inline uint8_t wrap() const {
    return byteAnd(operation, 0x40);
  }
  
  inline uint8_t swap_source() const {
    return byteAnd(operation, 0x20);
  }
};

struct Modifier {
  // Event filtering.
  uint16_t channel_bitmask;
  uint8_t message_type_bitmask;
  Range value[2];

  // Action / Transformation
  uint8_t action;
  uint16_t delay;
  ValueTransformation value_transformation[2];

  // maps a channel into a bitmask
  inline bool accept_channel(uint8_t channel) const {
    uint16_t channel_bit = 1;
    channel_bit <<= channel;
    return byteAnd(channel_bit, channel_bitmask);
  }

  // maps bytes with high nybble 8 -> f into a bit position
  // e.g 0x45 -> 0x00, 0x80 -> 0x01, 0x93 -> 0x02, 0xa5 -> 0x04, 0xff -> 0x80
  inline bool accept_message(uint8_t message) const {
    auto type_bits = (message >> 4u);
    // type_bits is between 0x00 and 0x0f
    if (type_bits >= 0x08) {
      auto type_bitmask = U8(1u << (type_bits - 0x08u));
      return byteAnd(type_bitmask, message_type_bitmask);
    } else { // unlikely
      return false;
    }
  }
  
  inline bool accept(uint8_t status, uint8_t data1, uint8_t data2) const {
    if (byteAnd(status, 0xf0) == 0xf0) {
      // For realtime messages, only test the message type.
      return accept_message(status);
    } else {
      return accept_channel(byteAnd(status, 0x0f)) && \
          accept_message(status) && \
          data1 >= value[0].min && \
          data1 <= value[0].max && \
          data2 >= value[1].min && \
          data2 <= value[1].max;
    }
  }
  
  inline uint8_t destroys() const {
    return byteAnd(action, 0x80);
  }
  
  inline uint8_t forwards() const {
    return byteAnd(action, 0x40);
  }
  
  inline uint8_t promotes_to_cc() const {
    return byteAnd(action, 0x20);
  }
  
  inline uint8_t remaps_channel() const {
    return byteAnd(action, 0x10);
  }
};

class GenericFilter {
 public:
  enum Parameter : uint8_t {
    active_program_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnRawMidiData(
     uint8_t status,
     uint8_t* data,
     uint8_t data_size,
     uint8_t accepted_channel);
 
  static void SetParameter(uint8_t key, uint8_t value);



 private:
  static inline void loadProgram(uint8_t num);

  static inline uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }

  static inline uint8_t& active_program() {
    return ParameterValue(active_program_);
  };

  static Modifier modifiers_[kNumModifiers];
  
  DISALLOW_COPY_AND_ASSIGN(GenericFilter);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_GENERIC_FILTER_H_
