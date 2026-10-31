# Convenience wrappers around scripts/, following esp32-bdshare's layout.
# Each target is a thin pass-through to the same command you'd type by hand.
#
# PORT passes through to the scripts, e.g.:  PORT=/dev/ttyUSB1 make deploy
# (without PORT, the first /dev/ttyUSB* / /dev/ttyACM* found is used)
#
# API targets take IP, ID and MS, e.g.:  make relay-pulse ID=3 MS=500

export PATH := $(HOME)/.local/bin:$(PATH)

PORT ?= $(firstword $(wildcard /dev/cu.usbmodem* /dev/cu.usbserial* /dev/ttyACM* /dev/ttyUSB*))
export PORT

# Board address (printed on the serial monitor at boot)
IP ?= 192.168.1.245
ID ?= 1
MS ?= 1000
CURL := curl -sS -w '\n'
JSON := -H 'Content-Type: application/json'

ESPTOOL ?= $(shell command -v esptool 2>/dev/null || ls -d $(HOME)/.arduino15/packages/esp32/tools/esptool_py/*/esptool 2>/dev/null | sort -V | tail -1)

.PHONY: help setup-env check-env compile deploy deploy-clean monitor flash flash-id backup erase clean \
        status relay relay-on relay-off relay-toggle relay-pulse all-on all-off all-toggle

help:
	@echo "Setup:"
	@echo "  make setup-env      check + install the toolchain (arduino-cli, esp32 core,"
	@echo "                       ArduinoJson), report connected boards"
	@echo "  make check-env      same checks, but report only -- installs nothing"
	@echo ""
	@echo "Build / flash (ESP32-WROOM-32E, 4MB, partitions.csv):"
	@echo "  make compile        compile into build/"
	@echo "  make deploy         compile -> upload to PORT"
	@echo "  make deploy-clean   erase whole flash -> upload (replaces an old bootloader/"
	@echo "                       OTA layout and wipes old NVS/filesystem data)"
	@echo "  make monitor        serial monitor on PORT (115200)"
	@echo "  make flash          deploy + monitor"
	@echo ""
	@echo "Device:"
	@echo "  make flash-id       chip type + flash size of the board on PORT"
	@echo "  make backup         save the board's full 4MB flash to backups/"
	@echo "  make erase          erase the whole flash (needed after changing partitions.csv)"
	@echo ""
	@echo "API (curl -> http://IP):"
	@echo "  make status         GET /relays -- state of all relays"
	@echo "  make relay          GET /relay?id=ID"
	@echo "  make relay-on       turn relay ID on"
	@echo "  make relay-off      turn relay ID off"
	@echo "  make relay-toggle   toggle relay ID"
	@echo "  make relay-pulse    relay ID on, then off after MS milliseconds"
	@echo "  make all-on         turn all relays on"
	@echo "  make all-off        turn all relays off"
	@echo "  make all-toggle     toggle all relays"
	@echo ""
	@echo "  make clean          remove build/"
	@echo ""
	@echo "PORT=$(PORT)  IP=$(IP)  ID=$(ID)  MS=$(MS)"

setup-env:
	./scripts/setup_env.sh

check-env:
	./scripts/setup_env.sh --check

compile:
	./scripts/deploy.sh --compile-only

deploy:
	./scripts/deploy.sh

deploy-clean:
	./scripts/deploy.sh --erase

monitor:
	arduino-cli monitor -p $(PORT) --config baudrate=115200

flash: deploy monitor

flash-id:
	$(ESPTOOL) --port $(PORT) flash-id

backup:
	@mkdir -p backups
	$(ESPTOOL) --port $(PORT) --baud 921600 read-flash 0 ALL backups/flash-$$(date +%Y%m%d-%H%M%S).bin

erase:
	$(ESPTOOL) --port $(PORT) erase-flash

# ---- API --------------------------------------------------------------------

status:
	$(CURL) http://$(IP)/relays

relay:
	$(CURL) "http://$(IP)/relay?id=$(ID)"

relay-on:
	$(CURL) -X POST http://$(IP)/relay $(JSON) -d '{"id": $(ID), "state": "on"}'

relay-off:
	$(CURL) -X POST http://$(IP)/relay $(JSON) -d '{"id": $(ID), "state": "off"}'

relay-toggle:
	$(CURL) -X POST http://$(IP)/relay $(JSON) -d '{"id": $(ID), "state": "toggle"}'

relay-pulse:
	$(CURL) -X POST http://$(IP)/relay $(JSON) -d '{"id": $(ID), "pulse_ms": $(MS)}'

all-on:
	$(CURL) -X POST http://$(IP)/relays $(JSON) -d '{"state": "on"}'

all-off:
	$(CURL) -X POST http://$(IP)/relays $(JSON) -d '{"state": "off"}'

all-toggle:
	$(CURL) -X POST http://$(IP)/relays $(JSON) -d '{"state": "toggle"}'

# ---- housekeeping -------------------------------------------------------------

clean:
	rm -rf build
