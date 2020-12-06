#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAddress.h>
#include <NimBLEBeacon.h>

#define MQTT_server ""         // FQDN of your MQTT broker
#define MQTT_tcpport 8883      // tcp port to use, defaults to 8883 for tls secured connections
#define MQTT_username ""       // user name to use to connect to MQTT broker
#define MQTT_password ""       // password to use to connect to MQTT broker
#define MQTT_clientid ""       // clientid to use when connecting with MQTT broker
#define MQTT_willtopic ""      // topic to publish testament/last will 
#define MQTT_sensortopic ""    // topic to publish found beacons

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int BLEscanTime = 5;

const char* ssid = "";         // name of your wireless network
const char* ssidkey = "";      // password to connect to your wireless network

const char root_ca[] PROGMEM = R"=====(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)=====" ;                      /* add your x509 encoded root certificate here. This certificate will be used to verify your MQTT broker's SSL certificate.
                                  In case there's an intermediate certificate in between, add both, root and intermediate certificate here. */


BLEScan* pBLEScan;
WiFiClientSecure wclient;
PubSubClient mqtt(MQTT_server, MQTT_tcpport, wclient);


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  #define MSG_BUFFER_SIZE (150)
  char msg[MSG_BUFFER_SIZE];

  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    if (advertisedDevice->haveServiceUUID())
    {
      delay(1);
    }
    else
    {
      if (advertisedDevice->haveManufacturerData() == true)
      {
        std::string strManufacturerData = advertisedDevice->getManufacturerData();
        std::string deviceAddress = advertisedDevice->getAddress();
        uint8_t cManufacturerData[100];
        strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

        if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00)
        {
          BLEBeacon iBeacon = BLEBeacon();
          iBeacon.setData(strManufacturerData);
          Serial.printf("iBeacon found - ID: %04X Major: %04X Minor: %04X UUID: %s MAC: %s Power: %d\n", iBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(iBeacon.getMajor()), ENDIAN_CHANGE_U16(iBeacon.getMinor()), iBeacon.getProximityUUID().toString().c_str(), deviceAddress.c_str(), iBeacon.getSignalPower());
          snprintf (msg, MSG_BUFFER_SIZE, "{\"IBEACON\":{\"UID\":\"%s\",\"MAJOR\":\"%04X\",\"MINOR\":\"%04X\",\"MAC\":\"%s\",\"RSSI\":%d}}", iBeacon.getProximityUUID().toString().c_str(), ENDIAN_CHANGE_U16(iBeacon.getMajor()), ENDIAN_CHANGE_U16(iBeacon.getMinor()), deviceAddress.c_str(), iBeacon.getSignalPower());
          mqtt.publish(MQTT_sensortopic, msg);
        }
      }
      return;
    }
  }
};


void setup()
{
  int i;
  int timeout = 0;
  Serial.begin(115200);
  Serial.println("Started up");
  Serial.println("Waiting for Wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, ssidkey);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    timeout++;
    if (timeout > 60)
    {
      Serial.println("\r\nWifi issue. Rebooting.");
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  wclient.setCACert(root_ca);
 
  if (mqtt.connect(MQTT_clientid, MQTT_username, MQTT_password, MQTT_willtopic, 0, true, "OFF"))
  {
    Serial.println("\r\nConnection to broker established");
    mqtt.setBufferSize(2048);
    mqtt.publish(MQTT_willtopic, "ON", true);
  }
 
  Serial.println("Starting BLE scan");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}


void loop()
{
  BLEScanResults foundDevices = pBLEScan->start(BLEscanTime, false);
  delay(100);
  pBLEScan->clearResults();
  delay(100);

  if (WiFi.status() != WL_CONNECTED) ESP.restart();
  
  if (mqtt.connected())
  {
    mqtt.loop();
  }
  else
  {
    Serial.println("Connection to broker lost");
    mqtt.connect(MQTT_clientid, MQTT_username, MQTT_password, MQTT_willtopic, 0, true, "Offline");
    int timeout = 0;
  
    while (!mqtt.connected())
    {
     delay(500);
     Serial.print(".");
     timeout++;
    
     if  (timeout > 60)
     {
       Serial.println("\r\nConnection to broker lost. Rebooting.");
       ESP.restart();
     }
    }

   mqtt.publish(MQTT_willtopic, "Online", true);
  }
}
