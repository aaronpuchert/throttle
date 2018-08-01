#ifndef THROTTLE_HPP
#define THROTTLE_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef DEBUG
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
		void GetAttr(const char *name, T *output) const;

private:
	template<typename T> static void parse(const std::string &str, T *ret);
	template<typename T> static void parse(const std::string &str, std::vector<T> *ret);

	// Sorted lexicographically by the first component.
	std::vector<std::pair<std::string, std::string>> attributes;
};

// IMPLEMENTATION OF THE TEMPLATES
template<typename T> void Conf::GetAttr(const char *name, T* output) const
{
	parse(GetAttr(name), output);
}

// Well, let's try something general first
template<typename T> void Conf::parse(const std::string &str, T *ret)
{
	std::istringstream stream(str);
	stream >> *ret;
}

// ... but we might want to specialize
template<> inline void Conf::parse<std::string>(const std::string &str, std::string *ret)
{
	*ret = str;
}

// For sets we do something completely different
template<typename T> void Conf::parse(const std::string &str, std::vector<T> *ret)
{
	std::istringstream stream(str);
	T cur;

	while (stream >> cur)
		ret->push_back(cur);
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

	// commands
	enum Command {
		SET_MAX,
		SET_MIN,
		SET_FREQ,
		RESET
	};
};

//---------------------------------------
// Main class controlling the throttling
//---------------------------------------
class Throttle {
public:
	Throttle(const char *config_fn, const char* pipe_fn);
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
	// number of cores
	int cores;

	// files
	std::string temp_fn;
	std::string freq_fn_prefix, freq_fn_suffix;

	// temperature thresholds
	int temp_max, temp_min;

	// Sorted frequencies.
	std::vector<int> freqs;
	int freq;

	// dynamics
	int wait_after_adjust;

	// Set to +wait_after_adjust after increasing frequency, to -wait_after_adjust after decreasing it.
	// Then it is diminished in absolute value after each step of adjusting.
	int stabilize;

	// STATUS & Override mechanics
	CommQueue queue;
	int override_freq;	// 0 = no override
};

#endif
