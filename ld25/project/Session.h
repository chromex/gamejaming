#ifndef SESSION_H
#define SESSION_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include <queue>
#include <string>
using namespace std;

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

	void Send(const string& message);
	void SendImmediate(const string& message);

private:
	// Commands
	void DoAbout(const string& message);
	void DoWho();
	void DoSay(const string& message);
	void DoQuit();
	void DoSave();
	void DoSetAbout(const string& message);

	// Auth/create
	void SendWelcome();
	void SendPrompt();
	void LoginMessage(const string& message);
	void CommandMessage(const string& message);

	// Network
	void HandleSend(const boost::system::error_code& error);
	void HandleRecv(const boost::system::error_code& error, size_t nRecvd);
	void ChainSend();
	void ChainRecv();

	// Types
	typedef queue<string> MessageQueue;

	// Members
	tcp::socket _socket;
	MessageQueue _sendQueue;
	int _loginStage;
	boost::asio::streambuf _buffer;

	string _authUsername;
	string _authPassword;

	User* _user;

	// Dead session handling
	bool _closing;
	bool _sending;
	bool _recving;
	bool _shouldQuit;
};

#endif