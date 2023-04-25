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

MQTT configuration topic </br>
```homeassistant/sensor/<object_id>/config``` </br>
MQTT configuration payload </br>
```
{"name":"","uniq_id":"","stat_t":"homeassistant/sensor/<object_id>/state","unit_of_meas":"","dev_cla":"","val_tpl":"{{value|default(0)|round(2)}}"}
```
