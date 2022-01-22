#include "throttle.hpp"
#include <algorithm>
#include <fstream>

static unsigned log10(unsigned x)
{
	// Not the fastest implementation, but we don't expect very large numbers.
	unsigned log = 0;
	while ((x /= 10) != 0)
		++log;
	return log;
}

Throttle::FreqFilenames::FreqFilenames(
	const std::string &prefix, const std::string &suffix, unsigned numCores)
	: numCores(numCores), templates(log10(numCores) + 1), prefix_size(prefix.size())
{
	size_t suffix_size = suffix.size();
	for (unsigned i = 0; i != templates.size(); ++i) {
		templates[i].reserve(prefix_size + (i + 1) + suffix_size);
		templates[i] = prefix;
		templates[i].append(i + 1, '#');
		templates[i] += suffix;
		DEBUG_PRINT("[Throttle] Template for length " << i + 1 << ": " << templates[i]);
	}
}

const char *Throttle::FreqFilenames::operator[](unsigned core) const
{
	unsigned bucket = log10(core);
	std::string &ret = templates[bucket];
	unsigned print = core;
	do {
		ret[prefix_size + bucket] = '0' + char(print % 10);
		print /= 10;
	} while (bucket--);
	DEBUG_PRINT("[Throttle] Filename for core " << core << ": " << ret);
	return ret.c_str();
}

Throttle Throttle::createFromConfig(const char *config_fn, const char* pipe_fn)
{
	// open the configuration file
	Conf conf(config_fn);

	// read all variables
	std::string temp_fn = conf.GetAttr("temp_file");
	std::vector<int> freqs = conf.ParseAttr<std::vector<int>>("freq_list");
	std::sort(freqs.begin(), freqs.end());

	Throttle ret(std::move(temp_fn),
	             FreqFilenames(conf.GetAttr("freq_set_prefix"),
	                           conf.GetAttr("freq_set_suffix"),
	                           conf.ParseAttr<unsigned>("cores")),
	             std::move(freqs), pipe_fn, conf.ParseAttr<int>("wait"));
	ret.setMinTemp(conf.ParseAttr<int>("temp_min"));
	ret.setMaxTemp(conf.ParseAttr<int>("temp_max"));
	return ret;
}

Throttle::Throttle(std::string &&temp_filename, FreqFilenames &&freq_filenames,
                   std::vector<int> &&frequencies, const char* pipe_fn, int wait)
	: temp_fn(std::move(temp_filename)),
	  freq_fn(std::move(freq_filenames)),
	  freqs(std::move(frequencies)),
	  wait_after_adjust(wait),
	  stabilize(0), queue(this, pipe_fn), override_freq(0)
{
	// read the current frequency (is that the right thing to do?)
	std::ifstream(freq_fn[0]) >> freq;

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
	std::vector<int>::iterator new_freq = freqs.end();

	DEBUG_PRINT("[Throttle] Temperature: " << (float)temp/1000 << "°C");

	// slowly reset stabilize counter
	if (stabilize > 0) --stabilize;
	if (stabilize < 0) ++stabilize;

	// If the temperature exceeds a threshold, find the next best frequency.
	if (temp > temp_max && stabilize >= 0) {
		new_freq = std::lower_bound(freqs.begin(), freqs.end(), freq);
		if (new_freq == freqs.begin())
			return;     // There is nothing we can do.
		else
			--new_freq;
		stabilize = -wait_after_adjust;
	}
	if (temp < temp_min && stabilize <= 0) {
		new_freq = std::upper_bound(freqs.begin(), freqs.end(), freq);
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

	// loop over the files, write the frequency in each
       for (unsigned core = 0; core != freq_fn.size(); ++core) {
		std::ofstream freq_file(freq_fn[core]);
		freq_file << freq;
	}
}
