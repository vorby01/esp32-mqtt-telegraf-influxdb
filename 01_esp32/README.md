2023 02 23
# esp32 to collect enviroment data (temperature,humidity,pressure) and output to mqtt broker(server)

I have three devices i have put together
esp32dvc (bme680, passive buzzer, tactile button)
esp32d_1 (bme280)
esp32d_2 (bme280)

### software
Arduino IDE 2.1.0
  - board libraries: esp32 by espressif systems v2.08

### arduino libraries
  - ArduinoMqttClient by arduino v0.1.7
  - Arduinojson by Benoit Blanchon v6.21.2
  - Adafruit BME680 Library by adafruit v2.0.2
  - Adafruit BME280 Library by adafruit v2.2.2
  - Adafruit Unified Sensor by adafruit v1.1.9
  
  
