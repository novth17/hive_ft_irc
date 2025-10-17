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

bool isValidChannelName(const char* name)
{
	if (*name == '\0' || (*name != '#' && *name != '&'))
		return false;
	for (; *name != '\0'; name++)
		if (*name == ' ' || *name == ',' || *name == '\a')
			return false;
	return true;
}

/**
 * Handle a JOIN message.
 */
void Server::handleJoin(Client& client, int argc, char** argv)
{
	// Check that we have enough parameters.
	const char* nick = client.nick.c_str();
	if (argc < 1 || argc > 2)
		return sendReply(client, "461 %s JOIN :Not enough parameters", nick);

	// Join a list of channels.
	if (argc == 1 || argc == 2) {
		char noKeys = '\0';
		char* name = argv[0];
		char* key = argc >= 2 ? argv[1] : &noKeys;
		while (*name != '\0') {

			// Get the next channel name and key in the comma-separated list.
			int name_length = strcspn(name, ",");
			int key_length = strcspn(key, ",");
			if (name[name_length] == ',')
				name[name_length++] = '\0';
			if (key[key_length] == ',')
				key[key_length++] = '\0';

			// Check that a valid channel name was given.
			if (!isValidChannelName(name)) {
				sendReply(client, "403 %s %s :No such channel", nick, name);

			// If there's no channel by that name, create it.
			} else {
				Channel* channel = findChannelByName(name);
				if (channel == nullptr) {
					logInfo("Creating new channel '%s'", name);
					channel = &_channels[name]; // Makes a new empty channel.
					channel->name = name;
				}

				// Only continue if the client is not already in the channel.
				if (channel->members.find(nick) == channel->members.end()) {

					// Stop if the key didn't match.
					if (channel->key != key) {
						sendReply(client, "%s %s :Cannot join channel (+k)", nick, name);

					// Otherwise, join the channel.
					} else {
						const char* topic = channel->topic.c_str();
						logInfo("%s joined channel %s", nick, name);
						channel->members[nick] = &client;
						sendReply(client, ":%s JOIN %s", nick, name);
						sendReply(client, "332 %s %s :%s", nick, name, topic);
						// TODO: Reply with list of channel users.
					}
				}
			}

			name += name_length;
			key += key_length;
		}
	}
}
