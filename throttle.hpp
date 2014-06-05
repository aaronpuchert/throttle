#include <set>
#include <map>
#include <queue>
#include <fstream>
#include <exception>
#include <sstream>
#include <pthread.h>

//---------------------------------------------
// Class for reading in the configuration file
//---------------------------------------------
class Conf {
public:
	Conf(std::string &config_fn);
	template<typename T>
		void GetAttr<T>(std::string &name, T *output);

protected:
	// attributes
	std::map<std::string, std::string> attributes;

private:
	template<typename T> static void parse(std::string &str, T *ret);
};

template<typename T> void Conf::GetAttr<T>(std::string &name, T *output)
{
	auto attr = attributes.find(name);
	if (attr != attributes.end())
		parse(attr.second, output);
	else
		throw std::runtime_error("No such attribute: " + name);
}

// Well, let's try something general first
template<typename T> void Conf::parse<T>(std::string &str, T *ret)
{
	std::istringstream stream(str);
	stream >> *ret;
}

// For strings we can do better
template<> void Conf::parse<std::string>(std::string &str, std::string *ret)
{
	*ret = str;
}

// ... and there might be integer vectors. Do we have to handle them separately?

//----------------------------------
// Class handling the command queue
//----------------------------------
class CommQueue {
public:
	CommQueue(std::string &pipe_fn = "/var/run/throttle");

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
	Throttle(std::string &config_fn);

	friend CommQueue;
protected:
	void adjust();
	int readTemp() const;
	int writeFreq();

	// files
	std::string temp_fn;
	std::string freq_fn;

	// temperature thresholds
	int temp_max, temp_min;

	// frequencies
	std::set<int> freqs;
	int freq;

	// status & override mechanics
	// ...
};
