# IC-905-control
ESP32 WIFI or BDE control of Rover setup.
using ESP32-WROOM-32 (WEMOS D1 Mini ESP32)

Using 8 channel 5V relay module to control RF swiches betwen the IC-905 and antennas.
using a second ESP32 to control power to motor and poseble it's speed.
Using Adafruit DRV8871 Motor Breakout (P3190) bord to control antenna rotor.
add 0.96 Inch OLED I2C IIC Display Module 12864 128x32 Pixel SSD1306 to show SSID and IP as a test if using wifi.
add 2.42 Inch OLED SPI display Modual 128x64 Pixel SSD1309 to show status of server via BLE in the BLEclient/server.
add NEXTION NX4827P043 for display inside vehical and it comunicats to the BLE/Wifi server.
