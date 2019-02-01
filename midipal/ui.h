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

#ifndef MIDIPAL_UI_H_
#define MIDIPAL_UI_H_

#include <avrlib/ui/event_queue.h>
#include "avrlib/base.h"

#include "avrlib/devices/pot_scanner.h"
#include "avrlib/devices/rotary_encoder.h"
#include "avrlib/devices/switch.h"
#include "avrlib/ui/event_queue.h"

#include "midipal/hardware_config.h"

namespace midipal {

struct PageDefinition {
  uint8_t key_res_id;
  uint8_t value_res_id;
  uint8_t min;
  uint8_t max;
};

// the first one represents which index the PageDefinition is stored in
// the second one represents which one of a group of repeated pages it is
struct PagePosition {
  uint8_t page_def_index;
  uint8_t repeat_index;
};

enum Unit {
  UNIT_INTEGER,
  UNIT_INDEX,
  UNIT_NOTE,
  UNIT_SCALE,
  UNIT_INTEGER_ALL,
  UNIT_SIGNED_INTEGER,
  UNIT_CHANNEL
};

enum PageScrollingStatus {
  PAGE_BAD = 0,
  PAGE_GOOD = 1,
  PAGE_LAST = 2
};

class Ui {
 public:
  static void Init();
  static void Poll();
  static void DoEvents();
  
  static void Clear();

  static void PrintKeyValuePair( uint8_t key_res_id, uint8_t index,
      uint8_t value_res_id, uint8_t value, bool editing );
  static void PrintChannel(char* buffer, uint8_t channel);
  static void PrintHex(char* buffer, uint8_t value);
  static void PrintNote(char* buffer, uint8_t note, bool flat);
  static void PrintNote(char* buffer, uint8_t note) {
    PrintNote(buffer, note, false);
  }
  static void PrintString(uint8_t res_id);

  static void RefreshScreen();
  
  static inline void set_read_pots(uint8_t value) {
    read_pots_ = value;
  }
  // warning: repeated pages (repeats > 1) must be added only after all others
  static void AddPage(uint8_t key_res_id, uint8_t value_res_id, uint8_t min, uint8_t max, uint8_t repeats = 1);
  // deprecate
  static void AddRepeatedPage(uint8_t key_res_id, uint8_t value_res_id, uint8_t min, uint8_t max, uint8_t num_repetitions) {
    AddPage(key_res_id, value_res_id, min, max, num_repetitions);
  };

  static void AddClockPages();

  static bool editing() { return editing_; }
  static uint8_t page() { return current_page_; }
  static uint8_t max_page_index() {
    return num_pages_ - 1_u8;
  };
  static void set_page(uint8_t page) { current_page_ = page; }

  static void scrollPage(bool forward);

  // Returns page information struct for a given page position
  static const PageDefinition& page_definition(uint8_t offset) {
    auto pos = page_position(offset);
    return page_definition(pos);
  }

  static const PageDefinition& page_definition(const PagePosition& p) {
    return page_info_[p.page_def_index];
  }

  // For repeated pages, information is stored only for the first one,
  // so we need to calculate its index based on the number of unique pages
  // and the stride (how many sets of repeated pages there are)
  static const PagePosition page_position(uint8_t page) {
    PagePosition pos{page, 0};
    if (num_stored_page_defs_ == 0) {
      // return a 'null' value
      pos.page_def_index = 0;
    } else if (repeated_page_groups_ > 0) {
      // if all pages are unique, we are done. Otherwise, iterate
      // backwards, jumping between pages of the same type
      // until we reach an index that was actually defined
      // This is why we can only add repeated pages at the end
      while (pos.page_def_index >= num_stored_page_defs_) {
        pos.page_def_index -= repeated_page_groups_;
        pos.repeat_index++;
      }
    }
    return pos;
  }
  
 private:
  typedef avrlib::RotaryEncoder<EncoderALine, EncoderBLine, EncoderClickLine> Encoder;
  typedef avrlib::PotScanner<8, 0, 8, 7> Pots;
  typedef avrlib::EventQueue<32> Queue;

  static void increment_parameter(int8_t inc, uint8_t page);
  static void handle_encoder_click(uint8_t value, uint8_t page);

  static uint16_t encoder_hold_time_;
  static uint8_t num_stored_page_defs_;
  static uint8_t num_pages_;
  static uint8_t repeated_page_groups_;
  static uint8_t current_page_;
  static bool editing_;
  static uint8_t read_pots_;
  static uint8_t pot_value_[8];
  static PageDefinition page_info_[48];
};

}  // namespace midipal

#endif // MIDIPAL_UI_H_
