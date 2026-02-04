.PHONY: help build upload monitor clean erase-flash lint format test

help:
	@echo "ESP8266 Spotify Display"
	@echo ""
	@echo "build              - Compile sketch"
	@echo "upload             - Upload to device"
	@echo "monitor            - Serial monitor (115200)"
	@echo "lint               - Check code with strict warnings"
	@echo "format             - Check code formatting"
	@echo "format-fix         - Auto-fix code formatting"
	@echo "clean              - Clean build files"
	@echo "erase-flash        - Erase ESP8266"
	@echo "test               - Show testing checklist"

build:
	cd ESP_DispSpotTrack && pio run -e esp8266

upload:
	cd ESP_DispSpotTrack && pio run -e esp8266 -t upload

monitor:
	cd ESP_DispSpotTrack && pio device monitor -b 115200

lint:
	cd ESP_DispSpotTrack && pio run -e lint

format:
	@command -v clang-format >/dev/null 2>&1 || { echo "clang-format not found. Install: brew install clang-format"; exit 1; }
	clang-format --dry-run -Werror ESP_DispSpotTrack/src/main.cpp || echo "⚠️  Formatting issues found. Run 'make format-fix'"

format-fix:
	@command -v clang-format >/dev/null 2>&1 || { echo "clang-format not found. Install: brew install clang-format"; exit 1; }
	clang-format -i ESP_DispSpotTrack/src/main.cpp
	@echo "✅ Code formatted"

test:
	@cat TESTING.md

clean:
	cd ESP_DispSpotTrack && pio run -t clean 2>/dev/null

erase-flash:
	esptool.py --port $$(ls /dev/cu.usbserial-* 2>/dev/null | head -1) erase_flash

.DEFAULT_GOAL := help
