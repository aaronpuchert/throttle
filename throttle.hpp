#include <set>
#include <map>
#include <queue>
#include <fstream>
#include <exception>
#include <sstream>
#include <pthread.h>

#ifdef DEBUG
#include <iostream>
#endif

// Maximum config file line length
#define LINE_LENGTH 1024

// Define padding for clock file names
#define PADDING 4

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
		void GetAttr(const std::string &name, T *output);

protected:
	// attributes
	std::map<std::string, std::string> attributes;

private:
	template<typename T> static void parse(const std::string &str, T *ret);
	template<typename T> static void parse(const std::string &str, std::set<T> *ret);
};

// IMPLEMENTATION OF THE TEMPLATES
template<typename T> void Conf::GetAttr(const std::string &name, T *output)
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

protected:
	// where we write to
	Throttle *Throt;

	// thread handle and main function
	pthread_t thread;
	static void *watchPipe(void *);

	// look for input and process
	void processCommand(const std::string comm);

	// command pipe file name
	std::string comm_pipe;
};

//---------------------------------------
// Main class controlling the throttling
//---------------------------------------
class Throttle {
public:
	Throttle(const char *config_fn, const char* pipe_fn);
	void run();

	friend CommQueue;
protected:
	int adjust();
	int readTemp() const;
	void writeFreq();

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
	static const int wait = 1;

	// STATUS & Override mechanics
	CommQueue queue;
	bool term;
	// ...
};
