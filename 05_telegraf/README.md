# telegraf
Read and convert data to store in influxdb

### Install influxdata key and repository
```
wget -qO- https://repos.influxdata.com/influxdata-archive_compat.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg > /dev/null
echo "deb [signed-by=etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg] https://repos.influxdata.com/debian stable main" | sudo tee /etc/apt/sources.list.d/influxdata.list
sudo apt update

```

### Install telegraf
```
sudo apt install telegraf
```

### Configure telegraf
/etc/telegraf/telegraf.conf
  - will require influxdb token for read/write access

<pre>
[agent]
interval = "10s"
round_interval = true
metric_batch_size = 1000
metric_buffer_limit = 10000
collection_jitter = "0s"
flush_interval = "10s"
flush_jitter = "0s"
precision = "0s"
debug = true
#quiet = false
#logtarget = "file"
logfile = "/var/log/telegraf/telegraf.log
#logfile_rotation_interval = "24h"
#logfile_rotation_max_size = "100MB"
#logfile_rotation_max_archives = 5
#log_with_timezone = "local"
hostname = ""
omit_hostname = true

##### OUTPUT PLUGINS
urls = ["http://127.0.0.1:8086"]
token = ""
organization = ""
bucket = ""

</pre>


### Test telegraf configuration
```
telegraf --config /etc/telegraf/telegraf.conf --test
```

### Start telegraf
```
sudo systemctl enable telegraf
sudo systemctl start telegraf
```

### File output and permissions
/etc/telegraf/telegraf.conf
```
[[outputs.file]]
  files = ["stdout", "/data/telegraf.out"]
```
```
sudo mkdir /data
sudo chown user /data
touch /data/telegraf.out
sudo chgrp /data/telegraf.out telegraf
sudo chmod 766 /data/telegraf.out
```
