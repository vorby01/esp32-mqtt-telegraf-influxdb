# Home Assistant
Raspberry Pi 4
2023/04/24

### Write hassos to usb drive
Raspberry pi imager
  - Select home-assistant OS and install to usb

### Raspberry pi 4 hassos poe-hat better fan temperature control
boot/config.txt
```
dtoverlay=rpi-poe
dtparam=poe_fan_temp0=10000,poe_fan_temp0_hyst=1000
dtparam=poe_fan_temp1=55000,poe_fan_temp0_hyst=5000
dtparam=poe_fan_temp2=60000,poe_fan_temp0_hyst=5000
dtparam=poe_fan_temp3=65000,poe_fan_temp0_hyst=5000
```

### Home assistant OS setup
  - Boot usb
  - Home assistant OS requires internet connection to complete installation
  - Web interface http://home-assistant.local:8123
  - Follow prompts to complete install
  - Click on blue dot in web interface to view installation details
  
### Additional resources
#### Install home-assistant add-on: File Editor
```settings > add-ons > add-on store > file editor```
</br>
  
#### File editor allow navigating whole file system
configuration.yaml
```
homeassistant:
  allowlist_external_dirs:
  - /config 
```

#### Home assistant configuration
/config/configuration.yaml
  - hassos default config location </br>
  /config/.storage/core.config_entries

#### Additional logging
configuration.yaml
```
logger:
  #default: info
  default: warning  
  logs:  
    homeassistant.components.mqtt: debug
    homeassistant.core: critical
```

#### Mosquitto Mqtt
  - ended up switching to running a external mqtt broker
  - Add mqtt integration to home assistant </br>
  ```settings > devices & services > add-integration > MQTT```
