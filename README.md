# esp32cam

This is based on the demo code for the ESP32 camera module, with some tweaks:

* Prometheus exporter for monitoring CPU temperature, wifi signal strength, frames per second, and power consumption
* Watchdog that restarts the device if frame count stalls or WiFi disconnects for more than 5 minutes
* Set the default frame size to VGA for my personal needs
* probably some other stuff I'm forgetting right now


Known issues:
* The power consumption stuff doesn't work.  It's meant to use an external coulomb counter but for the moment I'm having a lot of problems using GPIOs on the AI-Thinker module

