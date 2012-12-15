#include "Session.h"
#include "Log.h"
#include "Settings.h"
#include "Users.h"
#include "Colors.h"

#include <istream>

#define SendStream(S) {stringstream ss; ss << S; Send(ss.str());}

Session::Session( boost::asio::io_service& service )
	: _socket(service)
	, _loginStage(0)
	, _remainingHeader(21)
	, _closing(false)
	, _sending(false)
	, _recving(false)
{}

Session::~Session()
{
	Log("Session destroyed");
}

tcp::socket& Session::Socket()
{
	return _socket;
}

void Session::Start()
{
	Log("Starting session");
	SendWelcome();
	ChainRecv();
}

void Session::SendWelcome()
{
	SendStream(REDCOLOR << "Connected to EndNet, a LD25 product\r\n" << CLEARCOLOR);
	Send("What is the username you have or would like to have?\r\n");
	SendPrompt();
	_loginStage = 1;
}

void Session::SendPrompt()
{
	SendStream(GREENCOLOR << "> " << CLEARCOLOR);
}

// TODO
// Vet password
// Vet handle
// Strip all illegal characters from input (alnum and punct)
// Strip telnet command codes
// Colorizing of sent data

void Session::LoginMessage( const string& message )
{
	switch(_loginStage)
	{
	case 0:
		LogWarning("Received message before auth began, ignoring");
		return;
	case 1:
		{
			User* user = Users::Instance()->GetUserByUsername(message);
			if(0 == user)
			{
				SendStream("Creating new user '" << message << "'\r\n");
				Send("What is your password?\r\n");
				++_loginStage;
			}
			else
			{
				Log("Old user");
				_loginStage = -1;
			}
		}
		break;
	case 2:
		{
			Send("Again!\r\n");
			++_loginStage;
		}
		break;
	case 3:
		{
			Send("User created!\r\n");
			_loginStage = -1;
		}
		break;
	}
}

void Session::CommandMessage( const string& message )
{
	Log("Command message: " << message);
}

// NETWORK
//

void Session::Send( const string& message )
{
	_sendQueue.push(message);

	if(1 == _sendQueue.size())
	{
		ChainSend();
	}
}

void Session::HandleSend( const boost::system::error_code& error )
{
	_sending = false;

	if(!error)
	{
		_sendQueue.pop();
		if(!_sendQueue.empty())
		{
			ChainSend();
		}
	}
	else
	{
		_closing = true;

		if(!_sending && !_recving)
		{
			delete this;
		}
	}
}

void Session::HandleRecv( const boost::system::error_code& error, size_t nRecvd )
{
	_recving = false;

	if(!error)
	{
		// Deal with the telnet header, really should be watching for command messages
		if(_remainingHeader > 0)
		{
			if(nRecvd < _remainingHeader)
			{
				_buffer.consume(nRecvd);
				_remainingHeader -= nRecvd;
				ChainRecv();
				return;
			}
			else
			{
				_buffer.consume(_remainingHeader);
				nRecvd -= _remainingHeader;
				_remainingHeader = 0;
			}
		}

		string line;
		istream is(&_buffer);
		getline(is, line);

		if(line.length() > 50)
		{
			LogWarning("Over long message received, killing session");
			_socket.close();
			_closing = true;

			if(!_sending && !_recving)
			{
				delete this;
			}

			return;
		}
		else if(line.length() > 1)
		{
			if(_loginStage > 0)
			{
				LoginMessage(line.substr(0,line.length()-1));
			}
			else
			{
				CommandMessage(line.substr(0,line.length()-1));
			}

			SendPrompt();
		}

		ChainRecv();
	}
	else
	{
		_closing = true;

		if(!_sending && !_recving)
		{
			delete this;
		}
	}
}

void Session::ChainSend()
{
	if(!_closing)
	{
		_sending = true;
		boost::asio::async_write(_socket,
			boost::asio::buffer(_sendQueue.front().data(), _sendQueue.front().length()),
			boost::bind(&Session::HandleSend, this, boost::asio::placeholders::error));
	}
}

void Session::ChainRecv()
{
	if(!_closing)
	{
		_recving = true;
		boost::asio::async_read_until(_socket,
			_buffer,
			"\r\n",
			boost::bind(&Session::HandleRecv, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
}