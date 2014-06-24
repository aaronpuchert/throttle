#include "throttle.hpp"

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
	while (conf_file) {
		// parse: first comes the name of the command
		conf_file >> name;

		// if line starts with '#', ignore it
		if (name[0] == '#') {
			conf_file.ignore(LINE_LENGTH, '\n');
			continue;
		}

		conf_file.ignore(LINE_LENGTH, '=');
		conf_file.ignore(LINE_LENGTH, ' ');
		conf_file.getline(line, LINE_LENGTH);

		// write into map
		attributes[name] = std::string(line);
	}
}
