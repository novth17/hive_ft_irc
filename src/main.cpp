#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <string>

#include "bot.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utility.hpp"

int main(int argc, char** argv)
{
	// Check that two arguments were given.
	if (argc != 3 && argc != 4) {
		printf("usage: ./ircserv <port> <password> [botname]\n");
		return EXIT_FAILURE;
	}
	char* port = argv[1];
	char* password = argv[2];

	// Check that the first argument is a valid port number.
	int portInt;
	if (!parseInt(argv[1], portInt) || portInt < 0 || portInt > PORT_MAX) {
		log::error("invalid port number: '", port, "'");
		return EXIT_FAILURE;
	}

	// Check that the password only contains printable characters.
	for (char c: std::string_view(password)) {
		if (!std::isprint(c)) {
			log::error("password contains non-printable characters");
			return EXIT_FAILURE;
		}
	}

	// Check if the bot option was passed.
	char* botName = nullptr;
	if (argc == 4) {
		botName = argv[3];
		if (!Client::isValidName(botName)) {
			log::error("invalid bot name: '", botName, "'");
			return EXIT_FAILURE;
		}
	}

	// Start the server.
	try {
		// Start a normal server.
		if (botName == nullptr) {
			Server server(port, password);
			server.eventLoop(port);

		// Start the bot.
		} else {
			Bot bot(botName);
			bot.run(port, password);
		}
	} catch (std::exception& error) {
		log::error("Unhandled exception: ", error.what());
	}
}
