#include "throttle.hpp"
#include <iostream>
#include <signal.h>
#include <stdexcept>

// How many seconds shall we wait between updates?
static constexpr int wait = 3;

/**
 * Main function.
 */
int main(int argc, char **argv)
{
	const char *config_fn, *pipe_fn;

	// Read arguments from command line, or take the defaults
	config_fn = (argc > 1) ? argv[1] : "throttle.conf";
	pipe_fn = (argc > 2) ? argv[2] : "pipe";

	// Block SIGINT, SIGQUIT, SIGTERM - we're going to wait for them explicitly.
	sigset_t signals;
	sigemptyset(&signals);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGQUIT);
	sigaddset(&signals, SIGTERM);
	sigprocmask(SIG_BLOCK, &signals, nullptr);

	const struct timespec wait_time{wait, 0};

	try {
		// create Throttle object
		Throttle throttle = Throttle::createFromConfig(config_fn, pipe_fn);

		// Run the feedback loop
		do
			throttle();
		while (sigtimedwait(&signals, nullptr, &wait_time) == -1);
	}
	catch (const std::runtime_error& ex) {
		std::cerr << "Error: " <<  ex.what() << std::endl;
		return 1;
	}

	DEBUG_PRINT("[Main] Successful shutdown");

	return 0;
}
