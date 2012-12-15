#include "Server.h"
#include "Log.h"

Server::Server( int port )
	: _acceptor(_service, tcp::endpoint(tcp::v4(), port))
{
	Log("Listening on port: " << port);

	ChainAccept();
}

Server::~Server()
{}

void Server::Run()
{
	_service.run();
}

void Server::ChainAccept()
{
	Session* session = new Session(_service);
	_acceptor.async_accept(session->Socket(), boost::bind(&Server::Accept, this, session, boost::asio::placeholders::error));
}

void Server::Accept( Session* session, const boost::system::error_code& error )
{
	if(!error)
	{
		session->Start();
	}
	else
	{
		LogError("Issue on accept: " << error);
		delete session;
	}

	ChainAccept();
}
