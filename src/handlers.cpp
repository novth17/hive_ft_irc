#include "irc.hpp"

/**
 * Handle any type of message. Removes the command from the parameter list, then
 * calls the handler for that command with the remaining parameters.
 */
void Server::handleMessage(Client& client, std::string_view* params, int count)
{
	// Ignore empty messages.
	if (count == 0)
		return;

	// Array of message handlers.
	using Handler = void (Server::*)(Client&, std::string_view*, int);
	static const std::pair<std::string_view, Handler> handlers[] = {
		{"USER", &Server::handleUser},
		{"NICK", &Server::handleNick},
		{"PASS", &Server::handlePass},
		{"CAP",  &Server::handleCap},
		{"JOIN", &Server::handleJoin},
	};

	// Send the message to the handler for that command.
	std::string_view name = params[0];
	for (const auto& [command, handler]: handlers) {
		if (matchIgnoreCase(command, name)) {
			return (this->*handler)(client, params + 1, count - 1);
		}
	}

	// Log any unimplemented commands, so that they can be added eventually.
	logError("Unimplemented command '%.*s'", int(name.size()), name.data());
}

/**
 * Handle a USER message.
 */
void Server::handleUser(Client& client, std::string_view* params, int count)
{
	(void) client, (void) params, (void) count;
	logWarn("Unimplemented command: USER");
	sendReply(client, "Hello %d", 42);
}

/**
 * Handle a NICK message.
 */
void Server::handleNick(Client& client, std::string_view* params, int count)
{
	(void) client, (void) params, (void) count;
	logWarn("Unimplemented command: NICK");
}

/**
 * Handle a PASS message.
 */
void Server::handlePass(Client& client, std::string_view* params, int count)
{
	(void) client, (void) params, (void) count;
	logWarn("Unimplemented command: PASS");
}

/**
 * Handle a CAP message.
 */
void Server::handleCap(Client& client, std::string_view* params, int count)
{
	(void) client, (void) params, (void) count;
	logWarn("Unimplemented command: CAP");
}

/**
 * Handle a JOIN message.
 */
void Server::handleJoin(Client& client, std::string_view* params, int count)
{
	(void) client, (void) params, (void) count;
	logWarn("Unimplemented command: JOIN");
}
