#pragma once

#include <climits>
#include <set>
#include <string>
#include <string_view>

#include "server.hpp"

class Channel;

class Client
{
public:
	int socket = -1;				// The socket used for the client's connection
	std::string nick;				// The client's nickname
	std::string user;				// The client's user name
	std::string realname;			// The client's real name
	std::string host;				// The client's host IP address
	std::string fullname;			// The full nick!user@host name

	std::string input;				// Buffered data from recv()
	std::string output;				// Buffered data for send()
	bool isRegistered = false;		// Whether the client completed registration
	bool isPassValid = false;		// Whether the client gave the correct password
	bool isDisconnected = false;	// Set to true when the client is disconnected
	Server* server = nullptr;		// Pointer to the server object
	std::set<Channel*>	channels;	// All channels the client is joined to

	void setChannelMode(Channel& channel, char* modes, char* args);

	void handleUser(int argc, char** argv);
	void handleNick(int argc, char** argv);
	void handlePass(int argc, char** argv);
	void handlePart(int argc, char** argv);
	void handleJoin(int argc, char** argv);
	void handlePing(int argc, char** argv);
	void handleQuit(int argc, char** argv);
	void handleMode(int argc, char** argv);
	void handleWho(int argc, char** argv);
	bool handlePrivMsgParams(int argc, char** argv);
	void handlePrivMsg(int argc, char** argv);
	void handleKick(int argc, char** argv);
	void handleTopic(int argc, char** argv);
	void handleInvite(int argc, char** argv);
	void handleNames(int argc, char** argv);
	void handleList(int argc, char** argv);
	void handleLusers(int argc, char** argv);
	void handleMotd(int argc, char** argv);

	// Send a numeric reply.
	template <typename... Arguments>
	void sendNumeric(const char* num, const Arguments&... args)
	{
		const char* name = nick.empty() ? "*" : nick.c_str();
		sendLine(":", server->getHostname(), " ", num, " ", name, " ", args...);
	}

	// Send a string to the client.
	void send(const std::string_view& string);

	// Send a single value of numeric type (using std::to_string).
	template <typename Type>
	void send(const Type& value)
	{
		send(std::to_string(value));
	}

	// Send multiple values by recursively calling other send() functions.
	template <typename First, typename... Rest>
	void send(const First& first, const Rest&... rest)
	{
		send(first);
		send(rest...);
	}

	// Send multiple values and add a CRLF line break at the end.
	template <typename... Arguments>
	void sendLine(const Arguments&... arguments)
	{
		send(arguments...); // Send all the arguments.
		send("\r\n"); // Add a newline at the end.
	}

	// Types that must be converted to string_view to be sent.
	void send(char character) { send(std::string_view(&character, 1)); }
	void send(char* string) { send(std::string_view(string)); }
	void send(const char* string = "") { send(std::string_view(string)); }
	void send(const std::string& string) { send(std::string_view(string)); }

	static bool isValidName(std::string_view name);
	void handleRegistrationComplete();
	bool commonChecks(const char* cmd, bool reg, int argc, int min, int max = INT_MAX);
};
