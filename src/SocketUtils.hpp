#pragma once

namespace SocketUtils {
	bool	setNonBlocking(int fd);
	void	closeAndClean(const std::string& msg, int sockfd, struct addrinfo* result);
	int		createListenSocket(const char* host, const char* port, bool isNonBlocking);
};
