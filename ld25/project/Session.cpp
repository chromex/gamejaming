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
	, _user(0)
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
			_user = Users::Instance()->GetUserByUsername(message);
			if(0 == _user)
			{
				if(message.length() < 33)
				{
					SendStream("Creating new user '" << message << "'\r\n");
					Send("What would you like for your password?\r\nKnow that telnet is not secure so don't use your normal password!\r\n");
					++_loginStage;
					_authUsername = message;
				}
				else
				{
					Send("Username is too long, 32 character max. Try again!\r\n");
				}
			}
			else
			{
				Send("And your password?\r\n");
				_loginStage = 10;
				_authUsername = message;
			}
		}
		break;
	case 2:
		{
			if(message.length() < 129)
			{
				Send("Again!\r\n");
				++_loginStage;
				_authPassword = message;
			}
			else
			{
				Send("Password was too long, try again!\r\n");
			}
		}
		break;
	case 3:
		{
			if(_authPassword == message)
			{
				_user = Users::Instance()->CreateUser(_authUsername, _authPassword);

				if(0 != _user)
				{
					SendStream("Created user '" << _authUsername << "' with password '" << _authPassword << "'\r\n");
					_loginStage = -1;
				}
				else
				{
					Send("It looks like your username was taken while you were signing up.\r\nWhat username would you like?\r\n");
					_loginStage = 1;
				}
			}
			else
			{
				Send("Passwords don't match.\r\nWhat would you like for your password?\r\n");
				--_loginStage;
			}
		}
		break;
	case 10:
		{
			if(_user->Password == message)
			{
				SendStream(GREENCOLOR << "Authentication successful\r\n" << CLEARCOLOR);
				if(_user->Admin)
				{
					SendStream("Welcome back, administrator.\r\n");
				}
				_loginStage = -1;
				Log("User '" << _user->Username << "' returns");
			}
			else
			{
				SendStream("Failed authentication!\r\nWhat is the username you have or would like to have?\r\n");
				_loginStage = 1;
			}
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

// TODO
// Should be attempting to strip the telnet command codes in here
string StripInput(const char* input)
{
	stringstream ss;

	while(0 != *input)
	{
		if(isprint(*input))
			ss << *input;
		++input;
	}

	return ss.str();
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
		line = StripInput(line.c_str());

		if(line.length() > 1024)
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
				LoginMessage(line);
			}
			else
			{
				CommandMessage(line);
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