#include "throttle.hpp"
#include <cstdio>

/*
 * Constructor of a Throttle object. The argument config_fn
 * sould be the name of the configuration file.
 */
Throttle::Throttle(const char *config_fn) : queue(this)
{
	// open the configuration file
	Conf conf(config_fn);

	// read all variables
	conf.GetAttr("cores", &cores);
	conf.GetAttr("temp_file", &temp_fn);
	conf.GetAttr("freq_set", &freq_fn);
	conf.GetAttr("freq_list", &freqs);
	conf.GetAttr("temp_min", &temp_min);
	conf.GetAttr("temp_max", &temp_max);
	conf.GetAttr("wait", &wait);

	// read the current frequency (is that the right thing to do?)
	const char filename[freq_fn.size() + PADDING];
	std::snprintf(filename, freq_fn.size() + PADDING, freq_fn, core);
	std::ofstream freq_file(filename);
	freq_file >> freq;
}

/*
 * Adjust the frequency according to the current temperature.
 * Returns the number of seconds to wait until the next adjustment.
 */
int Throttle::adjust()
{
	int temp = readTemp();
	std::set<int>::iterator new_freq;

	// If the temperature exceeds a threshold, find the next best frequency.
	if (temp > temp_max)
		new_freq = freqs.lower_bound(freq)-1;
	if (temp < temp_min)
		new_freq = freqs.upper_bound(freq);

	// Have we found one? Then adjust.
	if (new_freq != freqs.end()) {
		freq = *new_freq;
		writeFreq();
		return wait;
	}
	else
		return 1;
}

/*
 * Read and return the current CPU temperature.
 */
int Throttle::readTemp() const
{
	int temp;
	std::ifstream temp_file(temp_fn);
	temp_file >> temp;
	return temp;
}

/*
 * Write the determined frequency to the specific files.
 */
void Throttle::writeFreq()
{
	// loop over the files, write the frequency in each
	for (int core=0; core<cores; ++core) {
		const char filename[freq_fn.size() + PADDING];
		std::snprintf(filename, freq_fn.size() + PADDING, freq_fn, core);
		std::ofstream freq_file(filename);
		freq_file << freq;
	}
}
