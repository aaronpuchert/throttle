#include <set>
#include <map>
#include <queue>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <sstream>

#ifdef DEBUG
#include <iostream>
#define DEBUG_PRINT(...) std::cout << __VA_ARGS__ << std::endl
#else
#define DEBUG_PRINT(...)
#endif

// Maximum config file line length
#define LINE_LENGTH 1024

// forward declaring the classes
class Conf;
class CommQueue;
class Throttle;

//---------------------------------------------
// Class for reading in the configuration file
//---------------------------------------------
class Conf {
public:
	Conf(const char *config_fn);
	template<typename T>
		void GetAttr(const std::string &name, T *output) const;

private:
	template<typename T> static void parse(const std::string &str, T *ret);
	template<typename T> static void parse(const std::string &str, std::set<T> *ret);

	// attributes
	std::map<std::string, std::string> attributes;
};

// IMPLEMENTATION OF THE TEMPLATES
template<typename T> void Conf::GetAttr(const std::string &name, T* output) const
{
	auto attr = attributes.find(name);
	if (attr != attributes.end())
		parse(attr->second, output);
	else
		throw std::runtime_error("No such attribute: " + name);
}

// Well, let's try something general first
template<typename T> void Conf::parse(const std::string &str, T *ret)
{
	std::istringstream stream(str);
	stream >> *ret;
}

// ... but we might want to specialize
template<> void Conf::parse<std::string>(const std::string &str, std::string *ret);

// For sets we do something completely different
template<typename T> void Conf::parse(const std::string &str, std::set<T> *ret)
{
	std::istringstream stream(str);
	T cur;

	while (stream >> cur)
		ret->insert(cur);
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
		DEFAULT = 0,
		SET_MAX,
		SET_MIN,
		SET_FREQ,
		RESET
	};

	static const std::map<std::string, Command> translate;
};

//---------------------------------------
// Main class controlling the throttling
//---------------------------------------
class Throttle {
public:
	Throttle(const char *config_fn, const char* pipe_fn);
	void run();

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

	bool term;

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

	// frequencies
	std::set<int> freqs;
	int freq;

	// dynamics
	int wait_after_adjust;
	static const int wait = 3;

	// Set to +wait_after_adjust after increasing frequency, to -wait_after_adjust after decreasing it.
	// Then it is diminished in absolute value after each step of adjusting.
	int stabilize;

	// STATUS & Override mechanics
	CommQueue queue;
	int override_freq;	// 0 = no override
};
