#pragma once
#include "events.hpp"
#include <string>


class Connection {
private:
    int _fd;
    std::string _readBuffer;
    std::string _writeBuffer;
    bool _closed;

public:
	Connection() = default;
    Connection(int clientFd);
	~Connection();

    int get_fd() const;

    bool isClosed() const;
    void markClosed();

    bool hasDataToSend() const;
    std::string& getReadBuffer();
	const std::string& getWriteBuffer() const ;
	std::string& getWriteBuffer();

	// --- API ---
    void appendRead(const char* data, size_t len);

    void clearReadBuffer();

    void queueWrite(const std::string& data);

    // Called by server when socket is writable
    int flushWriteBuffer();
};
