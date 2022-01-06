throttle
========

Daemon to regulate the maximum CPU clock based on its temperature.
[![Build Status](https://travis-ci.org/aaronpuchert/throttle.svg?branch=master)](https://travis-ci.org/aaronpuchert/throttle)

Compilation & Installation
--------------------------
After cloning the repository and `cd`ing into the main directory, execute `make` to compile.
If you want a debug version, run `make debug` instead.
The debug version reads the temperature from  a local file `temp_input` and writes the frequencies to `cpu#freq`, where `#` runs through the cores.

The daemon can be installed as [systemd](http://freedesktop.org/wiki/Software/systemd/) service.
Just execute (as root) `make install`, then `systemctl daemon-reload` and `systemctl enable throttle.service`.
With `start` instead of `enable`, you can try the daemon in the current session.
You might want to adjust the settings in `throttle.conf` before (see below).
Uninstalling works similarly by `make uninstall` (as root, of course).

The program can also be called directly via

	throttle [<config file> [<command pipe>]]

If not set, they default to `throttle.conf` and `pipe`, respectively.
Don't forget to run as root, if you want to let it change frequencies.

Mechanism
---------
CPU frequency drivers typically regulate the core frequencies based purely on demand.
For notebooks however, this is not the only factor to consider, especially if you use them for lengthy computations.

We do not override the on-demand mechanisms, instead we modify their input.
Whenever the temperature exceeds a certain threshold, we decrease the maximum CPU frequency.
If the CPU cools down and reaches another threshold, we increase the maximum frequency again.
Both thresholds can be set as `temp_max` and `temp_min` (in °C) via the configuration file (see below).

To efficiently regulate a system, one has to take into account the delay between changing the input and the effect on the output.
Therefore, we wait at least `3*wait` seconds after increasing/decreasing the frequency before we increase/decrease again.
After that, we check every three seconds if the temperature is still in the desired interval.

Configuration
-------------
Parameters are passed to the program via a configuration file.
If installed as a service, this is the file `/etc/trottle.conf`.
A template is produced by the Makefile.
Further, users can adjust some parameters at runtime by writing to a command pipe.
If installed as a service, this is the file `/run/throttle`.
Permissions should be set appropriately.
(The systemd configuration file sets them such that users can write to the pipe.)

The following settings are automatically determined at installation time and do not need to be changed under most circumstances.
- `cores`: number of CPU cores
- `temp_file`: a file from which we can read the current CPU temperature, like `/sys/class/hwmon/hwmon0/device/temp1_input`.
- `freq_list`: list of available CPU frequencies: We try to read them from `/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`.
  If this file doesn't exist, you have to provide the data yourself.
- `freq_set_prefix` and `freq_set_suffix`: describes thefiles containing the maximum frequency for each core. The cores are numbered 0, 1, 2, and so on.
  The file names are constructed as `freq_set_prefix + core number + freq_set_suffix`.
  Example: `"/sys/devices/system/cpu/cpu" + 0 + "/cpufreq/scaling_max_freq"`

The behavior of the throttling daemon is controlled by the settings `temp_min`, `temp_max`, and `wait`.
These were explained in the last section.

Real-time control
-----------------
You can control the throttling in real-time by sending commands to `/run/throttle` (if installed as service, or any other file you set in the command line).
This can be done by

	echo "<command>" >/run/throttle

Currently, the following commands are available:

*	`max n` sets the maximum temperature to n,
*	`min n` sets the minimum temperature to n, both in °C.
*	`freq n` sets the current (maximum) frequency directly to n MHz.
	This overrides the throttling mechanism.
*	`reset` reinstates the throttling mechanism.
