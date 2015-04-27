#include "throttle.hpp"
#include <iostream>
#include <signal.h>

// Pointer to throttle instance for signal handler.
Throttle *throt = NULL;

/*
 * Handle termination signals.
 */
void sighandle(int sig)
{
	if (throt)
		throt->term = true;
}

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

#ifdef DEBUG
	std::cout << "[Main] Successful shutdown" << std::endl;
#endif

	return 0;
}
