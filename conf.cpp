#include "throttle.hpp"
#include <stdexcept>
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
 * Construct a command queue. We want to now the parent, where we can write changes to,
 * and which pipe to listen on.
 */
CommQueue::CommQueue(Throttle *parent, const char *pipe_fn) : Throt(parent), comm_pipe(pipe_fn)
{
	comm_pipe = pipe_fn;

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

	// watch for input
	char buf[LINE_LENGTH];
	do {
		pipe.getline(buf, LINE_LENGTH);
		that->processCommand(std::string(buf));
	} while (buf[0] != '*');

	that->Throt->term = true;

	return 0;
}

/*
 * Process a command coming through the pipe.
 */
void CommQueue::processCommand(const std::string comm)
{
#ifdef DEBUG
	std::cout << "[CommQueue] " << comm << std::endl;
#endif

	// parse command
	// change appropriate variables
}
