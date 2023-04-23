//ESP32-DevKitC (DOIT ESP32 DEVKIT V1)
#include <WiFi.h> //(board manager: esp32 v2.0.7)
#include <ArduinoMqttClient.h> //(0.1.7) QoS: 0,1,2
#include <ArduinoJson.h> //(6.21.2) (json payload for device discovery)
#include <Wire.h>            //bme680
#include <Adafruit_Sensor.h> //bme680
#include "Adafruit_BME680.h" //bme680

#include "secrets.h"

const char* device_name = "esp32dkc";  //readonly*-------------------------------------------identifier

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
bool mqtt_received_availability = true;
//MQTT button
char mqtt_button_name[50];
char mqtt_button_uid[50];
char mqtt_button_configure_topic[100];
char mqtt_button_state_topic[100];
char mqtt_button_command_topic[100];
bool MQTT_BUTTON_STATE = false;
bool PREVIOUS_MQTT_BUTTON_STATE = false;
bool mqtt_button_off_delay = false;
unsigned long mqtt_button_off_delay_millis = 0;
//MQTT binary sensor (buzzer)
char mqtt_buzzer_binarySensorName[50];
char mqtt_buzzer_binarySensorUid[50];
char mqtt_buzzer_configure_topic[100];
char mqtt_buzzer_state_topic[100];
//MQTT bme680
char mqtt_bme680_state_topic[100];
//MQTT entity (bme680 temperature)
char mqtt_bme680t_sensorName[50];
char mqtt_bme680t_sensorUid[50];
char mqtt_bme680t_configure_topic[100];
char mqtt_bme680t_state_topic[100];
//MQTT entity (bme680 humidity)
char mqtt_bme680h_sensorName[50];
char mqtt_bme680h_sensorUid[50];
char mqtt_bme680h_configure_topic[100];
char mqtt_bme680h_state_topic[100];
//MQTT entity (bme680 pressure)
char mqtt_bme680p_sensorName[50];
char mqtt_bme680p_sensorUid[50];
char mqtt_bme680p_configure_topic[100];
char mqtt_bme680p_state_topic[100];
//MQTT entity (bme680 gas_resistance)
char mqtt_bme680g_sensorName[50];
char mqtt_bme680g_sensorUid[50];
char mqtt_bme680g_configure_topic[100];
char mqtt_bme680g_state_topic[100];

//Button settings (mqtt switch)
const int BUTTON_PIN = 18;
bool BUTTON_STATE = false;
bool PREVIOUS_BUTTON_STATE = false;

//Buzzer settings
const int BUZZER_PIN = 19;
const int BUZZER_CHANNEL = 1; //esp32 pwm channel (0-15)
bool BUZZER_STATE = false; //buzzer playing
//FreeRTOS settings(concurrent tasks) (Free Real Time Operating System) core0:wifi | core1: setup/loop
TaskHandle_t Task1;

//BME680 settings
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME680 bme; //bme680 I2C
float temperature, humidity;
uint32_t pressure, gas;

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
  //initiate button
  pinMode(BUTTON_PIN,INPUT);
  BUTTON_STATE = digitalRead(BUTTON_PIN);
  PREVIOUS_BUTTON_STATE = BUTTON_STATE;
  //initiate buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  ledcSetup(BUZZER_CHANNEL, 2000, 8); //(pwm_channel, frequency, resolution(8bit= 0-255))
  //Setup bme680 sensor (0x77)
  if (!bme.begin(0x77)) {
    Serial.println("Could not find a valid BME680 sensor");
    while (1);
  }
  //Setup oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  //Read bme680
  bme680_read();
  //MQTT update topics
  mqtt_device_availability_set_variables();
  mqtt_button_set_variables();
  mqtt_buzzer_set_variables();
  //mqtt_bme680_state_set_variables();
  mqtt_bme680t_set_variables();
  mqtt_bme680h_set_variables();
  mqtt_bme680p_set_variables();
  mqtt_bme680g_set_variables();
  //Connect to mqtt broker (send will, send configure)
  mqtt_connect();
  //play tone
  buzzer_start_tone();
  //bme680 output
  bme680_serial_print();
  Serial.println("[#] END SETUP");
}//END_OF_SETUP

//----------------------------------------------------------------------------------------------------loop
void loop() {
  unsigned long currentMillis = millis();
  //bme680
  if(currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    bme680_read();
    bme680_serial_print();
    mqtt_bme680_send();
  }
  //button
  BUTTON_STATE = digitalRead(BUTTON_PIN);
  if(BUTTON_STATE != PREVIOUS_BUTTON_STATE){
    PREVIOUS_BUTTON_STATE = BUTTON_STATE;
    if(BUTTON_STATE == true){
      Serial.print("[B] button pressed:"); Serial.println(BUTTON_STATE);
      mqtt_button_state_set("ON");
      buzzer_play();
    } else {
      Serial.print("[B] button released:"); Serial.println(BUTTON_STATE);
      mqtt_button_state_set("OFF");
    }
  }
  //mqtt button
  if(MQTT_BUTTON_STATE != PREVIOUS_MQTT_BUTTON_STATE){
    PREVIOUS_MQTT_BUTTON_STATE = MQTT_BUTTON_STATE;
    if(MQTT_BUTTON_STATE == true){
      mqtt_button_state_set("ON");
      buzzer_play();
      mqtt_button_off_delay = true; //mqtt needs return delay or crash 
      mqtt_button_off_delay_millis = currentMillis;
    } else {
      mqtt_button_state_set("OFF");
    }
  }
  if(mqtt_button_off_delay == true){
    if(currentMillis - mqtt_button_off_delay_millis >= 100){
      //mqtt_button_off_delay_millis = currentMillis;
      MQTT_BUTTON_STATE = false;
      mqtt_button_off_delay = false;
    }
  }
  //device availabilty mqtt sync
  if(mqtt_received_availability == false){
    mqtt_device_availability("online");
    mqtt_received_availability = true;
  }
  //mqtt check connection
  mqttClient.poll(); //mqtt keep alive
  if(currentMillis - pulsePreviousMillis >= pulseInterval){
    pulsePreviousMillis = currentMillis;
    if(WiFi.status() == WL_CONNECTED && !mqttClient.connected()){
      Serial.println("[#] MQTT disconnected");
      mqtt_connect();
    }
    //Serial.println("[#] connection check");
  }
}//END_OF_LOOP

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
//home assistant topic structure: <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
void mqtt_device_availability_set_variables(){
  snprintf(mqtt_device_availability_topic, sizeof(mqtt_device_availability_topic), "homeassistant/%s/status", device_name);
}
void mqtt_connect(){
  mqttClient.setId(device_name);
  mqttClient.setUsernamePassword(secret_mqtt_username, secret_mqtt_password);
  //set mqtt LWT message, must be set before connecting [beginWill(const char *topic, bool retain, uint8_t qos)]
  mqttClient.beginWill(mqtt_device_availability_topic, mqtt_retain, mqtt_qos);
  mqttClient.print("offline");
  mqttClient.endWill();
  if (!mqttClient.connect(broker, port)) {
    Serial.print("[M] MQTT failed: ");
    Serial.println(mqttClient.connectError());
    //while (1);
    delay(15000);
    Serial.println("[#] Timed out, restarting");
    ESP.restart();
  }
  Serial.println("[M] MQTT connected");
  mqtt_send_device_configurations(); //send device configurations
  mqtt_device_availability("online");
  mqtt_send_device_states();
  mqttClient.onMessage(mqtt_on_message); //set the message receive callback
  //mqttClient.subscribe("homeassistant/status", mqtt_qos);
  mqttClient.subscribe(mqtt_device_availability_topic, mqtt_qos);
  mqttClient.subscribe(mqtt_button_command_topic, mqtt_qos);
}
void mqtt_send_device_configurations(){
  mqtt_buzzer_configure();
  mqtt_button_configure();
  mqtt_bme680t_configure();
  mqtt_bme680h_configure();
  mqtt_bme680p_configure();
  mqtt_bme680g_configure();
}
void mqtt_device_availability(char* send_data){
  mqttClient.beginMessage(mqtt_device_availability_topic, mqtt_retain, mqtt_qos);
  mqttClient.print(send_data);
  mqttClient.endMessage();
  Serial.print("[M] Sent availability: "); Serial.print(mqtt_device_availability_topic); Serial.print(":"); Serial.println(send_data);
}
void mqtt_send_device_states(){
  mqtt_buzzer_state_sync();
  mqtt_button_state_sync();
  mqtt_bme680_send();
}
void mqtt_on_message(int messageSize) {
  Serial.print("[+] MQTT RECEIVED: ");
  if(mqttClient.messageTopic() == mqtt_device_availability_topic){ 
    if(mqttClient.available()){
      int mqtt_length = messageSize + 1;
      byte mqtt_payload[mqtt_length] = {0};
      mqttClient.readBytes(mqtt_payload, mqtt_length);
      if(strcmp("online", (char *)mqtt_payload) == 0){
        Serial.print("device status: "); Serial.println((char *)mqtt_payload);
        mqtt_received_availability = true;
      } else {
        Serial.print("device status: "); Serial.println((char *)mqtt_payload);
        mqtt_received_availability = false;
      }
    }
  } else if(mqttClient.messageTopic() == mqtt_button_command_topic){
    if(mqttClient.available()){
      int mqtt_length = messageSize + 1;
      byte mqtt_payload[mqtt_length] = {0};
      mqttClient.readBytes(mqtt_payload, mqtt_length);
      if(strcmp("ON", (char *)mqtt_payload) == 0){
        MQTT_BUTTON_STATE = true;
        Serial.print("mqtt switch state: "); Serial.println((char *)mqtt_payload);
      } else {
        MQTT_BUTTON_STATE = false;
        Serial.print("mqtt switch state: "); Serial.println((char *)mqtt_payload);
      }
    }
  } else {
    Serial.print(mqttClient.messageTopic()); Serial.print(":!: ");
    while (mqttClient.available()) {
      Serial.print((char)mqttClient.read());
    } 
    Serial.println();
  } 
}
//button functions
void mqtt_button_set_variables(){
  snprintf(mqtt_button_name, sizeof(mqtt_button_name), "%s_button", device_name);
  snprintf(mqtt_button_uid, sizeof(mqtt_button_uid), "%s_uid", mqtt_button_name);
  snprintf(mqtt_button_configure_topic, sizeof(mqtt_button_configure_topic), "homeassistant/switch/%s/button/config", device_name);
  snprintf(mqtt_button_state_topic, sizeof(mqtt_button_state_topic), "homeassistant/switch/%s/button/state", device_name);
  snprintf(mqtt_button_command_topic, sizeof(mqtt_button_command_topic), "homeassistant/switch/%s/button/set", device_name);
}
void mqtt_button_state_set(char * state){
  mqttClient.beginMessage(mqtt_button_state_topic, false, mqtt_qos);
  mqttClient.print(state);
  mqttClient.endMessage();
  Serial.print("[M] mqtt_button_state_sent:"); Serial.println(state);
}
void mqtt_button_state_sync(){
  switch (BUTTON_STATE){
    case true:
      mqtt_button_state_set("ON"); //sync mqtt button state
      break;
    case false:
      mqtt_button_state_set("OFF");
      break;
  }
}
void mqtt_button_command_set(char * state){
  mqttClient.beginMessage(mqtt_button_command_topic, false, mqtt_qos);
  mqttClient.print(state);
  mqttClient.endMessage();
  Serial.print("[M] mqtt_button_command_sent:"); Serial.println(state);
}
void mqtt_button_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_button_name;
  doc["uniq_id"] = mqtt_button_uid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_button_state_topic;
  doc["cmd_t"] = mqtt_button_command_topic;
  doc["pl_on"] = "ON";
  doc["pl_off"] = "OFF";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_button_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
//buzzer functions
void mqtt_buzzer_set_variables(){
  snprintf(mqtt_buzzer_binarySensorName, sizeof(mqtt_buzzer_binarySensorName), "%s_buzzer", device_name);
  snprintf(mqtt_buzzer_binarySensorUid, sizeof(mqtt_buzzer_binarySensorUid), "%s_uid", mqtt_buzzer_binarySensorName);
  snprintf(mqtt_buzzer_configure_topic, sizeof(mqtt_buzzer_configure_topic), "homeassistant/binary_sensor/%s/buzzer/config", device_name);
  snprintf(mqtt_buzzer_state_topic, sizeof(mqtt_buzzer_state_topic), "homeassistant/binary_sensor/%s/buzzer/state", device_name);
}
void mqtt_buzzer_state_set(char * state){
  mqttClient.beginMessage(mqtt_buzzer_state_topic, false, mqtt_qos);
  mqttClient.print(state);
  mqttClient.endMessage();
  Serial.print("[M] mqtt_buzzer_state_sent:"); Serial.println(state);
}
void mqtt_buzzer_state_sync(){
  switch(BUZZER_STATE){
    case true:
      mqtt_buzzer_state_set("ON");
      break;
    case false:
      mqtt_buzzer_state_set("OFF");
      break;
  }
}
void mqtt_buzzer_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_buzzer_binarySensorName;
  doc["uniq_id"] = mqtt_buzzer_binarySensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_buzzer_state_topic;
  doc["pl_on"] = "ON";
  doc["pl_off"] = "OFF";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_buzzer_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
//bme680 functions
//void mqtt_bme680_state_set_variables(){
//  snprintf(mqtt_bme680_state_topic, sizeof(mqtt_bme680_state_topic), "homeassistant/sensor/%s/state", device_name);
//}
void mqtt_bme680t_set_variables(){
  snprintf(mqtt_bme680t_sensorName, sizeof(mqtt_bme680t_sensorName), "%s_bme680t", device_name);
  snprintf(mqtt_bme680t_sensorUid, sizeof(mqtt_bme680t_sensorUid), "%s_uid", mqtt_bme680t_sensorName);
  snprintf(mqtt_bme680t_configure_topic, sizeof(mqtt_bme680t_configure_topic), "homeassistant/sensor/%s/temperature/config", device_name);
  snprintf(mqtt_bme680t_state_topic, sizeof(mqtt_bme680t_state_topic), "homeassistant/sensor/%s/temperature/state", device_name);
}
void mqtt_bme680h_set_variables(){
  snprintf(mqtt_bme680h_sensorName, sizeof(mqtt_bme680h_sensorName), "%s_bme680h", device_name);
  snprintf(mqtt_bme680h_sensorUid, sizeof(mqtt_bme680h_sensorUid), "%s_uid", mqtt_bme680h_sensorName);
  snprintf(mqtt_bme680h_configure_topic, sizeof(mqtt_bme680h_configure_topic), "homeassistant/sensor/%s/humidity/config", device_name);
  snprintf(mqtt_bme680h_state_topic, sizeof(mqtt_bme680h_state_topic), "homeassistant/sensor/%s/humidity/state", device_name);
}
void mqtt_bme680p_set_variables(){
  snprintf(mqtt_bme680p_sensorName, sizeof(mqtt_bme680p_sensorName), "%s_bme680p", device_name);
  snprintf(mqtt_bme680p_sensorUid, sizeof(mqtt_bme680p_sensorUid), "%s_uid", mqtt_bme680p_sensorName);
  snprintf(mqtt_bme680p_configure_topic, sizeof(mqtt_bme680p_configure_topic), "homeassistant/sensor/%s/pressure/config", device_name);
  snprintf(mqtt_bme680p_state_topic, sizeof(mqtt_bme680p_state_topic), "homeassistant/sensor/%s/pressure/state", device_name);
}
void mqtt_bme680g_set_variables(){
  snprintf(mqtt_bme680g_sensorName, sizeof(mqtt_bme680g_sensorName), "%s_bme680g", device_name);
  snprintf(mqtt_bme680g_sensorUid, sizeof(mqtt_bme680g_sensorUid), "%s_uid", mqtt_bme680g_sensorName);
  snprintf(mqtt_bme680g_configure_topic, sizeof(mqtt_bme680g_configure_topic), "homeassistant/sensor/%s/gas/config", device_name);
  snprintf(mqtt_bme680g_state_topic, sizeof(mqtt_bme680g_state_topic), "homeassistant/sensor/%s/gas/state", device_name);
}
void mqtt_bme680_send(){
  mqttClient.beginMessage(mqtt_bme680t_state_topic, false, mqtt_qos);
  mqttClient.print(temperature);
  mqttClient.endMessage();
  mqttClient.beginMessage(mqtt_bme680h_state_topic, false, mqtt_qos);
  mqttClient.print(humidity);
  mqttClient.endMessage();
  mqttClient.beginMessage(mqtt_bme680p_state_topic, false, mqtt_qos);
  mqttClient.print(pressure);
  mqttClient.endMessage();
  mqttClient.beginMessage(mqtt_bme680g_state_topic, false, mqtt_qos);
  mqttClient.print(gas);
  mqttClient.endMessage();
}
void mqtt_bme680t_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme680t_sensorName;
  doc["uniq_id"] = mqtt_bme680t_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme680t_state_topic;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["val_tpl"] = "{{value|float|default(0)|round(1)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme680t_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos); //beginMessage(const char *topic, bool retain = false, uint8_t qos = 0, bool dup = false)
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
void mqtt_bme680h_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme680h_sensorName;
  doc["uniq_id"] = mqtt_bme680h_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme680h_state_topic;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["val_tpl"] = "{{value|float|default(0)|round(1)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme680h_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
void mqtt_bme680p_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme680p_sensorName;
  doc["uniq_id"] = mqtt_bme680p_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme680p_state_topic;
  doc["unit_of_meas"] = "hPa";
  doc["dev_cla"] = "pressure";
  doc["val_tpl"] =  "{{ ( value | float / 100) |round(2)}}"; //"{{(value_json.pressure / 100)|default(0)|round(2)}}";  "{{value|int|default(0)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme680p_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}
void mqtt_bme680g_configure(){
  DynamicJsonDocument doc(512);
  doc["name"] = mqtt_bme680g_sensorName;
  doc["uniq_id"] = mqtt_bme680g_sensorUid;
  doc["avty_t"] = mqtt_device_availability_topic;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["stat_t"] = mqtt_bme680g_state_topic;
  doc["unit_of_meas"] = "ohms";
  //doc["dev_cla"] = "";
  doc["val_tpl"] = "{{value|int|default(0)}}";
  JsonObject device = doc.createNestedObject("device");
  device["ids"][0] = device_name;
  device["mf"] = device_name;
  device["mdl"] = device_name;
  device["name"] = device_name;
  mqttClient.beginMessage(mqtt_bme680g_configure_topic, (unsigned long)measureJson(doc), mqtt_retain, mqtt_qos);
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
}

//----------------------------------------------------------------------------------------------------buzzer
void buzzer_start_tone(){
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 125);
  ledcWriteTone(BUZZER_CHANNEL, 800); delay(163);
  ledcWrite(BUZZER_CHANNEL, 0); delay(163);
  ledcWrite(BUZZER_CHANNEL, 125);
  ledcWriteTone(BUZZER_CHANNEL, 600); delay(163);
  ledcWrite(BUZZER_CHANNEL, 0);
  ledcDetachPin(BUZZER_PIN);
}
void buzzer_play(){
    if(BUZZER_STATE == false){
      //Serial.println("BUZZER_PLAY");
      BUZZER_STATE = true;
      xTaskCreatePinnedToCore(buzzer_play_doom, "Task1", 10000, NULL, 1, &Task1, 1); //setup concurrent tasks
      //xTaskCreatePinnedToCore(buzzer_freeRTOS_test, "Task1", 10000, NULL, 1, &Task1, 1); //setup concurrent tasks
  } else {
    Serial.println("[Z] Buzzer Busy");
  }
}

//----------------------------------------------------------------------------------------------------buzzer/freeRTOS
//for(;;){}; = infinate loop
void buzzer_play_doom(void * parameter){
  //BUZZER_STATE = true;
  mqtt_buzzer_state_set("ON");
  Serial.print("[2]# buzzer starting concurrently,core:"); Serial.println(xPortGetCoreID());
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 125);
  ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,165); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,147); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,82); delay(163);
  ledcWriteTone(BUZZER_CHANNEL,131); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,117); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,123); delay(163); ledcWriteTone(BUZZER_CHANNEL,131); delay(163);
  ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,165); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,147); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,82); delay(163);
  ledcWriteTone(BUZZER_CHANNEL,131); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163); ledcWriteTone(BUZZER_CHANNEL,82); delay(163);  ledcWriteTone(BUZZER_CHANNEL,117); delay(650);
  ledcWrite(BUZZER_CHANNEL, 0);
  ledcDetachPin(BUZZER_PIN);
  mqtt_buzzer_state_set("OFF");
  Serial.println("[2]# buzzer finished");
  BUZZER_STATE = false;
  vTaskDelete( NULL );
}
void buzzer_freeRTOS_test(void * parameter){
  BUZZER_STATE = true;
  //mqtt_buzzer_state_set("ON");
  Serial.println("[2] START");
  delay(1000);
  Serial.println("[2] END");
  //mqtt_buzzer_state_set("OFF");
  BUZZER_STATE = false;
  vTaskDelete( NULL );
}
//----------------------------------------------------------------------------------------------------bme680
void bme680_read(){
Serial.println("[#] Reading bme680");
if(!bme.performReading()){
  Serial.println("[#] bme680 error");
  return;
}
temperature = bme.temperature;
humidity = bme.humidity;
pressure = bme.pressure; //bme.pressure / 100.0 (hPa)
gas = bme.gas_resistance; //bme.gas_resistance / 1000.0 (KOhms)
//bme.readAltitude(SEALEVELPRESSURE_HPA) (m)
}
void bme680_serial_print(){
    Serial.print("T:"); Serial.print(temperature); Serial.print("*C"); Serial.print(" | ");
    Serial.print("H:"); Serial.print(humidity); Serial.print("%"); Serial.print(" | ");
    Serial.print("P:"); Serial.print(pressure); Serial.print(" pa"); Serial.print(" | ");
    Serial.print("G:"); Serial.print(gas); Serial.println(" ohms");
}
