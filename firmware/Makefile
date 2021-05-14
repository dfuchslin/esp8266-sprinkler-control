BOARD = esp8266:esp8266:nodemcuv2
BAUD = 115200
FQBN = $(BOARD):led=2,baud=$(BAUD)
PORT = /dev/cu.usbserial-0001
TTY = /dev/cu.usbserial-0001
CC = arduino-cli
HOSTNAME = sprinkler-control.local

compile: 
	$(CC) compile --verbose --fqbn $(FQBN) --export-binaries

flash: 
	$(CC) upload -p $(PORT) --fqbn $(FQBN) `pwd`

upload:
	curl http://$(HOSTNAME)/update -F "name=firmware" -F "image=@build/esp8266.esp8266.nodemcuv2/firmware.ino.bin"

serial:
	@echo "Type ~. to exit"
	cu -l $(TTY) -s $(BAUD)

setup:
	$(CC) config init --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json,https://dl.espressif.com/dl/package_esp32_index.json
	$(CC) core update-index
	$(CC) core install esp8266:esp8266
	$(CC) lib install WiFiManager

clean:
	rm -rf build
