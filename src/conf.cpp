#include "throttle.hpp"
#include <unistd.h>
#include <fcntl.h>

/*
 * Parsing function: for strings we can do better
 */
template<> void Conf::parse<std::string>(const std::string &str, std::string *ret)
{
	*ret = str;
}

/*
 * Configuration reader.
 * This function is guaranteed to read in all syntactically correct files, but might also accept others.
 */
Conf::Conf(const char *config_fn) {
	// open the file
	std::ifstream conf_file(config_fn);

	// read every line and put it in the map
	char line[LINE_LENGTH];
	std::string name;

	// parse: first comes the name of the command
	while (conf_file >> name) {
		// if line starts with '#', ignore it
		if (name[0] == '#') {
			conf_file.ignore(LINE_LENGTH, '\n');
			continue;
		}

		conf_file.ignore(LINE_LENGTH, '=');
		conf_file.ignore(LINE_LENGTH, ' ');
		conf_file.getline(line, LINE_LENGTH);

#ifdef DEBUG
		std::cout << "[Conf] " << name << " = " << line << std::endl;
#endif

		// write into map
		attributes[name] = std::string(line);
	}
}

/*
 * Construct a command queue. We want to know the parent, where we can write changes to,
 * and which pipe to listen on.
 */
CommQueue::CommQueue(Throttle *parent, const char *pipe_fn) : Throt(parent)
{
	// init translation table
	translate["max"] = SET_MAX;
	translate["min"] = SET_MIN;
	translate["freq"] = SET_FREQ;
	translate["reset"] = RESET;
	translate["quit"] = QUIT;

	// Open the command pipe
	comm_file = open(pipe_fn, O_NONBLOCK);
}

CommQueue::~CommQueue()
{
	close(comm_file);
}

/*
 * Look for updates from the command pipe and process them.
 */
void CommQueue::update()
{
	char buf[LINE_LENGTH];
	ssize_t ret;
	do {
		ret = read(comm_file, buf, LINE_LENGTH-1);
		if (ret > 0)
			processCommand(std::string(buf, ret));
	} while (ret > 0);

	// This should be because there's nothing more to read
	if (ret != 0 && errno != EAGAIN)
		throw std::runtime_error("Read error while checking command pipe");
}

/*
 * Process a command coming through the pipe.
 */
void CommQueue::processCommand(const std::string &comm)
{
	std::istringstream stream(comm);
	std::string command;
	stream >> command;

	int value;
	switch (translate.find(command)->second) {
	case DEFAULT:		// we land here if "command" is not found
#ifdef DEBUG
		std::cout << "[CommQueue] Ignored unknown command: " << command << std::endl;
#endif
		break;
	case SET_MIN:
		stream >> value;
		Throt->setMinTemp(value);
#ifdef DEBUG
		std::cout << "[CommQueue] Set minimum temperature to " << value << std::endl;
#endif
		break;
	case SET_MAX:
		stream >> value;
		Throt->setMaxTemp(value);
#ifdef DEBUG
		std::cout << "[CommQueue] Set maximum temperature to " << value << std::endl;
#endif
		break;
	case SET_FREQ:
		stream >> value;
		Throt->setOverrideFreq(value);
#ifdef DEBUG
		std::cout << "[CommQueue] Set frequency to " << value << std::endl;
#endif
		break;
	case RESET:
		Throt->setOverrideFreq(0);
#ifdef DEBUG
		std::cout << "[CommQueue] Reset mechanism" << std::endl;
#endif
		break;
	case QUIT:
		Throt->term = true;
		break;
	}
}
