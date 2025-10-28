#pragma once

#include <set>
#include <string>
#include <string_view>

class Bot
{
public:
	explicit Bot(const char* name);
	~Bot();

	void run(const char* port, const char* password);

private:
	int connectToServer(const char* port);
	void parseMessage(std::string message);
	void handleMessage(int argc, char** argv);
	void receive();

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

	int socket = -1;
	int epoll = -1;
	bool disconnected = false;
	std::string name;
	std::string input;
	std::string output;
	std::set<std::string> channels;
};
