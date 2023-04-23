2023 02 23
# esp32 to collect enviroment data (temperature,humidity,pressure) and output to mqtt broker(server)

I have three devices i have put together:
  - esp32dkc (bme680, passive buzzer, tactile button)
  - esp32d_1 (bme280)
  - esp32d_2 (bme280)

### software
Arduino IDE 2.1.0
  - board libraries: esp32 by espressif systems v2.0.8

### arduino libraries
  - ArduinoMqttClient by arduino v0.1.7
  - Arduinojson by Benoit Blanchon v6.21.2
  - Adafruit BME680 Library by adafruit v2.0.2
  - Adafruit BME280 Library by adafruit v2.2.2
  - Adafruit Unified Sensor by adafruit v1.1.9
  
### example code (C++)
  - device_name needs to be unique for each use
  - this script sets up topics compatible for home-assistant mqtt to automatically discover devices 
  <discovery_prefix>/<component>/[<node_id>/]<object_id>/config

``` cpp
//ESP32-Dev (DOIT ESP32 DEVKIT V1)
#include <WiFi.h> //(board manager: esp32 v2.0.8)
#include <ArduinoMqttClient.h> //(0.1.7) QoS: 0,1,2
#include <ArduinoJson.h> //(6.21.2) (json payload for device discovery)
#include <Wire.h>            //bme280
#include <Adafruit_Sensor.h> //bme280
#include <Adafruit_BME280.h> //bme280

#include "secrets.h"

const char* device_name = "esp32d_2";  //readonly*-------------------------------------------identifier

//WiFi settings
const char *wifi_ssid =  secret_wifi_ssid;
const char *wifi_password =  secret_wifi_password;
WiFiClient wifiClient; //creates a Wi-Fi client

//MQTT settings
MqttClient mqttClient(wifiClient);
const char broker[]    = secret_mqtt_ip;
int        port        = secret_mqtt_port;
//mqtt test/debug
bool mqtt_retain = true;
int mqtt_qos = 1;

//MQTT device
char mqtt_device_availability_topic[100];
//MQTT entity (bme280 temperature)
char mqtt_bme280t_sensorName[50];
char mqtt_bme280t_sensorUid[50];
char mqtt_bme280t_configure_topic[100];
char mqtt_bme280t_state_topic[100];
//MQTT entity (bme280 humidity)
char mqtt_bme280h_sensorName[50];
char mqtt_bme280h_sensorUid[50];
char mqtt_bme280h_configure_topic[100];
char mqtt_bme280h_state_topic[100];
//MQTT entity (bme280 pressure)
char mqtt_bme280p_sensorName[50];
char mqtt_bme280p_sensorUid[50];
char mqtt_bme280p_configure_topic[100];
char mqtt_bme280p_state_topic[100];

//BME280 settings
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; //bme280 I2C
float temperature, humidity;
uint32_t pressure;

//Global variables
unsigned long previousMillis = 0;
unsigned long interval = 60000; //sensor read

unsigned long pulsePreviousMillis = 0;
unsigned long pulseInterval = 60000; //mqtt connection check

//----------------------------------------------------------------------------------------------------setup
void setup() {
  Serial.begin(115200);
  Serial.print("[#] STARTING: "); Serial.println(device_name);

  //initiate wifi
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE); //must also call to set hostname
  WiFi.setHostname(device_name); //set hostname
  WiFi.onEvent(wifi_connected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED); //WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED
  WiFi.onEvent(wifi_received_address, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP); //WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP
  WiFi.onEvent(wifi_disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED); //WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED
  WiFi.begin(wifi_ssid, wifi_password);
  wait_for_wifi();

  //Setup bme280 sensor (0x76)
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor");
    while (1);
  }

  //MQTT update topics
  mqtt_device_availability_set_variables();
  mqtt_bme280t_set_variables();
  mqtt_bme280h_set_variables();
  mqtt_bme280p_set_variables();

  //Connect to mqtt broker (send will, send configure)
  mqtt_connect();

  //Read bme280
  bme280_read();
  bme280_serial_print();
  bme280_mqtt_send();

  Serial.println("[#] END SETUP");
}

//----------------------------------------------------------------------------------------------------loop
void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval){
    bme280_read();
    bme280_serial_print();
    bme280_mqtt_send();
    //mqtt_device_availability("online");
    previousMillis = currentMillis;
  }

  mqttClient.poll(); //poll for new MQTT messages and send keep alives
  if(currentMillis - pulsePreviousMillis >= pulseInterval){
    if(WiFi.status() == WL_CONNECTED && !mqttClient.connected()){
      Serial.println("[#] MQTT disconnected");
      mqtt_connect();
    }
    pulsePreviousMillis = currentMillis;
  }
}

//----------------------------------------------------------------------------------------------------wifi
void wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("[+] Connected to AP: "); Serial.print(WiFi.SSID()); Serial.print(", "); Serial.println(WiFi.RSSI());
}
void wifi_received_address(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("[+] Received IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(" | ");
  Serial.println(WiFi.getHostname());
}
void wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("[#] wifi disconnected: ");
  Serial.println(info.wifi_sta_disconnected.reason); //info.disconnected.reason
  Serial.println("[#] Retrying");
  WiFi.reconnect();
  wait_for_wifi();
}
void wait_for_wifi(){
  Serial.println("[#] wifi connecting");
  unsigned long timeout_count = millis();
  while (WiFi.status() != WL_CONNECTED) { // Wait for connection
    delay(500);
    Serial.print(".");
    if(millis() - timeout_count >= 15000){
      Serial.println("[#] Timed out, restarting");
      ESP.restart();
    }
  }
}

//----------------------------------------------------------------------------------------------------mqtt
void mqtt_connect(){
  mqttClient.setId(device_name);
  mqttClient.setUsernamePassword(secret_mqtt_username, secret_mqtt_password);
  //set mqtt LWT message, must be set before connecting [beginWill(const char *topic, bool retain, uint8_t qos)]
  mqttClient.beginWill(mqtt_device_availability_topic, mqtt_retain, mqtt_qos);
  mqttClient.print("offline");
  mqttClient.endWill();
  if (!mqttClient.connect(broker, port)) {
    Serial.print("[#] MQTT failed: ");
    Serial.println(mqttClient.connectError());
    //while (1);
    delay(15000);
    Serial.println("[#] Timed out, restarting");
    ESP.restart();
  }
  Serial.println("[+] MQTT connected");
  mqtt_send_device_configure();
  mqtt_device_availability("online"); //MQTT send device available
  mqttClient.onMessage(mqtt_on_message); //set the message receive callback
  mqttClient.subscribe(mqtt_device_availability_topic, mqtt_qos);
}
void mqtt_on_message(int messageSize) {
  Serial.print("[+] MQTT RECEIVED: ");
  if(mqttClient.messageTopic() == mqtt_device_availability_topic){ 
    if(mqttClient.available()){
      int mqtt_length = messageSize + 1;
      byte mqtt_payload[mqtt_length] = {0};
      mqttClient.readBytes(mqtt_payload, mqtt_length);
      if(strcmp("offline", (char *)mqtt_payload) == 0){
        Serial.print("device status: "); Serial.println((char *)mqtt_payload);
        mqtt_device_availability("online");
        mqtt_send_device_configure();
        bme280_mqtt_send();
      } else {
        Serial.print("device status: "); Serial.println((char *)mqtt_payload);
      }
    }
  } else {
    Serial.print(mqttClient.messageTopic()); Serial.print(": ");
    while (mqttClient.available()) {
      Serial.print((char)mqttClient.read());
    }
    Serial.println();
  }
}
void mqtt_send_device_configure(){
  mqtt_bme280t_configure();
  mqtt_bme280h_configure();
  mqtt_bme280p_configure();
}
void mqtt_device_availability(char* send_data){
  mqttClient.beginMessage(mqtt_device_availability_topic, true, mqtt_qos);
  mqttClient.print(send_data);
  mqttClient.endMessage();
  Serial.print("[M] Sent availability: "); Serial.print(mqtt_device_availability_topic); Serial.print(":"); Serial.println(send_data);
}
//home assistant topic structure: <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
void mqtt_device_availability_set_variables(){
  snprintf(mqtt_device_availability_topic, sizeof(mqtt_device_availability_topic), "homeassistant/%s/status", device_name);
}
void mqtt_bme280t_set_variables(){
  snprintf(mqtt_bme280t_sensorName, sizeof(mqtt_bme280t_sensorName), "%s_bme280t", device_name);
  snprintf(mqtt_bme280t_sensorUid, sizeof(mqtt_bme280t_sensorUid), "%s_uid", mqtt_bme280t_sensorName);
  snprintf(mqtt_bme280t_configure_topic, sizeof(mqtt_bme280t_configure_topic), "homeassistant/sensor/%s/temperature/config", device_name);
  snprintf(mqtt_bme280t_state_topic, sizeof(mqtt_bme280t_state_topic), "homeassistant/sensor/%s/temperature/state", device_name);
}
void mqtt_bme280h_set_variables(){
  snprintf(mqtt_bme280h_sensorName, sizeof(mqtt_bme280h_sensorName), "%s_bme280h", device_name);
  snprintf(mqtt_bme280h_sensorUid, sizeof(mqtt_bme280h_sensorUid), "%s_uid", mqtt_bme280h_sensorName);
  snprintf(mqtt_bme280h_configure_topic, sizeof(mqtt_bme280h_configure_topic), "homeassistant/sensor/%s/humidity/config", device_name);
  snprintf(mqtt_bme280h_state_topic, sizeof(mqtt_bme280h_state_topic), "homeassistant/sensor/%s/humidity/state", device_name);
}
void mqtt_bme280p_set_variables(){
  snprintf(mqtt_bme280p_sensorName, sizeof(mqtt_bme280p_sensorName), "%s_bme280p", device_name);
  snprintf(mqtt_bme280p_sensorUid, sizeof(mqtt_bme280p_sensorUid), "%s_uid", mqtt_bme280p_sensorName);
  snprintf(mqtt_bme280p_configure_topic, sizeof(mqtt_bme280p_configure_topic), "homeassistant/sensor/%s/pressure/config", device_name);
  snprintf(mqtt_bme280p_state_topic, sizeof(mqtt_bme280p_state_topic), "homeassistant/sensor/%s/pressure/state", device_name);
}
void mqtt_bme280t_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme280t_sensorName;
  doc["uniq_id"] = mqtt_bme280t_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme280t_state_topic; //mqtt_bme280_state_topic;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["val_tpl"] =  "{{value|float|default(0)|round(1)}}"; //"{{value_json.temperature|default(0)|round(1)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme280t_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos); //beginMessage(const char *topic, bool retain = false, uint8_t qos = 0, bool dup = false)
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
void mqtt_bme280h_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme280h_sensorName;
  doc["uniq_id"] = mqtt_bme280h_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme280h_state_topic;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["val_tpl"] = "{{value|float|default(0)|round(1)}}"; //"{{value_json.humidity|default(0)|round(1)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme280h_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
void mqtt_bme280p_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme280p_sensorName;
  doc["uniq_id"] = mqtt_bme280p_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme280p_state_topic;
  doc["unit_of_meas"] = "hPa";
  doc["dev_cla"] = "pressure";
  doc["val_tpl"] = "{{(value | float / 100)|default(0)|round(2)}}"; //"{{(value_json.pressure / 100)|default(0)|round(2)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme280p_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}

//----------------------------------------------------------------------------------------------------bme280
void bme280_read(){
Serial.println("[#] Reading bme280");
temperature = bme.readTemperature();
humidity = bme.readHumidity();
pressure = bme.readPressure(); //bme.readPressure()/ 100.0F; (hPa)
//bme.readAltitude(SEALEVELPRESSURE_HPA) (m)
}
void bme280_serial_print(){
    Serial.print("T:"); Serial.print(temperature); Serial.print("*C"); Serial.print(" | ");
    Serial.print("H:"); Serial.print(humidity); Serial.print("%"); Serial.print(" | ");
    Serial.print("P:"); Serial.print(pressure); Serial.println(" pa");
}
void bme280_mqtt_send(){
  mqttClient.beginMessage(mqtt_bme280t_state_topic, false, mqtt_qos);
  mqttClient.print(temperature);
  mqttClient.endMessage();
  mqttClient.beginMessage(mqtt_bme280h_state_topic, false, mqtt_qos);
  mqttClient.print(humidity);
  mqttClient.endMessage();
  mqttClient.beginMessage(mqtt_bme280p_state_topic, false, mqtt_qos);
  mqttClient.print(pressure);
  mqttClient.endMessage();
}
```
