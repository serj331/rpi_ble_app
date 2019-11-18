simple bluetooth low energy application writed on c for rasspberry pi zero w

ble application register and implements: 1 custom local gatt service with 2 characteristics in it (charc0 for led switch, charc1 for pwm control of second led), advertiser object for device name & local service uuid adversments

application communicate with bluez stack through dbus, protocol handling performed by using glib

build:
CC=arm-linux-gnueabihf-gcc make

