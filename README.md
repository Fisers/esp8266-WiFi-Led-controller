# LED Controller over wifi (esp8266)
## Modes
- Solid color mode
- Multiple patterns (most of them require an individually addressable led strip) 
  - Utilizes patterns provided [here](https://github.com/jasoncoon/esp8266-fastled-webserver)
- Pulsing mode (gradually turns on and off LEDs)
## Features
- Adjustable brightness
- Easily adjustable LED count
- Auto connect to home WI-FI
- Failsafe mode in case connection failed (creates it's own WI-FI)
- Easily configurable WI-FI SSID and password using packets.
- Adjustable frame rate for different patterns
- Settings get saved on an onboard EEPROM
## Packets
There are multiple commands that are understood by the controller.
- "On" - Turns on LED strip
- "Off" - Turns off the LED strip
- "Rst" - Restarts the microcontroller
- "Led:(amount)" - Sets the number of LEDs in an strip
- "fps:(amount)" - Sets the framerate to be used for patterns
- "ssid:(wifi name)" - Sets the wifi SSID to be used for connecting
- "pass:(wifi pass)" - Sets the wifi password to be used for connecting
- "pat:(pattern number)" - Sets the active pattern (solid color mode gets disabled)
- "pal:(palette number)" - Sets the active color palette
- "puls:(on/off)" - Toggles if LEDs should pulsate
- "pSpeed:(speed)" - Sets the pulsating speed
- "bright:(0-255)" - Globally sets the brightness (also sets max brightness for pulsing)
- "minbright:(0-255)" - Sets min brightness pulse should reach before going back
- "(0-255):(0-255):(0-255)" - Enable and set solid color for LEDs format - RED:GREEN:BLUE

Sending any packet to the controller will respond with "acknowledged" (used for finding the current IP of the controller and delay before sending additional commands)

## Other
- Utilizes UDP packets to communicate
- Utilizes the FastLED library for communicating with the LED strip
- Utilizes ArduinoJSON library for saving the config
- I have created an [android app](https://github.com/Fisers/Android-LED-strip-controller-app) for sending commands to the controller.
