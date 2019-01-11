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

#include "avrlib/gpio.h"
#include "avrlib/boot.h"
#include "avrlib/serial.h"
#include "avrlib/time.h"
#include "avrlib/timer.h"
#include "avrlib/watchdog_timer.h"

#include "midi/midi.h"
#include "midipal/app.h"
#include "midipal/apps/settings.h"
#include "midipal/clock.h"
#include "midipal/event_scheduler.h"
#include "midipal/midi_handler.h"
#include "midipal/note_stack.h"
#include "midipal/resources.h"
#include "midipal/ui.h"

using namespace avrlib;
using namespace midi;
using namespace midipal;

// Midi input.
using MidiIo = Serial<MidiPort, 31250, POLLED, POLLED>;
MidiStreamParser<MidiHandler> midi_parser;

volatile uint8_t num_clock_ticks = 0;

inline int freeRam() {
  extern int __heap_start, *__brkval;
  uint8_t v;
  if (__brkval /* > 0 */) {
    return (int)(&v) - (int)__brkval;
  } else {
    return (int)(&v) - (int)&__heap_start;
  }
}

ISR(TIMER2_OVF_vect, ISR_NOBLOCK) {
  static uint8_t sub_clock;

  if (MidiIo::readable()) {
    uint8_t byte = MidiIo::ImmediateRead();
    if (byte != 0xfe || !apps::Settings::filter_active_sensing()) {
      LedIn::High();
      midi_parser.PushByte(byte);
    }
  }
  
  // 4kHz
  if (MidiHandler::OutputBuffer::readable() && MidiIo::writable()) {
    LedOut::High();
    MidiIo::Overwrite(MidiHandler::OutputBuffer::ImmediateRead());
  }
  
  while (num_clock_ticks) {
    --num_clock_ticks;
    App::OnClock(CLOCK_MODE_INTERNAL);
  }

  if (freeRam() <= 10) {
    LedIn::High();
    LedOut::High();
  }
  
  sub_clock = byteAnd(sub_clock + 1, 3);
  if (byteAnd(sub_clock, 1) == 0) {
    // 2kHz
    Ui::Poll();
    if (byteAnd(sub_clock, 3) == 0) {
      TickSystemClock();
      LedOut::Low();
      LedIn::Low();
    }
  }
}

ISR(TIMER1_COMPA_vect, ISR_BLOCK) {
  PwmChannel1A::set_frequency(Clock::Tick());
  if (Clock::running()) {
    if (App::realtime_clock_handling()) {
      App::OnClock(CLOCK_MODE_INTERNAL);
    } else {
      ++num_clock_ticks;
    }
  }
}

void Init() {
  sei();
  UCSR0B = 0;
  
  LedOut::set_mode(DIGITAL_OUTPUT);
  LedIn::set_mode(DIGITAL_OUTPUT);
  
  note_stack.Init();
  event_scheduler.Init();
  
  // Boot the settings app.
  App::Launch(App::num_apps() - 1_u8);
  App::LoadSettings();
  
  // Boot the app selector app.
  App::Launch(0);
  App::Init();

  // Now that the app selector app has booted, we can retrieve the
  // app to launch.
  auto launch_app = App::GetParameter(0);
  if (launch_app >= App::num_apps()) {
    launch_app = 0;
    App::SetParameter(0, launch_app);
  }
  App::Launch(launch_app);
  
  Ui::Init();
  Clock::Init();
  
  // Configure the timers.
  Timer<1>::set_prescaler(1);
  Timer<1>::set_mode(0, bitFlag(WGM12), 3);
  PwmChannel1A::set_frequency(6510);
  Timer<1>::StartCompare();
  
  Timer<2>::set_prescaler(2);
  Timer<2>::set_mode(TIMER_PWM_PHASE_CORRECT);
  Timer<2>::Start();
  App::Init();
  MidiIo::Init();
}

int main() {
  ResetWatchdog();
  Init();
  while (true) {
    Ui::DoEvents();
  }
}
