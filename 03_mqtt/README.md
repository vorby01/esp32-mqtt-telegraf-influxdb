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
