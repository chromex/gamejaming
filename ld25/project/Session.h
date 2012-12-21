#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include <queue>
#include <string>

#include "Users.h"

class Session
{
public:
	Session(boost::asio::io_service& service);
	~Session();

	void Start();
	void Stop();

	tcp::socket& Socket();
	User* GetUser();

	void Send(const std::string& message);
	void SendImmediate(const std::string& message);

private:
	// Commands
	void DoAbout(const std::string& message);
	void DoHelp(const std::string& message);
	void DoWho();
	void DoSay(const std::string& message);
	void DoQuit();
	void DoSave();
	void DoSetAbout(const std::string& message);
	void DoTell(const std::string& message);
	void DoLeaders();
	void DoOffer(const std::string& message);
	void DoEvilOffer(const std::string& message);
	void DoOffers();
	void DoContracts();
	void DoResults();
	void DoAccept(const std::string& message);
	void DoEvilAccept(const std::string& message);
	void DoReject(const std::string& message);
	void DoRate(const std::string& message);

	// Auth/create
	void SendWelcome();
	void SendPrompt();
	void LoginMessage(const std::string& message);
	void CommandMessage(const std::string& message);

	// Network
	void HandleSend(const boost::system::error_code& error);
	void HandleRecv(const boost::system::error_code& error, size_t nRecvd);
	void ChainSend();
	void ChainRecv();

	// Types
	typedef std::queue<std::string> MessageQueue;

	// Members
	tcp::socket _socket;
	MessageQueue _sendQueue;
	int _loginStage;
	boost::asio::streambuf _buffer;

	std::string _authUsername;
	std::string _authPassword;

	User* _user;
	User* _authUser;

	// Dead session handling
	bool _closing;
	bool _sending;
	bool _recving;
	bool _shouldQuit;
};

#endif