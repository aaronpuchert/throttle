#include "throttle.hpp"
#include <iostream>

/*
 * Main function.
 */
int main(int argc, char **argv)
{
	const char *config_fn, *pipe_fn;

	// read arguments from command line
	if (argc > 1)
		config_fn = argv[1];
	else
		config_fn = "throttle.conf";
	if (argc > 2)
		pipe_fn = argv[2];
	else
		pipe_fn = "pipe";

	try {
		// create Throttle object
		Throttle throttle(config_fn, pipe_fn);

		// run
		throttle.run();
	}
	catch (std::runtime_error ex) {
		std::cerr << "Error: " <<  ex.what() << std::endl;
		return 1;
	}

	return 0;
}
