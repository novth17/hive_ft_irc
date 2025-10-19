#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include "irc.hpp"
#include "server.hpp"
#include "utility.hpp"

bool Server::setNonBlocking(int fd) {
	int flg = fcntl(fd, F_GETFL, 0);
	if (flg < 0)
		return false;
	if (fcntl(fd, F_SETFL, flg | O_NONBLOCK) == -1)
		return false;
	return true;
}

void Server::closeAndClean(const std::string& msg, int sockfd, struct addrinfo* result) {
	if (sockfd != -1)
		close(sockfd);
	if (result)
		freeaddrinfo(result);
	throw std::runtime_error(msg);
}

/*
setup phase of a TCP server socket.
The socket returned by createListenSocket is the listening socket.
When a client connects, you call accept().
accept() returns a new socket (client_fd) for that connection.
You use this new socket to recv() and send().
The listening socket itself never sends/receives application data. It just waits for more connections.
*/
int Server::createListenSocket(const char* host, const char* port, bool isNonBlocking)
{
	int sockfd = -1;
	struct addrinfo hints = {}, *result;
	int opt = 1;

	hints.ai_family   = AF_INET;	   // IPv4 only
	hints.ai_socktype = SOCK_STREAM;   // TCP
	hints.ai_protocol = 0;			   // Any protocol

	int status = getaddrinfo(host, port, &hints, &result);
	if (status != 0)
		fail("getaddrinfo failed: ", gai_strerror(status));

	// Single IPv4 result
	//socketsetup()
	sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sockfd < 0) {
		freeaddrinfo(result);
		throw std::runtime_error("Failed to create socket");
	}
	//avoid bind to fail: address already in use for testing
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		closeAndClean("setsockopt failed", sockfd, result);
	}

	//socketbind()
	if (bind(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
		logError("bind failed: %s", strerror(errno));
		closeAndClean("Failed to bind socket", sockfd, result);
	}
	if (listen(sockfd, MAX_BACKLOG) < 0)
		closeAndClean("Failed to listen on socket", sockfd, result);

	if (isNonBlocking && !setNonBlocking(sockfd))
		closeAndClean("Failed to set non-blocking", sockfd, result);

	freeaddrinfo(result);
	return sockfd;
}
