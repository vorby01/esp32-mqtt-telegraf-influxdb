# Mosquitto MQTT
mqtt broker (publish/subscribe)

### Install mosquitto mqtt broker
```
sudo apt install mosquitto
```

### Install mosquitto mqtt client (testing/debug)
```
sudo apt install mosquitto-clients
```

### Setup password file for mosquitto mqtt
  - create password file: </br>
  replace mqtt_username with username </br>
  replace mqtt_password with password </br>
  ```echo "mqtt_username:mqtt_password" > pwfile```
  
  - encrypt password file: </br>
  ```mosquitto_passwd -U pwfile```
  
  - check password file encrypt: </br>
  ```cat pwfile```
  
  - move password file to mosquitto directory: </br>
  ```sudo mv pwfile /etc/mosquitto/```
  
### Configure mosquitto mqtt broker
/etc/mosquitto/mosquitto.conf
```
pid_file /run/mosquitto/mosquitto.pid
persistence true
persistence_location /var/lib/mosquitto/
log_dest file /var/log/mosquitto/mosquitto.log

allow_anonymous false
password_file /etc/mosquitto/pwfile

#mosquitto default only available on localhost
listener 1883 <ip_address>

#mosquitto bind interface requires running with root privileges
#bind_interface eth1

include_dir /etc/mosquitto/conf.d
```

### Start mosquitto mqtt broker
```
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

### Additional resources
  - found issue with mosquitto failing to start on boot, added wait for network online. </br>
  /usr/lib/systemd/system/mosquitto.service
  ```
  [Unit]
  Description=Mosquitto MQTT Broker
  Documentation=man:mosquitto.conf(5) man:mosquitto(8)
  After=network.target network-online.target
  Wants=network.target
  ```
  Update systemd
  ```
  sudo systemctl daemon-reload
  ```
  
#### Home assistant mqtt discovery
The discovery topic needs to follow a specific format: </br>
<discovery_prefix>/\<component>/[<node_id>/]<object_id>/config
```
<discovery_prefix> - mqtt discovery topic (homeassistant) </br>
<component> - a supported mqtt component (sensor) </br>
<node_id> - (optional) not used by home assistant </br>
<object_id> - id of device (seperate topics per device)[a-zA-Z0-9_-] Best practice for entities with a unique_id is to set <object_id> to unique_id </br>
```

home assistant default mqtt discovery topic: homeassistant </br>
home assistant discovery payload must be sent as a serialized json </br>

### MQTT home assistant discovery example
```
Device name: esp32d_1
sensor: bme280(temperature, humidity, pressure)
readings: temperature:24.72, humidity:50.53, pressure:101426
```

MQTT configuration topics (each reading type as sperate homeassitant entity)</br>
```homeassistant/sensor/esp32d_1/temperature/config``` </br>
```homeassistant/sensor/esp32d_1/humidity/config``` </br>
```homeassistant/sensor/esp32d_1/pressure/config``` </br>

MQTT configuration payloads
```
{"name":"esp32d_1_temperature","unique_id":"esp32d_1_temperature_uid","availability_topic":"homassistant/esp32d_1/status","payload_available":"online","payload_not_available:"offline:,"state_topic":"homeassistant/sensor/esp32d_1/temperature/state","unit_of_measurement":"C","device_class":"temperature","value_template":"{{value|float|default(0)|round(2)}}","device":{"identifiers:["esp32d_1"],"manufacturer":"esp32d_1","model":"esp32d_1","name":"esp32d_1"}}
```
```
{"name":"esp32d_1_humidity","unique_id":"esp32d_1_humidity_uid","availability_topic":"homassistant/esp32d_1/status","payload_available":"online","payload_not_available:"offline:,"state_topic":"homeassistant/sensor/esp32d_1/humidity/state","unit_of_measurement":"%","device_class":"humidity","value_template":"{{value|float|default(0)|round(2)}}","device":{"identifiers:["esp32d_1"],"manufacturer":"esp32d_1","model":"esp32d_1","name":"esp32d_1"}}
```
```
{"name":"esp32d_1_pressure","unique_id":"esp32d_1_pressure_uid","availability_topic":"homassistant/esp32d_1/status","payload_available":"online","payload_not_available:"offline:,"state_topic":"homeassistant/sensor/esp32d_1/pressure/state","unit_of_measurement":"hPa","device_class":"pressure","value_template":"{{value|int/100|default(0)|round(2)}}","device":{"identifiers:["esp32d_1"],"manufacturer":"esp32d_1","model":"esp32d_1","name":"esp32d_1"}}
```

Example pretified json
```
{
  "name":"esp32d_1_temperature",
  "unique_id":"esp32d_1_temperature_uid",
  "availability_topic":"homassistant/esp32d_1/status",
  "payload_available":"online",
  "payload_not_available:"offline:,
  "state_topic":"homeassistant/sensor/esp32d_1/temperature/state",
  "unit_of_measurement":"C",
  "device_class":"temperature",
  "value_template":"{{value|float|default(0)|round(2)}}",
  "device":{
    "identifiers:[
      "esp32d_1"
    ],
    "manufacturer":"esp32d_1",
    "model":"esp32d_1",
    "name":"esp32d_1"
   }
 }
}
```

MQTT update sensor readings to mqtt state topic as set in config payload
```
homeassistant/sensor/esp32d_1/temperature/state:24.72
```
```
homeassistant/sensor/esp32d_1/humidity/state:50.53
```
```
homeassistant/sensor/esp32d_1/pressure/state:101426
```

### Commands for mqtt client (manually send mqtt)
home assistant mqtt discovery create entity
```
mosquitto_pub -h <host_address> -p 1883 -u <mqtt_user> -P <mqtt_password> -t homeassistant/sensor/esp32d_1/temperature/config -m '{"name":"esp32d_1_temperature","unique_id":"esp32d_1_temperature_uid","availability_topic":"homassistant/esp32d_1/status","payload_available":"online","payload_not_available:"offline:,"state_topic":"homeassistant/sensor/esp32d_1/temperature/state","unit_of_measurement":"C","device_class":"temperature","value_template":"{{value|float|default(0)|round(2)}}","device":{"identifiers:["esp32d_1"],"manufacturer":"esp32d_1","model":"esp32d_1","name":"esp32d_1"}}'
```

home assistant mqtt discovery remove entity (sending blank config removes from home assistant)
```
mosquitto_pub -r -h <host_address> -p 1883 -u <mqtt_user> -P <mqtt_password> -t homeassistant/sensor/test/temperature/config -m '{}'
```

home assistant mqtt update state
```
mosquitto_pub -r -h <host_address> -p 1883 -u <mqtt_user> -P <mqtt_password> -t homeassistant/sensor/test/temperature/state -m '24.72'
```
