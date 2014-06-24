#include <set>
#include <map>
#include <queue>
#include <fstream>
#include <exception>
#include <sstream>
#include <pthread.h>

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
};

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
template<> void Conf::parse<std::set<int>>(const std::string &str, std::set<int> *ret);

//----------------------------------
// Class handling the command queue
//----------------------------------
class CommQueue {
public:
	CommQueue(Throttle *parent, const char *pipe_fn = "/var/run/throttle");

protected:
	// where we write to
	Throttle *Throt;

	// thread main function
	static void *watchPipe(void *);

	// look for input and process
	void processCommand(std::string &comm);

	// command pipe
	std::ifstream comm_pipe;
};

//---------------------------------------
// Main class controlling the throttling
//---------------------------------------
class Throttle {
public:
	Throttle(const char *config_fn);

	friend CommQueue;
protected:
	void adjust();
	int readTemp() const;
	void writeFreq();

	// number of cores
	int cores;

	// files
	std::string temp_fn;
	std::string freq_fn;

	// temperature thresholds
	int temp_max, temp_min;

	// frequencies
	std::set<int> freqs;
	int freq;

	// dynamics
	int wait;

	// status & override mechanics
	CommQueue queue;
	// ...
};
