#include "throttle.hpp"

#ifndef DEBUG
#define THROTTLE_CONF "throttle.conf"
#else
#define THROTTLE_CONF "throttle-debug.conf"
#endif

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
		config_fn = THROTTLE_CONF;
	if (argc > 2)
		pipe_fn = argv[2];
	else
		pipe_fn = "pipe";

	// create Throttle object
	Throttle throttle(config_fn, pipe_fn);

	// run
	throttle.run();
}