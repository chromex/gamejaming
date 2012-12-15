#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Session.h"

using boost::asio::ip::tcp;

class Server
{
public:
	Server(int port);
	~Server();

	void Run();

private:
	void ChainAccept();
	void Accept(Session* session, const boost::system::error_code& error);
	void SaveTick(const boost::system::error_code& error);
	void UpdateTick(const boost::system::error_code& error);

	boost::asio::io_service _service;
	tcp::acceptor _acceptor;
	boost::asio::deadline_timer _timer;
	boost::asio::deadline_timer _tickTimer;
};

#endif