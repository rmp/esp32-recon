# WiFi Network Diagnostics — ESP32E build targets
#
# Usage:
#   make              Build firmware
#   make upload       Build and flash to device
#   make monitor      Open serial monitor
#   make deploy       Build, flash, and monitor
#   make test         Run native unit tests (no hardware needed)
#   make size         Show RAM/flash usage
#   make erase        Erase entire flash
#   make clean        Remove build artifacts
#   make buildfs      Build SPIFFS filesystem image
#   make uploadfs     Upload filesystem image
#   make fuse         Build, flash firmware + filesystem

.PHONY: all build upload monitor deploy test size erase clean buildfs uploadfs fuse

all: build

build:
	pio run -e esp32

upload:
	pio run --target upload

monitor:
	pio device monitor

deploy: upload monitor

test:
	pio test -e native

size:
	pio run --target size

erase:
	pio run --target erase

clean:
	pio run --target clean

buildfs:
	pio run --target buildfs

uploadfs:
	pio run --target uploadfs

fuse: upload uploadfs monitor
