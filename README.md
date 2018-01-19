# ESP8266-ESP12F-OTA-WebSockets-Led
ESP12F controls Led String via GPIO using websockets, so dimming is immediate.

Watch this video:



[![WebSockets](http://img.youtube.com/vi/PNQiu9GexXI/0.jpg)](http://www.youtube.com/watch?v=PNQiu9GexXI "WebSockets")

ESP uses  standalone wifi but could connect to your wifi for full IOT joy.
ESP12F serves webpage featuring a slider.
Webpage javascript sends slider value back to ESP12F via websocket.
ESP12F receives value, sets PWM of GPIO pin so Leds dim.

Built using Arduino IDE. 
Extends for Katz's code from http://www.esp8266.com/viewtopic.php?f=8&t=11887 
