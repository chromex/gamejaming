#include "Server.h"
#include "Log.h"
#include "Users.h"
#include "Contracts.h"
#include "Settings.h"

#include <boost/bind.hpp>

Server::Server( int port )
	: _acceptor(_service, tcp::endpoint(tcp::v4(), port))
	, _timer(_service, boost::posix_time::seconds(Settings::saveTicks))
	, _tickTimer(_service, boost::posix_time::seconds(Settings::updateTicks))
{
	Log("Listening on port: " << port);

	ChainAccept();

	_timer.async_wait(boost::bind(&Server::SaveTick, this, boost::asio::placeholders::error));
	_tickTimer.async_wait(boost::bind(&Server::UpdateTick, this, boost::asio::placeholders::error));
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

void Server::SaveTick( const boost::system::error_code& error )
{
	if(!error)
	{
		Users::Instance()->Save();
		Contracts::Instance()->Save();
		Log("Saved");

		_timer.expires_at(_timer.expires_at() + boost::posix_time::seconds(Settings::saveTicks));
		_timer.async_wait(boost::bind(&Server::SaveTick, this, boost::asio::placeholders::error));
	}
	else
	{
		LogError("Save tick encountered an error?!");
	}
}

void Server::UpdateTick( const boost::system::error_code& error )
{
	if(!error)
	{
		Users::Instance()->Tick();
		Contracts::Instance()->Tick();

		_tickTimer.expires_at(_tickTimer.expires_at() + boost::posix_time::seconds(Settings::updateTicks));
		_tickTimer.async_wait(boost::bind(&Server::UpdateTick, this, boost::asio::placeholders::error));
	}
	else
	{
		LogError("Update tick encountered an error?!");
	}
}
