#include "Session.h"
#include "Log.h"
#include "Settings.h"
#include "Users.h"
#include "Colors.h"
#include "World.h"
#include "Contracts.h"

#include <istream>

#define SendStream(S) {stringstream ss; ss << S; Send(ss.str());}

Session::Session( boost::asio::io_service& service )
	: _socket(service)
	, _loginStage(0)
	, _user(0)
	, _closing(false)
	, _sending(false)
	, _recving(false)
	, _shouldQuit(false)
{}

Session::~Session()
{
	Log("Session destroyed");
	World::Instance()->RemoveSession(this);
}

tcp::socket& Session::Socket()
{
	return _socket;
}

User* Session::GetUser()
{
	return _user;
}

void Session::Start()
{
	Log("Starting session");
	SendWelcome();
	ChainRecv();
}

void Session::Stop()
{
	_socket.close();
	_closing = true;

	if(!_sending && !_recving)
	{
		delete this;
	}
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
	if(!_closing)
		SendStream(GREENCOLOR << "> " << CLEARCOLOR);
}

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
					World::Instance()->AddSession(this);
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
				World::Instance()->AddSession(this);
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

string ExtractCommand(const string& message, string& remainder)
{
	size_t idx = message.find(' ');
	string command;
	if(string::npos == idx)
	{
		command = message;
		remainder = "";
	}
	else
	{
		command = message.substr(0, idx);
		idx = message.find_first_not_of(' ', idx);
		if(string::npos == idx)
			remainder = "";
		else
			remainder = message.substr(idx);
	}

	for(idx = 0; idx < command.length(); ++idx)
	{
		if(isupper(command[idx]))
			command[idx] = tolower(command[idx]);
	}

	return command;
}

void Session::DoWho()
{
	vector<User*> users;
	World::Instance()->GetUsers(users);

	stringstream ss;
	ss << "--[" << GREENCOLOR << "Users Online" << CLEARCOLOR << "]------\r\n";
	for(vector<User*>::const_iterator user = users.begin(); user != users.end(); ++user)
	{
		User* up = *user;
		if(up->Admin)
			ss << "[Admin] ";
		ss << up->Username << " - $" << up->Money << " R" << up->Respect << "\r\n";
	}
	Send(ss.str());
}

void Session::DoHelp(const string& message)
{
	SendStream("--[" << GREENCOLOR << "Help Topics" << CLEARCOLOR << "]------\r\n");
	SendStream("game\r\nchat\r\ncontracts\r\ncorps\r\n");
}

void Session::DoLeaders()
{
	const vector<User*>& users = Users::Instance()->GetLeaders();

	int pos = 1;

	stringstream ss;
	ss << "--[" << GREENCOLOR << "Leaders" << CLEARCOLOR << "]------\r\n";
	for(vector<User*>::const_iterator up = users.begin(); up != users.end(); ++up)
	{
		ss << pos << " - " << (*up)->Username << " $" << (*up)->Money << "\r\n";
		++pos;
	}

	Send(ss.str());
}

void Session::DoAbout( const string& message )
{
	string remainder;
	string username = ExtractCommand(message, remainder);

	if(0 == username.length())
	{
		Send("Syntax: about <user>\r\n");
		return;
	}

	User* user = Users::Instance()->GetUserByUsername(username);
	if(0 == user)
	{
		SendStream("No such user '" << username << "'\r\n");
		return;
	}

	SendStream("--[" << GREENCOLOR << "About " << username << CLEARCOLOR << "]------\r\n");
	SendStream(user->About << "\r\n");
	SendStream("Money: $" << user->Money << "\r\n");
	SendStream("Respect: R" << user->Respect << "\r\n");
}

void Session::DoTell( const string& message )
{
	string remainder;
	string username = ExtractCommand(message, remainder);

	if(0 == username.length() || 0 == remainder.length())
	{
		Send("Syntax: tell <user> <message>\r\n");
		return;
	}

	Session* session = World::Instance()->GetSession(username);
	if(0 == session)
	{
		SendStream("No user named '" << username << "' is online\r\n");
		return;
	}

	stringstream ss;
	ss << _user->Username << " whispers: " << remainder << "\r\n";
	session->Send(ss.str());
}

void Session::DoSay( const string& message )
{
	stringstream ss;
	ss << "<" << _user->Username << "> " << message << "\r\n";
	World::Instance()->Broadcast(ss.str());
}

void Session::DoSetAbout( const string& message )
{
	_user->About = message;
	SendStream("About set to:\r\n" << message << "\r\n");
}

void Session::DoQuit()
{
	SendImmediate("Bye\r\n");
	_shouldQuit = true;
}

void Session::DoSave()
{
	Users::Instance()->Save();
	Contracts::Instance()->Save();
	SendStream(REDCOLOR << "Saved\r\n" << CLEARCOLOR);
}

void Session::DoOffer( const string& message )
{
	string remainder;
	string myAmountStr = ExtractCommand(message, remainder);
	string target = ExtractCommand(remainder, remainder);
	string theirAmountStr = ExtractCommand(remainder, remainder);
	string timeStr = ExtractCommand(remainder, remainder);

	if(0 == myAmountStr.length() || 0 == target.length() || 0 == theirAmountStr.length() || 0 == timeStr.length())
	{
		Send("Syntax: offer <my amount> <target user> <their amount> <minutes>\r\n");
		return;
	}

	int myAmount = atoi(myAmountStr.c_str());
	int theirAmount = atoi(theirAmountStr.c_str());
	int time = atoi(timeStr.c_str());

	if(0 >= myAmount || 0 >= theirAmount || 0 >= time)
	{
		Send("Syntax: offer <my amount> <target user> <their amount> <minutes>\r\n");
		return;
	}

	time = min(Settings::maxDuration, time);

	Contracts::Instance()->CreateContract(this, myAmount, target, theirAmount, time);
}

void Session::DoOffers()
{
	vector<Contract*> offers;
	Contracts::Instance()->GetOffers(_user, offers);

	stringstream ss;
	ss << "--[" << GREENCOLOR << "Offers" << CLEARCOLOR << "]------\r\n";

	if(0 == offers.size())
	{
		ss << "No pending offers.\r\n";
	}

	for(vector<Contract*>::iterator offer = offers.begin(); offer != offers.end(); ++offer)
	{
		Contract* ptr = *offer;
		ss << ptr->User1 << " (" << ptr->User1Contribution << ") <-> (" << ptr->User2Contribution << ") " << ptr->User2 << " -- " << ptr->Duration << " minutes\r\n";
	}

	Send(ss.str());
}

void Session::DoContracts()
{
	vector<Contract*> contracts;
	Contracts::Instance()->GetContracts(_user, contracts);

	stringstream ss;
	ss << "--[" << GREENCOLOR << "Contracts " << contracts.size() << "/" << Settings::maxContracts << CLEARCOLOR << "]------\r\n";

	if(0 == contracts.size())
	{
		ss << "No existing contracts.\r\n";
	}

	for(vector<Contract*>::iterator contract = contracts.begin(); contract != contracts.end(); ++contract)
	{
		Contract* ptr = *contract;
		ss << ptr->User1 << " (" << ptr->User1Contribution << ") <-> (" << ptr->User2Contribution << ") " << ptr->User2 << " -- " << ptr->Duration << " minutes\r\n";
	}

	Send(ss.str());
}

void Session::DoAccept(const string& message)
{
	string remainder;
	string target = ExtractCommand(message, remainder);

	if(0 == target.length())
	{
		Send("Syntax: accept <user>\r\n");
		return;
	}

	Contracts::Instance()->AcceptOffer(this, target);
}

void Session::DoReject(const string& message)
{
	string remainder;
	string target = ExtractCommand(message, remainder);

	if(0 == target.length())
	{
		Send("Syntax: reject <user>\r\n");
		return;
	}

	Contracts::Instance()->RejectOffer(this, target);
}

void Session::CommandMessage( const string& message )
{
	string remainder;
	string command = ExtractCommand(message, remainder);

	if(command.length() == 0)
	{
		Send("Empty command received\r\n");
		return;
	}

	switch(command[0])
	{
	case 'a':
		if("about" == command)
		{
			DoAbout(remainder);
			return;
		}
		else if("accept" == command)
		{
			DoAccept(remainder);
			return;
		}
		break;
	case 'b':
		break;
	case 'c':
		if("contracts" == command)
		{
			DoContracts();
			return;
		}
		break;
	case 'd':
		break;
	case 'e':
		break;
	case 'f':
		break;
	case 'g':
		break;
	case 'h':
		if("help" == command)
		{
			DoHelp(remainder);
			return;
		}
		break;
	case 'i':
		// Ignore?
		break;
	case 'j':
		break;
	case 'k':
		break;
	case 'l':
		if("leaders" == command)
		{
			DoLeaders();
			return;
		}
		break;
	case 'm':
		break;
	case 'n':
		break;
	case 'o':
		if("offer" == command)
		{
			DoOffer(remainder);
			return;
		}
		else if("offers" == command)
		{
			DoOffers();
			return;
		}
		break;
	case 'p':
		break;
	case 'q':
		if("quit" == command)
		{
			DoQuit();
			return;
		}
		break;
	case 'r':
		// Rate user +/-
		if("reject" == command)
		{
			DoReject(remainder);
			return;
		}
		break;
	case 's':
		if("say" == command)
		{
			DoSay(remainder);
			return;
		}
		else if("save" == command && _user->Admin)
		{
			DoSave();
			return;
		}
		else if("setabout" == command)
		{
			DoSetAbout(remainder);
			return;
		}
		// Stats
		break;
	case 't':
		if("tell" == command)
		{
			DoTell(remainder);
			return;
		}
		break;
	case 'u':
		break;
	case 'v':
		break;
	case 'w':
		if("who" == command)
		{
			DoWho();
			return;
		}
		break;
	case 'x':
		break;
	case 'y':
		break;
	case 'z':
		break;
	}

	SendStream("Unknown command '" << command << "'\r\n");
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

void Session::SendImmediate(const string& message)
{
	boost::asio::write(_socket, boost::asio::buffer(message.data(), message.length()));
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

string StripInput(const char* input)
{
	stringstream ss;
	bool leading = true;

	while(0 != *input)
	{
		if(*input & (0x1 << 7))
		{
			++input;
			if(0 == *input)
				break;
			++input;
			if(0 == *input)
				break;
			++input;

			continue;
		}

		if(!(leading && ' ' == *input) && isprint(*input))
		{
			leading = false;
			ss << *input;
		}
		++input;
	}

	return ss.str();
}

void Session::HandleRecv( const boost::system::error_code& error, size_t nRecvd )
{
	_recving = false;

	if(!error)
	{
		string line;
		istream is(&_buffer);
		getline(is, line);
		line = StripInput(line.c_str());

		if(line.length() > 1024)
		{
			LogWarning("Over long message received, killing session");
			Stop();
			return;
		}
		else if(line.length() > 0)
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

		if(_shouldQuit)
			Stop();
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
