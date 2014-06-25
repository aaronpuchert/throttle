throttle
========

Daemon to regulate the maximum CPU clock based on its temperature.

Compilation & Installation
--------------------------
After cloning the repository and `cd`ing into the main directory, execute `make` to compile. If you want a debug version, run `make debug` instead. This reads the temperature from  a local file `./temp_input` and writes the frequencies to `./cpu#freq`, where `#` runs through the cores.

You can also install the throttle daemon as a system service. At the moment, this is only tested to work on SuSE Linux. Just execute `sudo make install`. You might want to adjust the settings in `throttle.conf` before. (see below) Uninstalling works similarly by `sudo make uninstall`.

Mechanism
---------
CPU frequency drivers typically regulate the core frequencies based purely on demand. For notebooks however, this is not the only factor to consider, especially if you use them for lengthy computations.

We do not override the on-demand mechanisms, instead we modify their input. Whenever the temperature exceeds a certain threshold, we decrease the maximum CPU frequency. If the CPU cools down and reaches another threshold, we increase the maximum frequency again. Both thresholds can be set as `temp_max` and `temp_min` via the configuration file. (see below)

To efficiently regulate a system, one has to take into account the delay between changing the input and the effect on the output. Therefore, we wait at least `wait` seconds after increasing/decreasing the frequency before we check the temperature again. After that, we check every five seconds if the temperature is still in the desired interval.

Configuration
-------------
Parameters are passed to the program via `/etc/trottle.conf`. A template is produced by the Makefile. Further, users can adjust some parameters at runtime via writing to the pipe `/var/run/throttle`. Permissions should be set appropriately.

The following settings are automatically determined at installation time and do not need to be changed under most circumstances.
- `cores`: number of CPU cores
- `temp_file`: a file from which we canread the current CPU temperature, like `/sys/class/hwmon/hwmon0/device/temp1_input`.
- `freq_list`: list of available CPU frequencies: We try to take them from `/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`. If this file doesn't exist, you have to provide the data yourself.
- `freq_set`: files containing the maximum frequency for each core. The cores are numbered 0, 1, 2, ..., you can use `%d` for their number. Example: `/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq`
