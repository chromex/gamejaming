#ifndef SESSION_H
#define SESSION_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include <queue>
#include <string>
using namespace std;

class Session
{
public:
	Session(boost::asio::io_service& service);
	~Session();

	void Start();
	tcp::socket& Socket();

	void Send(const string& message);

private:
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
	size_t _remainingHeader;

	// Dead session handling
	bool _closing;
	bool _sending;
	bool _recving;
};

#endif