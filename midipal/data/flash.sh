#! /bin/sh
avrdude -B 100 -V -p m328p -c usbasp -P usb -e -u -U efuse:w:0xfd:m -U hfuse:w:0xd4:m -U lfuse:w:0xff:m -U lock:w:0x2f:m
avrdude -B 1 -V -p m328p -c usbasp -P usb -U flash:w:midipal\_flash\_golden_v1.4.hex:i -U eeprom:w:midipal\_eeprom\_golden_v1.4.hex:i -U lock:w:0x2f:m
