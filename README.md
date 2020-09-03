# esp32cam

This is based on the demo code for the ESP32 camera module, with some tweaks:

* Prometheus exporter for monitoring CPU temperature, wifi signal strength, frames per second, and power consumption
* syslog support for the same metrics (hardcoded IP for now)
* Watchdog that restarts the device if frame count stalls or WiFi disconnects for more than 5 minutes
* Set the default frame size to VGA for my personal needs
* maximum FPS cap
* hostname by mac address (hardcoded for now)
* probably some other stuff I'm forgetting


Known issues:
* The power consumption stuff doesn't work.  It's meant to use an external coulomb counter but for the moment I'm having a lot of problems using GPIOs on the AI-Thinker module

