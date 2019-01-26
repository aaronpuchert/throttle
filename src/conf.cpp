#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "throttle.hpp"

/**
 * Read a configuration file.
 *
 * This function is guaranteed to read in all syntactically correct files, but
 * might also accept others.
 */
Conf::Conf(const char *config_fn) {
	// open the file
	std::ifstream conf_file(config_fn);
	if (!conf_file)
		throw std::runtime_error(std::string("Couldn't open config file '") + config_fn + '\'');

	// Read every line and put it in the list.
	char line[LINE_LENGTH];
	std::string name;

	// parse: first comes the name of the command
	while (conf_file >> name) {
		// if line starts with '#', ignore it
		if (name[0] == '#') {
			conf_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}

		conf_file.ignore(std::numeric_limits<std::streamsize>::max(), '=');
		conf_file.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
		conf_file.getline(line, LINE_LENGTH);

		DEBUG_PRINT("[Conf] " << name << " = " << line);

		// Write into attribute list
		attributes.emplace_back(name, std::string(line));
	}
}

const std::string& Conf::GetAttr(const char *name) const
{
	auto it = std::find_if(attributes.begin(), attributes.end(),
		[name](const std::pair<std::string, std::string>& p)
		{
			return p.first == name;
		});

	if (it != attributes.end())
		return it->second;
	else
		throw std::runtime_error(std::string("No such attribute: ") + name);
}

/**
 * Construct a command queue.
 *
 * We want to know the parent, where we can write changes to,
 * and which pipe to listen on.
 */
CommQueue::CommQueue(Throttle *parent, const char *pipe_fn) : Throt(parent)
{
	// Open the command pipe
	comm_file = open(pipe_fn, O_RDONLY | O_NONBLOCK);

	if (comm_file < 0)
		throw std::runtime_error(std::string("Couldn't open command pipe '") + pipe_fn + '\'');
}

/**
 * Destruct the command queue.
 *
 * We have to close the file manually here.
 */
CommQueue::~CommQueue()
{
	close(comm_file);
}

/**
 * Look for updates from the command pipe and process them.
 */
void CommQueue::update()
{
	char buf[LINE_LENGTH];
	ssize_t ret;
	while (true) {
		ret = read(comm_file, buf, LINE_LENGTH-1);
		if (ret <= 0)
			break;
		processCommand(std::string(buf, ret));
	}

	// This should be because there's nothing more to read
	if (ret != 0 && errno != EAGAIN)
		throw std::runtime_error("Read error while checking command pipe");
}

/**
 * Process a command coming through the pipe.
 */
void CommQueue::processCommand(const std::string &comm)
{
	std::istringstream stream(comm);

	std::string command;
	stream >> command;

	if (command == "min") {
		int temp;
		stream >> temp;
		Throt->setMinTemp(temp);
		DEBUG_PRINT("[CommQueue] Set minimum temperature to " << temp);
	}
	else if (command == "max") {
		int temp;
		stream >> temp;
		Throt->setMaxTemp(temp);
		DEBUG_PRINT("[CommQueue] Set maximum temperature to " << temp);
	}
	else if (command == "freq") {
		int freq;
		stream >> freq;
		Throt->setOverrideFreq(freq);
		DEBUG_PRINT("[CommQueue] Set frequency to " << freq);
	}
	else if (command == "reset") {
		Throt->setOverrideFreq(0);
		DEBUG_PRINT("[CommQueue] Reset mechanism");
	}
	else {
		DEBUG_PRINT("[CommQueue] Ignored unknown command: " << command);
	}
}
