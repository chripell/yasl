# homergw

`homergw` is a system to store data from various sources for
monitoring purposes. It is made of producer and consumer modules.

## datastore

Consumer, stores data to a sqlite databse. Parameters:

* `--data-store-flush-time`
* `--data-store-db`

Dependencies:

* `pip install julian`
* `pip install aiosqlite`

## disphat

Consumer, shows information on the Pimoroni Disphat. You can use the
left buttons to scroll and enable/disabling auto-scrolling.Parameters:

* `--disphat-values='T3_52pi:T,PL_8.8.8.8:Lat,P%_8.8.8.8:Loss'`
* `--disphat-contrast` 
* `--disphat-showtime`
* `--disphat-hue`
* `--disphat-alerts='V["PL_8.8.8.8"]>10'`

Dependencies:

* `pip install dot3k`

## pinger

Producer, runs ping and populates the following variables:

* `P%_[IP]` packet loss
* `PL_[IP]` average latency

Parameters:

* `--pinger-c`
* `--pinger-w`
* `--pinger-ip`

Dependencies:

* `ping` installed

## sensor52

Producer, fetches data from the 52pi sensorhub:

* `PR_52pi` presence
* `S1_52pi`, `T1_52pi` external temperature status and value.
* `S2_52pi`, `LUX_52pi` lux status and value.
* `S2_52pi`, `T2_52pi`, `HU_52pi` on-board status, temp and humidity.
* `S3_52pi`, `T3_52pi`, `PR_52pi` VMP280 status, temp and pressure.

Dependencies:

* `pip install smbus`
* `pip install julian`

Parameters:

* `--sensor52-refresh-time`

## getmqtt

Producer. Gets data from a mqtt broker. The data produced is defined
by the user with the `--getmqtt-topics` option.

Dependencies:

* `pip install asyncio-mqtt`

Parameters:

* `--getmqtt-server`
* `--getmqtt-filter`
* `--getmqtt-topics`

## bwe

Producer, gets the bandwidth and the packet/s on network interfaces:

* `RXB_[interface]`
* `TXB_[interface]`
* `RXP_[interface]`
* `TXP_[interface]`

Parameters:

* `--bwe-refresh-time`
* `--bwe-ifaces`

## sb

Producer, handles a [Sensor Blue module via
Bluetooth](https://www.evolware.org/?p=642).

Data:

* `SB_BAT_{label}`
* `SB_T_{label}`
* `SB_HUM_{label}`

Parameters:

* `--sb-list` comma separated list of label@mac.
* `--sb-minsec` minimum time between data logged.
