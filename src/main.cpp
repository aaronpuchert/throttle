#include "throttle.hpp"
#include <iostream>
#include <signal.h>

// Pointer to throttle instance for signal handler.
Throttle *throt = NULL;

/**
 * Handle termination signals.
 */
void sighandle(int sig)
{
	if (throt)
		throt->term = true;
}

/**
 * Main function.
 */
int main(int argc, char **argv)
{
	const char *config_fn, *pipe_fn;

	// Read arguments from command line, or take the defaults
	config_fn = (argc > 1) ? argv[1] : "throttle.conf";
	pipe_fn = (argc > 2) ? argv[2] : "pipe";

	// Install signal handler
	struct sigaction sa;
	sa.sa_handler = sighandle;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL) < 0 ||
		sigaction(SIGQUIT, &sa, NULL) < 0 ||
		sigaction(SIGTERM, &sa, NULL) < 0) {
		std::cerr << "Error: Couldn't register signal handlers" << std::endl;
		return -1;
	}

	try {
		// create Throttle object
		Throttle throttle(config_fn, pipe_fn);

		// Put in global variable for signal handler
		throt = &throttle;

		// run
		throttle.run();

		// Reset global variable
		throt = NULL;
	}
	catch (std::runtime_error ex) {
		std::cerr << "Error: " <<  ex.what() << std::endl;
		return 1;
	}

	DEBUG_PRINT("[Main] Successful shutdown");

	return 0;
}
