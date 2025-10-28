#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <string>

#include "irc.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utility.hpp"

int main(int argc, char** argv)
{
	// Check that two arguments were given.
	if (argc != 3) {
		printf("usage: ./ircserv <port> <password>\n");
		return EXIT_FAILURE;
	}
	char* port = argv[1];
	char* password = argv[2];

	// Check that the first argument is a valid port number.
	int portInt;
	if (!parseInt(argv[1], portInt) || portInt < 0 || portInt > PORT_MAX) {
		log::error("invalid port number: ", port);
		return EXIT_FAILURE;
	}

	// Check that the password only contains printable characters.
	for (char c: std::string_view(password)) {
		if (!std::isprint(c)) {
			log::error("password contains non-printable characters");
			return EXIT_FAILURE;
		}
	}

	// Start the server.
	try {
		Server server(argv[1], argv[2]);
		server.eventLoop(argv[1]);
	} catch (std::exception& error) {
		log::error("Unhandled exception: ", error.what());
	}
}
