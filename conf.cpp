#include "throttle.hpp"
#include <unistd.h>

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
CommQueue::CommQueue(Throttle *parent, const char *pipe_fn) : Throt(parent), comm_pipe(pipe_fn)
{
	// init translation table
	translate["max"] = SET_MAX;
	translate["min"] = SET_MIN;
	translate["freq"] = SET_FREQ;
	translate["reset"] = RESET;
	translate["quit"] = QUIT;

	// start the thread
	if (pthread_create(&thread, NULL, watchPipe, this))
		throw std::runtime_error("Could not create thread.");
}

/*
 * Thread main function: `void *obj` should point to the generating object.
 * We watch the pipe for input.
 */
void *CommQueue::watchPipe(void *obj)
{
	CommQueue *that = (CommQueue *)obj;

	// open the pipe
	std::ifstream pipe(that->comm_pipe, std::ifstream::in);

	char buf[LINE_LENGTH];
	int length;
	do {
		length = pipe.readsome(buf, LINE_LENGTH-1);
		if (length)
			that->processCommand(std::string(buf, length));
		sleep(Throttle::wait);
	} while (!that->Throt->term);

	return 0;
}

/*
 * Process a command coming through the pipe.
 */
void CommQueue::processCommand(const std::string comm)
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
		Throt->temp_min = value*1000;
#ifdef DEBUG
		std::cout << "[CommQueue] Set minimum temperature to " << value << std::endl;
#endif
		break;
	case SET_MAX:
		stream >> value;
		Throt->temp_max = value*1000;
#ifdef DEBUG
		std::cout << "[CommQueue] Set maximum temperature to " << value << std::endl;
#endif
		break;
	case SET_FREQ:
		stream >> value;
		Throt->freq = value*1000;
		Throt->override = true;
		Throt->writeFreq();
#ifdef DEBUG
		std::cout << "[CommQueue] Set frequency to " << value << std::endl;
#endif
		break;
	case RESET:
		Throt->override = false;
#ifdef DEBUG
		std::cout << "[CommQueue] Reset mechanism" << std::endl;
#endif
		break;
	case QUIT:
		Throt->term = true;
		break;
	}
}
