
#ifndef AVRLIB_MIDI_CONSTANTS_H_
#define AVRLIB_MIDI_CONSTANTS_H_

#include "avrlib/bitops.h"

/* source: https://www.midimountain.com/midi/midi_status.htm */
static constexpr uint8_t MIDI_NOTE_OFF = 0x80;
static constexpr uint8_t MIDI_NOTE_ON = 0x90;
static constexpr uint8_t MIDI_POLY_AFTERTOUCH = 0xa0;
static constexpr uint8_t MIDI_CONTROL_CHANGE = 0xb0;
static constexpr uint8_t MIDI_PROGRAM_CHANGE = 0xc0;
static constexpr uint8_t MIDI_CHAN_AFTERTOUCH = 0xd0;
static constexpr uint8_t MIDI_PITCH_WHEEL = 0xe0;
static constexpr uint8_t MIDI_SYSEX = 0xf0;
static constexpr uint8_t MIDI_SYSEX_END = 0xf7;
static constexpr uint8_t MIDI_SYS_CLK_TICK = 0xf8;
static constexpr uint8_t MIDI_SYS_CLK_START = 0xfa;
static constexpr uint8_t MIDI_SYS_CLK_CONT = 0xfb;
static constexpr uint8_t MIDI_SYS_CLK_STOP = 0xfc;
static constexpr uint8_t MIDI_SYS_CLK_ACTIVE_SENSING = 0xfe;
static constexpr uint8_t MIDI_SYS_CLK_RESET = 0xff;

static constexpr uint8_t channelMask(uint8_t channel) {
  return byteAnd(channel, 0x0f);
}

static constexpr uint8_t channelMessage(uint8_t type, uint8_t channel) {
  return byteOr(type, channelMask(channel));
}

static constexpr uint8_t noteOffFor(uint8_t channel) {
  return byteOr(MIDI_NOTE_OFF, channelMask(channel));
}

static constexpr uint8_t noteOnFor(uint8_t channel) {
  return byteOr(MIDI_NOTE_ON, channelMask(channel));
}
static constexpr uint8_t polyAftertouchFor(uint8_t channel) {
  return byteOr(MIDI_POLY_AFTERTOUCH, channelMask(channel));
}

static constexpr uint8_t controlChangeFor(uint8_t channel) {
  return byteOr(MIDI_CONTROL_CHANGE, channelMask(channel));
}

static constexpr uint8_t progChangeFor(uint8_t channel) {
  return byteOr(MIDI_PROGRAM_CHANGE, channelMask(channel));

}


#endif //AVRLIB_MIDI_CONSTANTS_H_
