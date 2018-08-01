#include "throttle.hpp"

/**
 * Constructor of a Throttle object. The argument config_fn
 * should be the name of the configuration file.
 */
Throttle::Throttle(const char *config_fn, const char* pipe_fn)
	: stabilize(0), queue(this, pipe_fn), override_freq(0)
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

	DEBUG_PRINT("[Throttle] Initial frequency: " << (float)freq/1000 << " MHz");
}

/**
 * Execute one feedback cycle.
 *
 * Reads the current temperature and adapts the maximum frequency, if necessary.
 */
void Throttle::operator()()
{
	// Look if there is new input in the pipe
	queue.update();

	if (override_freq) {
		if (freq != override_freq) {
			freq = override_freq;
			writeFreq();
		}
	}
	else
		adjust();
}

/**
 * Adjust the frequency according to the current temperature.
 * Returns the number of seconds to wait until the next adjustment.
 */
void Throttle::adjust()
{
	int temp = readTemp();
	std::set<int>::iterator new_freq = freqs.end();

	DEBUG_PRINT("[Throttle] Temperature: " << (float)temp/1000 << "°C");

	// slowly reset stabilize counter
	if (stabilize > 0) --stabilize;
	if (stabilize < 0) ++stabilize;

	// If the temperature exceeds a threshold, find the next best frequency.
	if (temp > temp_max && stabilize >= 0) {
		new_freq = --freqs.lower_bound(freq);
		stabilize = -wait_after_adjust;
	}
	if (temp < temp_min && stabilize <= 0) {
		new_freq = freqs.upper_bound(freq);
		stabilize = wait_after_adjust;
	}

	// Have we found one? Then adjust.
	if (new_freq != freqs.end()) {
		freq = *new_freq;
		writeFreq();
	}
}

/**
 * Read and return the current CPU temperature.
 */
int Throttle::readTemp() const
{
	int temp;
	std::ifstream temp_file(temp_fn);
	temp_file >> temp;
	return temp;
}

/**
 * Write the determined frequency to the specific files.
 */
void Throttle::writeFreq() const
{
	DEBUG_PRINT("[Throttle] New frequency: " << (float)freq/1000 << " MHz");
	std::string freq_fn = freq_fn_prefix + '#' + freq_fn_suffix;

	// loop over the files, write the frequency in each
	for (int core=0; core<cores; ++core) {
		freq_fn[freq_fn_prefix.length()] = '0' + char(core);
		std::ofstream freq_file(freq_fn);
		freq_file << freq;
	}
}
