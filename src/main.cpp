#include "throttle.hpp"
#include <iostream>
#include <unistd.h>
#include <signal.h>

// How many seconds shall we wait between updates?
static const int wait = 3;

/**
 * Handle termination signals.
 */
bool term = false;
void sighandle(int sig)
{
	term = true;
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

		// Run the feedback loop
		while (!term) {
			throttle();
			sleep(wait);
		}
	}
	catch (std::runtime_error ex) {
		std::cerr << "Error: " <<  ex.what() << std::endl;
		return 1;
	}

	DEBUG_PRINT("[Main] Successful shutdown");

	return 0;
}
