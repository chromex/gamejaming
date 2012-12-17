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
	, _authUser(0)
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
	Send("\r\n\
        :::        :::::::::    ::::::::   :::::::::: \r\n\
       :+:        :+:    :+:  :+:    :+:  :+:    :+:  \r\n\
      +:+        +:+    +:+        +:+   +:+          \r\n\
     +#+        +#+    +:+      +#+     +#++:++#+     \r\n\
    +#+        +#+    +#+    +#+              +#+     \r\n\
   #+#        #+#    #+#   #+#        #+#    #+#      \r\n\
  ########## #########   ##########   ########        \r\n\
	\r\n");

	SendStream(REDCOLOR << "Connected to EndNet, a LD25 entry\r\n" << CLEARCOLOR);
	Send("What is the username you have or would like to have?\r\n");
	SendPrompt();
	_loginStage = 1;
}

void Session::SendPrompt()
{
	if(!_closing)
	{
		if(0 != _user && _user->Started && !_user->Done)
		{
			boost::posix_time::time_duration td = boost::posix_time::second_clock::local_time() - _user->StartTime;
			int minutes = 60 - td.minutes();
			SendStream(REDCOLOR << minutes << "m remaining" << GREENCOLOR << "> " << CLEARCOLOR);
		}
		else if(0 != _user && _user->Done)
		{
			SendStream(REDCOLOR << "Time's up" << GREENCOLOR << "> " << CLEARCOLOR)
		}
		else
		{
			SendStream(GREENCOLOR << "> " << CLEARCOLOR);
		}
	}
}

bool IsAlnum(const string& message)
{
	for(size_t idx = 0; idx < message.length(); ++idx)
	{
		if(!isalnum(message[idx]))
			return false;
	}
	return true;
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
			_authUser = Users::Instance()->GetUserByUsername(message);
			if(0 == _authUser)
			{
				if(!IsAlnum(message))
				{
					Send("Username can only contain letters and numbers. Try again!\r\n");
				}
				else if(message.length() < 33)
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

					DoHelp("story");
					SendStream(REDCOLOR << "\r\nNew to the game? Try 'help game', 'help contracts', and 'help commands'\r\n" << CLEARCOLOR);
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
			if(_authUser->Password == message)
			{
				_user = _authUser;
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
				_authUser = 0;
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
		else if(up->Done)
			ss << "[Dead] ";
		ss << up->Username << " - $" << up->Money << " R" << up->Respect << " - " << up->About.substr(0,30) << "\r\n";
	}
	Send(ss.str());
}

void Session::DoHelp(const string& message)
{
	string remainder;
	string topic = ExtractCommand(message, remainder);

	if("commands" == topic)
	{
		stringstream ss;
		ss << "--[" << GREENCOLOR << "Help - Commands" << CLEARCOLOR << "]------\r\n";
		ss << "[" << REDCOLOR << "Social Commands" << CLEARCOLOR << "]\r\n";
		ss << GREENCOLOR << "about" << CLEARCOLOR << " <player> - Get information about the player\r\n";
		ss << GREENCOLOR << "setabout" << CLEARCOLOR << " <message> - Set the message for your 'about' page\r\n";
		ss << GREENCOLOR << "say" << CLEARCOLOR << " <message> - Send a message on global chat\r\n";
		ss << GREENCOLOR << "tell" << CLEARCOLOR << " <player> <message> - Send the online player a message\r\n";
		ss << GREENCOLOR << "leaders" << CLEARCOLOR << " - Show the richest players ever\r\n";
		ss << GREENCOLOR << "who" << CLEARCOLOR << " - Show online players\r\n\r\n";
		ss << "[" << REDCOLOR << "Contract List Commands" << CLEARCOLOR << "]\r\n";
		ss << GREENCOLOR << "offers" << CLEARCOLOR << " - Show pending offers that involve you\r\n";
		ss << GREENCOLOR << "contracts" << CLEARCOLOR << " - Show currently running contracts that involve you\r\n";
		ss << GREENCOLOR << "results" << CLEARCOLOR << " - Show contracts that have completed that involve you\r\n\r\n";
		ss << "[" << REDCOLOR << "Contract Commands" << CLEARCOLOR << "]\r\n";
		ss << GREENCOLOR << "accept" << CLEARCOLOR << " <player> - Accept the offer from the player\r\n";
		ss << GREENCOLOR << "evilaccept" << CLEARCOLOR << " <player> - Backstab on the offer from the player\r\n";
		ss << GREENCOLOR << "offer" << CLEARCOLOR << " <investment1> <player> <investment2> <duration>\r\n";
		ss << "  - Offer a contract, you invest investment1, they invest investment2\r\n";
		ss << GREENCOLOR << "eviloffer" << CLEARCOLOR << " <investment1> <player> <investment2> <duration>\r\n";
		ss << "  - Offer a backstab contract with the player\r\n";
		ss << GREENCOLOR << "reject" << CLEARCOLOR << " <player> - Reject the offer from the player\r\n";
		ss << GREENCOLOR << "rate" << CLEARCOLOR << " <id> <player> <rating> - Rate the player on the finished contract\r\n";

		Send(ss.str());
	}
	else if("game" == topic)
	{
		stringstream ss;

		ss << "--[" << GREENCOLOR << "Help - Game" << CLEARCOLOR << "]------\r\n";

		ss << "The point of the game is to make money on contracts with a prisoner's dilemma\r\n";
		ss << "twist. Either or both of the players involved in a contract can choose to\r\n";
		ss << "backstab the other with the \"evil\" versions of the offer and accept commands.\r\n";
		ss << "If neither uses the evil version, both players profit. If both are evil, both\r\n";
		ss << "loose everything invested as well as the contract slot for its duration. And\r\n";
		ss << "if only one is evil, then they take all of the money for themselves. Whether\r\n";
		ss << "or not you got screwed is not known until the contract is complete.\r\n\r\n";

		ss << "Since you only have one hour to make as much money as you can, picking who you\r\n";
		ss << "make deals with and if you screw them over is paramount. To facilitate this\r\n";
		ss << "players can chat globally (say) and privately (tell) and can see info about\r\n";
		ss << "players (about). \r\n\r\n";

		ss << "To aid in this, a reputation system is in place. Each player starts with 10\r\n";
		ss << "reputation. When a contract completes, both players involved can rate the\r\n";
		ss << "contract interaction in the range of -3 to +3. This is applied to the other\r\n";
		ss << "player and is reflected on their public reputation where higher is better.\r\n";

		Send(ss.str());
	}
	else if("story" == topic)
	{
		stringstream ss;

		ss << "--[" << GREENCOLOR << "Help - Story" << CLEARCOLOR << "]------\r\n";
		ss << "Money. Its all about the damn money. To make everything \"fair\" it was decided\r\n";
		ss << "in the year 2025 to make it so everyone had access to the trade markets. The\r\n";
		ss << "problem? You only get one shot. For one hour.\r\n\r\n";

		ss << "Everyone is given a preset amount of cash and once you start trading, you have\r\n";
		ss << "one hour to make as much money as you can. Whats going on behind the trades?\r\n";
		ss << "It doesn't matter any more. Its just a system.\r\n\r\n";

		ss << "Once the hour is up, you'll still be able to connect to the system but you can\r\n";
		ss << "no longer trade. No longer make more money. You failed? Whelp, your boned. \r\n\r\n";

		ss << "Don't fail.\r\n";

		Send(ss.str());
	}
	else if("contracts" == topic)
	{
		stringstream ss;

		ss << "--[" << GREENCOLOR << "Help - Contracts" << CLEARCOLOR << "]------\r\n";
		ss << "Contracts are how you make money. To create a contract, one player makes an\r\n";
		ss << "offer to another with the contract details. The other player can choose to\r\n";
		ss << "accept or reject the contract. Once accepted the contract lasts the specified\r\n";
		ss << "duration. Any one player can only have a maximum of 5 open contracts at a \r\n";
		ss << "time. The \"offer\" and \"accept\" have evil versions called \"eviloffer\" and \r\n";
		ss << "\"evilaccept\", the effects of which are described in \"help game\".\r\n\r\n";

		ss << "The parameters of the offer commands are:\r\n";
		ss << "  - offer <investment1> <player> <investment2> <duration>\r\n\r\n";

		ss << "investment1 - The investment of the offering player\r\n";
		ss << "player - The player you are making the offer to\r\n";
		ss << "investment2 - How much the other player is supposed to invest\r\n";
		ss << "duration - How long the contract lasts in minutes if accepted, max of 5\r\n\r\n";

		ss << "Profit multipliers increase with duration. At 1 minute, each player makes\r\n";
		ss << "back their investment plus the average of both investments and at 5 minutes\r\n";
		ss << "each makes back their investment plus 16x the average of their investments!\r\n";

		Send(ss.str());
	}
	else
	{
		SendStream("--[" << GREENCOLOR << "Help Topics" << CLEARCOLOR << "]------\r\n");
		SendStream("Syntax: help <topic>\r\n\r\n[" << GREENCOLOR << "Topics" << CLEARCOLOR << "]\r\n");
		Send("commands\r\ngame\r\nstory\r\ncontracts\r\n");
	}
}

void Session::DoLeaders()
{
	const vector<User*>& users = Users::Instance()->GetLeaders();

	int pos = 1;

	stringstream ss;
	ss << "--[" << GREENCOLOR << "Leaders" << CLEARCOLOR << "]------\r\n";
	for(vector<User*>::const_iterator up = users.begin(); up != users.end(); ++up)
	{
		if((*up)->Done)
			ss << "[Dead] ";
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
	if(user->Done)
		Send("The user is out of time\r\n");
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
	ss << _user->Username << " whispers: " << remainder.substr(0,256) << "\r\n";
	session->Send(ss.str());

	Log(_user->Username << " whispers to " << session->GetUser()->Username << ": " << remainder.substr(0,256));
}

void Session::DoSay( const string& message )
{
	if(message.length() > 0)
	{
		stringstream ss;
		ss << "<" << _user->Username << "> " << message.substr(0,256) << "\r\n";
		World::Instance()->Broadcast(ss.str());

		Log(_user->Username << " says: " << message.substr(0,256));
	}
	else
	{
		Send("Syntax: say <message>\r\n");
	}
}

void Session::DoSetAbout( const string& message )
{
	_user->About = message.substr(0,80);
	SendStream("About set to:\r\n" << _user->About << "\r\n");
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
	if(_user->Done)
	{
		Send("Sorry, your done and can no longer do that.\r\n");
		return;
	}

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

	Contracts::Instance()->CreateContract(this, myAmount, target, theirAmount, time, false);
}

void Session::DoEvilOffer( const string& message )
{
	if(_user->Done)
	{
		Send("Sorry, your done and can no longer do that.\r\n");
		return;
	}

	string remainder;
	string myAmountStr = ExtractCommand(message, remainder);
	string target = ExtractCommand(remainder, remainder);
	string theirAmountStr = ExtractCommand(remainder, remainder);
	string timeStr = ExtractCommand(remainder, remainder);

	if(0 == myAmountStr.length() || 0 == target.length() || 0 == theirAmountStr.length() || 0 == timeStr.length())
	{
		Send("Syntax: eviloffer <my amount> <target user> <their amount> <minutes>\r\n");
		return;
	}

	int myAmount = atoi(myAmountStr.c_str());
	int theirAmount = atoi(theirAmountStr.c_str());
	int time = atoi(timeStr.c_str());

	if(0 >= myAmount || 0 >= theirAmount || 0 >= time)
	{
		Send("Syntax: eviloffer <my amount> <target user> <their amount> <minutes>\r\n");
		return;
	}

	time = min(Settings::maxDuration, time);

	Contracts::Instance()->CreateContract(this, myAmount, target, theirAmount, time, true);
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
		ss << ptr->User1 << " (" << ptr->User1Contribution << ") <-> (" << ptr->User2Contribution << ") " << ptr->User2 << " -- " << ptr->Duration << " minute(s)\r\n";
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
		ss << ptr->User1 << " (" << ptr->User1Contribution << ") <-> (" << ptr->User2Contribution << ") " << ptr->User2 << " -- " << ptr->Duration << " minute(s)\r\n";
	}

	Send(ss.str());
}

void Session::DoResults()
{
	vector<Contract*> contracts;
	Contracts::Instance()->GetFinished(_user, contracts);

	stringstream ss;
	ss << "--[" << GREENCOLOR << "Completed " << contracts.size() << CLEARCOLOR << "]------\r\n";

	if(0 == contracts.size())
	{
		ss << "No completed contracts.\r\n";
	}

	int index = 1;
	for(vector<Contract*>::iterator contract = contracts.begin(); contract != contracts.end(); ++contract)
	{
		Contract* ptr = *contract;
		ss << "id " << index << " ] " << ptr->User1 << " (" << ptr->User1Profit << " - " << ptr->User1Contribution << ") <-> (" << ptr->User2Profit << " - " << ptr->User2Contribution << ") " << ptr->User2 << "\r\n";
		++index;
	}

	Send(ss.str());
}

void Session::DoRate(const string& message)
{
	if(_user->Done)
	{
		Send("Sorry, your done and can no longer do that.\r\n");
		return;
	}

	string remainder;
	string idStr = ExtractCommand(message, remainder);
	string user = ExtractCommand(remainder, remainder);
	string ratingStr = ExtractCommand(remainder, remainder);

	if(0 == idStr.length() || 0 == user.length() || 0 == ratingStr.length())
	{
		Send("Syntax: rate <id> <user> <rating>\r\n");
		return;
	}

	User* otherUser = Users::Instance()->GetUserByUsername(user);
	if(0 == otherUser)
	{
		SendStream("No such user '" << user << "' to rate.\r\n");
		return;
	}

	if(otherUser == _user)
	{
		Send("Can't rate yourself, jackass.\r\n");
		return;
	}

	int id = atoi(idStr.c_str()) - 1;
	int rating = atoi(ratingStr.c_str());

	if(rating < -3 || rating > 3)
	{
		Send("Ratings must fall in the range of -3 to 3, inclusive.\r\n");
		return;
	}

	vector<Contract*> contracts;
	Contracts::Instance()->GetFinished(_user, contracts);

	if(id < 0 || id >= (int)contracts.size())
	{
		Send("Bad contract ID. Grab the ID from the 'results' command.\r\n");
		return;
	}

	Contract* c = contracts[id];
	if(otherUser->Username != c->User1 && otherUser->Username != c->User2)
	{
		Send("Specified username is not on the contract, try again?\r\n");
		return;
	}

	if(((_user->Username == c->User1) && c->Rated1) || ((_user->Username == c->User2) && c->Rated2))
	{
		Send("I can't let you do that, you have already rated that contract interaction.\r\n");
		return;
	}

	otherUser->Respect += rating;

	if(_user->Username == c->User1)
		c->Rated1 = true;
	else if(_user->Username == c->User2)
		c->Rated2 = true;

	Log(_user->Username << " rated " << otherUser->Username << " with a " << rating << " rating");

	SendStream("You rated '" << otherUser->Username << "' with a " << rating << " rating on contract id " << id+1 << ".\r\n");
}

void Session::DoAccept(const string& message)
{
	if(_user->Done)
	{
		Send("Sorry, your done and can no longer do that.\r\n");
		return;
	}

	string remainder;
	string target = ExtractCommand(message, remainder);

	if(0 == target.length())
	{
		Send("Syntax: accept <user>\r\n");
		return;
	}

	Contracts::Instance()->AcceptOffer(this, target, false);
}

void Session::DoEvilAccept(const string& message)
{
	if(_user->Done)
	{
		Send("Sorry, your done and can no longer do that.\r\n");
		return;
	}

	string remainder;
	string target = ExtractCommand(message, remainder);

	if(0 == target.length())
	{
		Send("Syntax: evilaccept <user>\r\n");
		return;
	}

	Contracts::Instance()->AcceptOffer(this, target, true);
}

void Session::DoReject(const string& message)
{
	if(_user->Done)
	{
		Send("Sorry, your done and can no longer do that.\r\n");
		return;
	}

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
		if("evilaccept" == command)
		{
			DoEvilAccept(remainder);
			return;
		}
		else if("eviloffer" == command)
		{
			DoEvilOffer(remainder);
			return;
		}
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
		if("reject" == command)
		{
			DoReject(remainder);
			return;
		}
		else if("results" == command)
		{
			DoResults();
			return;
		}
		else if("rate" == command)
		{
			DoRate(remainder);
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
