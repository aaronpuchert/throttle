#include "throttle.hpp"
#include <unistd.h>

/*
 * Constructor of a Throttle object. The argument config_fn
 * should be the name of the configuration file.
 */
Throttle::Throttle(const char *config_fn, const char* pipe_fn) : queue(this, pipe_fn), term(false), override(false)
{
	// open the configuration file
	Conf conf(config_fn);

	// read all variables
	conf.GetAttr("cores", &cores);
	conf.GetAttr("temp_file", &temp_fn);
	conf.GetAttr("freq_set_prefix", &freq_fn_prefix);
	conf.GetAttr("freq_set_suffix", &freq_fn_suffix);
	conf.GetAttr("freq_list", &freqs);
	conf.GetAttr("temp_min", &temp_min);
	conf.GetAttr("temp_max", &temp_max);
	conf.GetAttr("wait", &wait_after_adjust);

	// multiply temperatures with 1000
	temp_min *= 1000;
	temp_max *= 1000;

	// read the current frequency (is that the right thing to do?)
	std::ifstream freq_file(freq_fn_prefix + "0" + freq_fn_suffix);
	freq_file >> freq;

#ifdef DEBUG
	std::cout << "[Throttle] Initial frequency: " << (float)freq/1000 << " MHz" << std::endl;
#endif
}

/*
 * Adjust the frequency according to the current temperature.
 * Returns the number of seconds to wait until the next adjustment.
 */
int Throttle::adjust()
{
	int temp = readTemp();
	std::set<int>::iterator new_freq = freqs.end();

#ifdef DEBUG
	std::cout << "[Throttle] Temperature: " << (float)temp/1000 << "°C"<< std::endl;
#endif

	// If the temperature exceeds a threshold, find the next best frequency.
	if (temp > temp_max)
		new_freq = --freqs.lower_bound(freq);
	if (temp < temp_min)
		new_freq = freqs.upper_bound(freq);

	// Have we found one? Then adjust.
	if (new_freq != freqs.end()) {
		freq = *new_freq;
		writeFreq();
		return wait_after_adjust;
	}
	else
		return wait;
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
void Throttle::writeFreq() const
{
#ifdef DEBUG
	std::cout << "[Throttle] New frequency: " << (float)freq/1000 << " MHz" << std::endl;
#endif

	// loop over the files, write the frequency in each
	for (int core=0; core<cores; ++core) {
		std::ostringstream freq_fn;
		freq_fn << freq_fn_prefix << core << freq_fn_suffix;
		std::ofstream freq_file(freq_fn.str());
		freq_file << freq;
	}
}

/*
 * Run the throttle daemon. We stop if someone writes "quit" to the command pipe.
 */
void Throttle::run()
{
	while (!term) {
		int waitsec = wait;
		if (!override)
			waitsec = adjust();
		sleep(waitsec);
	}
}
