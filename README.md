# esp32-mqtt-telegraf-influxdb
info about programing a esp32 and collecting and storing the information
this is a work in progress for my own learning, but also if the information can help others

## Software
  - Home-assistant (hassos)(rpi4)
  - Mosquitto mqtt broker (standalone)
  - influxdb v2 (time-series database) (standalone)
  - telegraf (agent for translating data from mqtt to influxdb) (standalone)
  
#### home assistant discovery
  - This mqtt topic structure is to suit home assistant and allow automatic device discovery
  - Could not find many other examples on the net that allowed home assistant to find the mqtt devices without requireing having to edit configuration.yaml to add them
  - currently have home assistant and influxdb in parallel, both reading seperatly from mqtt.
    - homeassitant displaying live data
    - influxdb storing long term sensor data
