# GBS-Control build shortcuts (GNU Make)
#
# Prerequisites:
#   - PlatformIO CLI (`pio`) on PATH
#   - Node.js + npm (for web UI)
#   - bash, gzip, xxd for `make webui` (Git Bash / WSL / Linux / macOS)
#
# Examples:
#   make                  # web UI + firmware (default board: d1_mini)
#   make firmware-only    # skip web UI rebuild
#   make upload BOARD=esp32dev
#   make help

BOARD   ?= d1_mini
PIO     ?= pio
NPM     ?= npm
PUBLIC  := public

# PlatformIO environments from platformio.ini
BOARDS  := d1_mini esp32dev esp32-s3-devkitc-1 esp32-c3-devkitm-1 esp32-c6-devkitc-1

.PHONY: help all submodules webui firmware firmware-only upload monitor clean clean-all \
	esp8266 esp32 esp32-s3 esp32-c3 esp32-c6

help:
	@echo "GBS-Control Makefile targets:"
	@echo ""
	@echo "  make [all]              Build web UI + firmware (BOARD=$(BOARD))"
	@echo "  make webui              Regenerate generated/webui_html.h"
	@echo "  make firmware-only      Build firmware only (no npm)"
	@echo "  make upload             Build and upload to device"
	@echo "  make monitor            Serial monitor (BOARD=$(BOARD))"
	@echo "  make submodules         git submodule update --init --recursive"
	@echo "  make clean              PlatformIO clean"
	@echo "  make clean-all          clean + remove public/node_modules"
	@echo ""
	@echo "  make esp8266            Same as BOARD=d1_mini"
	@echo "  make esp32              Same as BOARD=esp32dev"
	@echo ""
	@echo "  BOARD=<env>             PlatformIO environment (default: d1_mini)"
	@echo "  Available: $(BOARDS)"
	@echo ""
	@echo "Web UI pipeline: public/ -> webui.html -> generated/webui_html.h"
	@echo "See public/README.md and docs/ARCHITECTURE.md"

all: firmware

submodules:
	git submodule update --init --recursive

webui:
	$(NPM) --prefix $(PUBLIC) install
	$(NPM) --prefix $(PUBLIC) run build

firmware-only:
	$(PIO) run -e $(BOARD)

firmware: webui firmware-only

upload:
	$(PIO) run -e $(BOARD) -t upload

monitor:
	$(PIO) device monitor -e $(BOARD)

clean:
	-$(PIO) run -t clean
	-$(RM) webui.html

clean-all: clean
	-$(RM) -r $(PUBLIC)/node_modules

esp8266:
	$(MAKE) firmware BOARD=d1_mini

esp32:
	$(MAKE) firmware BOARD=esp32dev

esp32-s3:
	$(MAKE) firmware BOARD=esp32-s3-devkitc-1

esp32-c3:
	$(MAKE) firmware BOARD=esp32-c3-devkitm-1

esp32-c6:
	$(MAKE) firmware BOARD=esp32-c6-devkitc-1
