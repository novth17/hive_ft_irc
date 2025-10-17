#include "irc.hpp"

/**
 * Handle any type of message. Removes the command from the parameter list, then
 * calls the handler for that command with the remaining parameters.
 */
void Server::handleMessage(Client& client, int argc, char** argv)
{
	// Ignore empty messages.
	if (argc == 0)
		return;

	// Array of message handlers.
	using Handler = void (Server::*)(Client&, int, char**);
	static const std::pair<const char*, Handler> handlers[] = {
		{"USER", &Server::handleUser},
		{"NICK", &Server::handleNick},
		{"PASS", &Server::handlePass},
		{"CAP",  &Server::handleCap},
		{"JOIN", &Server::handleJoin},
	};

	// Send the message to the handler for that command.
	for (const auto& [command, handler]: handlers) {
		if (matchIgnoreCase(command, argv[0])) {
			return (this->*handler)(client, argc - 1, argv + 1);
		}
	}

	// Log any unimplemented commands, so that they can be added eventually.
	logError("Unimplemented command '%s'", argv[0]);
}

/**
 * Handle a USER message.
 */
void Server::handleUser(Client& client, int argc, char** argv)
{
	(void) client, (void) argc, (void) argv;
	logWarn("Unimplemented command: USER");
	sendReply(client, "Hello %d", 42);
}

/**
 * Handle a NICK message.
 */
void Server::handleNick(Client& client, int argc, char** argv)
{
	(void) client, (void) argc, (void) argv;
	logWarn("Unimplemented command: NICK");
}

/**
 * Handle a PASS message.
 */
void Server::handlePass(Client& client, int argc, char** argv)
{
	(void) client, (void) argc, (void) argv;
	logWarn("Unimplemented command: PASS");
}

/**
 * Handle a CAP message.
 */
void Server::handleCap(Client& client, int argc, char** argv)
{
	(void) client, (void) argc, (void) argv;
	logWarn("Unimplemented command: CAP");
}

/**
 * Handle a JOIN message.
 */
void Server::handleJoin(Client& client, int argc, char** argv)
{
	(void) client, (void) argc, (void) argv;
	logWarn("Unimplemented command: JOIN");
}
