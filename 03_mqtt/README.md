# Mosquitto MQTT
mqtt broker

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
  cat pwfile
  
  - move password file to mosquitto directory: </br>
  ```sudo mv pwfile /etc/mosquitto/```
  
### Configure mosquitto mqtt broker
/etc/mosquitto/mosquitto.conf
<pass>
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
</pass>
