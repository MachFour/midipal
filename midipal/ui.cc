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

#include "midipal/ui.h"

#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midipal/app.h"
#include "midipal/display.h"
#include "midipal/resources.h"
#include "midipal/apps/settings.h"
#include "midipal/apps/monitor.h"

namespace midipal {

using namespace avrlib;

/* <static> */
PageDefinition Ui::page_info_[48];
uint8_t Ui::num_stored_page_defs_;
uint8_t Ui::num_pages_;
uint8_t Ui::current_page_;
uint8_t Ui::repeated_page_groups_;
uint8_t Ui::pot_value_[8];
bool Ui::editing_;
uint8_t Ui::read_pots_;
uint16_t Ui::encoder_hold_time_;
/* </static> */

/* extern */
Ui ui;

/* static */
void Ui::Init() {
  Encoder::Init();
  Pots::Init();
  Lcd::Init();
  Display::Init();
  read_pots_ = 0;
  num_stored_page_defs_ = 0;
  num_pages_ = 0;
  repeated_page_groups_ = 0;
  current_page_ = 0;
  editing_ = false;
  encoder_hold_time_ = 0;
}

/* static */
void Ui::AddPage(uint8_t key_res_id, uint8_t value_res_id, uint8_t min, uint8_t max, uint8_t repeats) {
  if (repeats > 0) {
    auto new_index = num_stored_page_defs_;
    page_info_[new_index].key_res_id = key_res_id;
    page_info_[new_index].value_res_id = value_res_id;
    page_info_[new_index].min = min;
    page_info_[new_index].max = max;
    ++num_stored_page_defs_;
    num_pages_ += repeats;
    if (repeats > 1) {
      // instead of storing a whole new page definition for each repeat
      // just record how many sets of repeated pages there are. Then we
      // can just jump between them
      ++repeated_page_groups_;
    }
  }
}

/* static */
void Ui::AddClockPages() {
  Ui::AddPage(STR_RES_CLK, STR_RES_INT, 0, 2);
  Ui::AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  Ui::AddPage(STR_RES_GRV, STR_RES_SWG, 0, 5);
  Ui::AddPage(STR_RES_AMT, UNIT_INTEGER, 0, 127);
}

/* static */
void Ui::Poll() {
  int8_t increment = Encoder::Read();
  if (increment != 0) {
    Queue::AddEvent(CONTROL_ENCODER, 0, increment);
  }
  if (Encoder::immediate_value() == 0x00) {
    ++encoder_hold_time_;
    if (encoder_hold_time_ > 800) {
      Queue::AddEvent(CONTROL_ENCODER_CLICK, 0, 0xff);
    }
  }
  if (Encoder::clicked()) {
    // Do not enqueue a click event when the encoder is released after a long
    // press.
    if (encoder_hold_time_ <= 800) {
      Queue::AddEvent(CONTROL_ENCODER_CLICK, 0, 1);
    }
    encoder_hold_time_ = 0;
  }
  if (read_pots_) {
    Pots::Read();
    uint8_t index = Pots::last_read();
    uint8_t value = U8(Pots::value(index));
    if (value != pot_value_[index]) {
      pot_value_[index] = value;
      Queue::AddEvent(CONTROL_POT, index, value);
    }
  }
  Lcd::Tick();
}

/* We increment the page until either we reach a good page,
 * in which case set the current page to that one, or we reach the
 * limit, without seeing a good one, in which case don't scroll
 */
void Ui::scrollPage(bool forward) {
  const uint8_t limit = forward ? max_page_index() : 0_u8;
  const uint8_t increment = U8(forward ? 1 : -1);
  uint8_t newPage = page();

  while (newPage != limit) {
    newPage += increment;
    if (App::CheckPageStatus(newPage) == PAGE_GOOD) {
      // found a page to jump to
      set_page(newPage);
      break;
    }
  }
  // else there were no more valid pages before the limit
  // so don't do anything
}

void Ui::increment_parameter(int8_t inc, uint8_t page) {
    auto page_def = page_definition(page);
    if (page_def.value_res_id == UNIT_SIGNED_INTEGER) {
      auto old_val = S8(App::GetParameter(page));
      auto min = S8(page_def.min);
      auto max = S8(page_def.max);
      auto new_val = S8(Clip(old_val + inc, min, max));
      App::SetParameter(page, U8(new_val));
    } else { // UNSIGNED INTEGER
      auto old_val = App::GetParameter(page);
      auto min = page_def.min;
      auto max = page_def.max;
      auto new_val = U8(Clip(old_val + inc, min, max));
      App::SetParameter(page, new_val);
    }
}

void Ui::handle_encoder_click(uint8_t value, uint8_t page) {
  if (value == 1) {
    if (!App::OnClick()) {
      const auto& page_def = page_definition(page);
      if (page_def.value_res_id == STR_RES_OFF) {
        App::SetParameter(page, App::GetParameter(page) ? 0_u8 : 1_u8);
        App::SaveSetting(page);
      } else {
        editing_ = !editing();
        if (!editing()) {
          // Left the editing mode, save settings.
          App::SaveSetting(page);
        }
      }
    }
  } else {
    App::Launch(0);
  }
}

/* static */
void Ui::DoEvents() {
  bool redraw = false;
  while (Queue::available()) {
    Queue::Touch();

    redraw = true;
    Event e = Queue::PullEvent();

    switch (e.control_type) {
      case CONTROL_ENCODER:
        if (App::OnIncrement(e.value)) {
          // do nothing; app handled the event already
        } else if (editing()) {
          increment_parameter(e.value, current_page_);
        } else {
          bool scrollForward = (e.value == 1);
          scrollPage(scrollForward);
        }
        break;
      case CONTROL_ENCODER_CLICK:
        handle_encoder_click(e.value, current_page_);
        break;
      case CONTROL_POT:
        App::OnPot(e.control_id, e.value);
        break;
      default:
        break;
    }
  }
  
  if (Queue::idle_time_ms() > 50) {
    redraw = true;
    Queue::Touch();
    if (App::app_name() == STR_RES_MONITOR) {
      apps::Monitor::OnIdle();
    }
  }
  
  if (redraw && !App::OnRedraw()) {
    const auto page_pos = page_position(current_page_);
    const auto page_def = page_definition(page_pos);
    PrintKeyValuePair(page_def.key_res_id, page_pos.repeat_index,
          page_def.value_res_id, App::GetParameter(current_page_), editing_);
  }
  Display::Tick();
}

static const char note_names[] PROGMEM =      " CC# DD# E FF# GG# AA# B";
static const char note_names_flat[] PROGMEM = " CDb DEb E FGb GAb ABb B";
static const char octaves[] PROGMEM = "-012345678";

/* static */
void Ui::PrintKeyValuePair(uint8_t key_res_id, uint8_t index,
    uint8_t value_res_id, uint8_t value, bool editing) {
  memset(line_buffer, ' ', kLcdWidth);
  if (key_res_id == UNIT_CHANNEL) {
    line_buffer[0] = 'c';
    line_buffer[1] = 'h';
    PrintChannel(&line_buffer[2], index);
  } else {
    ResourcesManager::LoadStringResource(key_res_id, &line_buffer[0], 3);
  }
  AlignRight(&line_buffer[0], 3);
  
  // This is a hack to add the index id on the step sequencer pages.
  if (line_buffer[2] < 0x20 && line_buffer[1] == ' ') {
    UnsafeItoa(index + 1, 2, &line_buffer[0]);
    PadRight(&line_buffer[0], 2, '0');
  }
  
  switch (value_res_id) {
    case UNIT_INTEGER:
    case UNIT_INTEGER_ALL:
      UnsafeItoa(value, 3, &line_buffer[4]);
      if (value == 0 && value_res_id == UNIT_INTEGER_ALL) {
          ResourcesManager::LoadStringResource(STR_RES_ALL, &line_buffer[4], 3);
      }
      break;
    case UNIT_SIGNED_INTEGER:
      UnsafeItoa(static_cast<int8_t>(value), 3, &line_buffer[4]);
      break;
    case UNIT_INDEX:
      UnsafeItoa(value + 1, 3, &line_buffer[4]);
      break;
    case UNIT_NOTE:
      PrintNote(&line_buffer[4], value, false);
      break;
    case UNIT_SCALE:
      if (value < 12) {
        PrintNote(&line_buffer[4], value, false);
        line_buffer[6] = ' ';
      } else if (value < 24) {
        PrintNote(&line_buffer[4], value, true);
        line_buffer[6] = ' ';
      } else {
        line_buffer[4] = 'k';
        line_buffer[5] = 'e';
        line_buffer[6] = 'y';
      }
      break;
    default:
      ResourcesManager::LoadStringResource(value_res_id + value, &line_buffer[4], 3);
      break;
  }
  AlignRight(&line_buffer[4], 3);
  if (editing) {
    line_buffer[3] = '[';
    line_buffer[7] = ']';
  }
  Display::Print(0, line_buffer);
}

/* static */
void Ui::PrintString(uint8_t res_id) {
  ResourcesManager::LoadStringResource(res_id, &line_buffer[0], 8);
  AlignRight(&line_buffer[0], 8);
  Display::Print(0, line_buffer);
}


/* static */
void Ui::Clear() {
  memset(line_buffer, ' ', kLcdWidth);
}

/* static */
void Ui::PrintChannel(char* buffer, uint8_t channel) {
  if (channel == 0xff) {
    *buffer = ' ';
  } else if (channel < 9) {
    *buffer = '1' + channel;
  } else {
    *buffer = channel - 8_u8;
  }
}

/* static */
void Ui::PrintHex(char* buffer, uint8_t value) {
  *buffer++ = NibbleToAscii(U8ShiftRight4(value));
  *buffer = NibbleToAscii(byteAnd(value, 0xf));
}

/* static */
void Ui::PrintNote(char* buffer, uint8_t note, bool flat) {
  uint8_t octave = 0;
  while (note >= 12) {
    ++octave;
    note -= 12;
  }
  const char* const table = flat ? note_names_flat : note_names;
  *buffer++ = ResourcesManager::Lookup<char, uint8_t>(table, note << 1u);
  *buffer++ = ResourcesManager::Lookup<char, uint8_t>(table, U8(1 + (note << 1u)));
  *buffer = ResourcesManager::Lookup<char, uint8_t>(octaves, octave);
}

/* static */
void Ui::RefreshScreen() {
  Display::Print(0, line_buffer);
}

}  // namespace midipal
