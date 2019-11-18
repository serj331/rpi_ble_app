CC:=arm-linux-gnueabihf-gcc
CFLAGS:=-O2 -Wall -DNDEBUG -Ilibs/include/glib-2.0 -Ilibs/include -I./
LDFLAGS:=-Llibs -lgio-2.0 -lz -lglib-2.0 -lgobject-2.0 -lgmodule-2.0 -lffi -pthread

all: src/main.o src/bcm_gpio.o src/bcm_pwm.o
	${CC} ${LDFLAGS} src/main.o src/bcm_gpio.o src/bcm_pwm.o -o build_out/ble_app
clean:
	rm -rf src/*.o
	rm -rf build_out/*
