.PHONY: help build upload monitor clean erase-flash

help:
	@echo "ESP8266 Spotify Display"
	@echo ""
	@echo "build              - Compile sketch"
	@echo "upload             - Upload to device"
	@echo "monitor            - Serial monitor (115200)"
	@echo "clean              - Clean build files"
	@echo "erase-flash        - Erase ESP8266"

build:
	cd ESP_DispSpotTrack && pio run -e esp8266

upload:
	cd ESP_DispSpotTrack && pio run -e esp8266 -t upload

monitor:
	cd ESP_DispSpotTrack && pio device monitor -b 115200

clean:
	cd ESP_DispSpotTrack && pio run -t clean 2>/dev/null

erase-flash:
	esptool.py --port $$(ls /dev/cu.usbserial-* 2>/dev/null | head -1) erase_flash

.DEFAULT_GOAL := help
