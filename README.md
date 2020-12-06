  
Fork of the esp32-bt-mqtt-scanner published in ct magazine 26/2020.
  
  
Differences:
  
  - Implementing WifiSecureClient library instead of WifiClient in order to allow TLS encrypted MQTT connections.
  
  - Implementing NimBLE library instead of ESP32 BLE in order to decrease program memory usage.
  
  - Searching for iBeacons only, ignore all other types. Can easily be extended with other beacon types.
  
  - MQTT payload as proper JSON string.
  
  - Publish retained message upon connecting to MQTT broker to indicate device is online
  
  - Implementing last will/testament
  
  
Usage:
  
Copy code to Arduino IDE and add required parameters. All parameters with a comment are required. Make sure PubSubClient by Nick O'Leary (tested with 2.8.0) and NimBLE-Arduino by h2zero (tested with 1.0.2) are in your library. Upload sketch to your ESP32:

  - Board according to your ESP32 device
  - Upload Speed 921600
  - CPU frequency 240MHz (WiFi/BT)
  - Flash Frequency 80MHz
  - Flash Mode QIO
  - Flash Size according to your ESP32 module, usually 4MB (32Mb)
  - Partition Scheme No OTA (2MB APP/2MB FATFS)
  - PSRAM Disabled

Go to Serial Monitor to verify Wifi and MQTT broker connection. Detected iBeacons will be listed here as well as published via MQTT.
