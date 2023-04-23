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

### Test telegraf configuration
```
telegraf --config /etc/telegraf/telegraf.conf --test
```

### Start telegraf
```
sudo systemctl enable telegraf
sudo systemctl start telegraf
```

