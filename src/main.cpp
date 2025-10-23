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

	// Check that the first argument is a valid port number.
	int port;
	if (!parseInt(argv[1], port) || port < 0 || port > PORT_MAX) {
		log::error("invalid port number: ", argv[1]);
		return EXIT_FAILURE;
	}

	// Start the server.
	try {
		Server server(argv[1], argv[2]);
		server.eventLoop(NULL, argv[1]);
	} catch (std::exception& error) {
		log::error("Unhandled exception: ", error.what());
	}
}
