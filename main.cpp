#include "throttle.hpp"

/*
 * Main function.
 */
int main(int argc, char **argv)
{
	// create Throttle object
	Throttle throttle("throttle.conf");

	// run
	throttle.run();
}
