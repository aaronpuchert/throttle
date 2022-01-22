#ifndef THROTTLE_HPP
#define THROTTLE_HPP

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef NDEBUG
#include <iostream>
#define DEBUG_PRINT(...) std::cout << __VA_ARGS__ << std::endl
#else
#define DEBUG_PRINT(...)
#endif

// Maximum config file line length
constexpr std::streamsize LINE_LENGTH = 1024;

// forward declaring the classes
class Throttle;

//---------------------------------------------
// Class for reading in the configuration file
//---------------------------------------------
class Conf {
public:
	Conf(const char *config_fn);
	const std::string& GetAttr(const char *name) const;
	template<typename T>
	T ParseAttr(const char *name) const;

private:
	template<typename T>
	struct Parser {
		static T parse(const std::string &str);
	};
	template<typename T>
	struct Parser<std::vector<T>> {
		static std::vector<T> parse(const std::string &str);
	};

	// Sorted lexicographically by the first component.
	std::vector<std::pair<std::string, std::string>> attributes;
};

// IMPLEMENTATION OF THE TEMPLATES
template<typename T>
T Conf::ParseAttr(const char *name) const
{
	return Parser<T>::parse(GetAttr(name));
}

template<typename T>
T Conf::Parser<T>::parse(const std::string &str)
{
	T ret;
	std::istringstream stream(str);
	stream >> ret;
	return ret;
}

// Unsupported, GetAttr should be used directly.
template<>
std::string Conf::Parser<std::string>::parse(const std::string &) = delete;

template<typename T>
std::vector<T> Conf::Parser<std::vector<T>>::parse(const std::string &str)
{
	std::istringstream stream(str);

	std::vector<T> ret;
	T cur;
	while (stream >> cur)
		ret.push_back(cur);
	return ret;
}

//----------------------------------
// Class handling the command queue
//----------------------------------
class CommQueue {
public:
	CommQueue(Throttle *parent, const char *pipe_fn);
	~CommQueue();
	void update();

private:
	// where we write to
	Throttle *Throt;

	// look for input and process
	void processCommand(const std::string &comm);

	// command pipe file descriptor
	int comm_file;
};

//---------------------------------------
// Main class controlling the throttling
//---------------------------------------
class Throttle {
	class FreqFilenames {
	public:
		FreqFilenames(const std::string& prefix, const std::string& suffix,
		              unsigned numCores);
		const char* operator[](unsigned core) const;
		unsigned size() const { return numCores; }

	private:
		unsigned numCores;
		mutable std::vector<std::string> templates;
		size_t prefix_size;
	};

public:
	static Throttle createFromConfig(const char *config_fn, const char* pipe_fn);

	Throttle(std::string &&temp_filename, FreqFilenames &&freq_filenames,
	         std::vector<int> &&frequencies, const char* pipe_fn, int wait);
	void operator()();

	/**
	 * Set new override frequency in MHz or reset (with freq=0).
	 */
	void setOverrideFreq(int freq)
	{
		override_freq = freq*1000;
		stabilize = 0;
	}

	/**
	 * Set minimum temperature in degrees Celsius.
	 */
	void setMinTemp(int temp) { temp_min = temp*1000; }

	/**
	 * Set maximum temperature in degrees Celsius.
	 */
	void setMaxTemp(int temp) { temp_max = temp*1000; }

private:
	void adjust();
	int readTemp() const;
	void writeFreq() const;

	// SETTINGS
	// files
	const std::string temp_fn;
	const FreqFilenames freq_fn;

	// temperature thresholds
	int temp_max, temp_min;

	// Sorted frequencies.
	const std::vector<int> freqs;
	int freq;

	// dynamics
	const int wait_after_adjust;

	// Set to +wait_after_adjust after increasing frequency, to -wait_after_adjust after decreasing it.
	// Then it is diminished in absolute value after each step of adjusting.
	int stabilize;

	// STATUS & Override mechanics
	CommQueue queue;
	int override_freq;	// 0 = no override
};

#endif
