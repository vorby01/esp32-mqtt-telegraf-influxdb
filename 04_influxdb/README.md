# Infludb
influxdb (time-series database)

### Install influxdata key and repository (2023 04 24)
```
wget -qO- https://repos.influxdata.com/influxdata-archive_compat.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg > /dev/null
echo "deb [signed-by=etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg] https://repos.influxdata.com/debian stable main" | sudo tee /etc/apt/sources.list.d/influxdata.list
sudo apt update

```

### Install influxdb
```
sudo apt install influxdb2
```

### Configure telegraf
/etc/influxdb/config.toml

#### influxdb telementry is enabled by default, to disable
/etc/influxdb/config.toml
```
reporting-disabled = true
```

### Start Influxdb
sudo systemctl enable influxdb
sudo systemctl start influxdb




### Additional Influxdb commands
Check influxdb2 install paths
```
dpkg -L influxdb2
```

Check if influxdb2 is masked
```
systemctl list-unit-files | grep masked
```

Check influx authorization token
```
influx auth list
```

Clear data from influxdb bucket
```
influx delete --org "" --bucket "" --token "" --start "1970-01-01T00:00:00Z" --stop $(date +"%Y-%m-%dT%H:%M:%SZ")
```

Influxdb enviroment variables
~/.influxdbv2/configs
<pass>
[default]
  url = "http://localhost:8086"
  token = ""
  org = ""
  active = true
</pass>

