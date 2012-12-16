#include "Contracts.h"
#include "LawlJSON.h"
#include "Util.h"
#include "Log.h"
#include "Session.h"
#include "Users.h"
#include "World.h"
#include "Settings.h"

Contracts::Contracts()
{
	char* data = Util::LoadFile("contracts.json");
	if(0 == data)
	{
		LogError("Could not load user data from contracts.json");
		return;
	}

	LJValue results;

	try
	{
		ParseJSON(data, results);
	}
	catch(std::exception& e)
	{
		delete[] data;
		LogError("Error parsing JSON data: " << e.what());
		return;
	}

	delete[] data;

	if(results.IsArray())
	{
		LJArray& arr = results.array();
		for(LJArray::iterator entry = arr.begin(); entry != arr.end(); ++entry)
		{
			if(entry->IsObject())
			{
				LJObject& obj = entry->object();

				LJObject::iterator user1 = obj.find("User1");
				LJObject::iterator user2 = obj.find("User2");
				LJObject::iterator user1contrib = obj.find("User1Contribution");
				LJObject::iterator user2contrib = obj.find("User2Contribution");
				LJObject::iterator duration = obj.find("Duration");
				LJObject::iterator pending = obj.find("Pending");

				if( obj.end() != user1 && user1->second.IsString() &&
					obj.end() != user2 && user2->second.IsString() &&
					obj.end() != user1contrib && user1contrib->second.IsNumber() &&
					obj.end() != user2contrib && user2contrib->second.IsNumber() &&
					obj.end() != duration && duration->second.IsNumber() &&
					obj.end() != pending && pending->second.IsBoolean() )
				{
					Contract* contract = new Contract;
					contract->User1 = user1->second.string();
					contract->User2 = user2->second.string();
					contract->User1Contribution = (size_t)user1contrib->second.number();
					contract->User2Contribution = (size_t)user2contrib->second.number();
					contract->Duration = (size_t)duration->second.number();
					contract->Pending = pending->second.boolean();
					_contracts.push_back(contract);
				}
			}
		}
	}
}

Contracts::~Contracts()
{
	Save();

	for(ContractVec::iterator cp = _contracts.begin(); cp != _contracts.end(); ++cp)
	{
		delete *cp;
	}
}

Contracts* Contracts::Instance()
{
	static Contracts db;
	return &db;
}

void Contracts::Save() const
{
	LJArray arr;

	for(ContractVec::const_iterator cp = _contracts.begin(); cp != _contracts.end(); ++cp)
	{
		LJObject entry;
		entry["User1"] = (*cp)->User1;
		entry["User2"] = (*cp)->User2;
		entry["User1Contribution"] = (double)(*cp)->User1Contribution;
		entry["User2Contribution"] = (double)(*cp)->User2Contribution;
		entry["Duration"] = (double)(*cp)->Duration;
		entry["Pending"] = (*cp)->Pending;
		arr.push_back(entry);
	}

	string results;
	Serialize(arr, results);

	Util::WriteFile("contracts.json", results.data(), results.length());
}

void Contracts::Tick()
{

}

Contract* Contracts::CreateContract( Session* sender, int myAmount, const string& other, int theirAmount, int time )
{
	User* otherUser = Users::Instance()->GetUserByUsername(other);
	if(0 == otherUser)
	{
		stringstream ss;
		ss << "No such user '" << other << "'. Cannot create contract offer.\r\n";
		sender->Send(ss.str());
		return 0;
	}

	if(otherUser == sender->GetUser())
	{
		sender->Send("Cannot create a contract with yourself, jackass.\r\n");
		return 0;
	}

	if(ContractExists(sender->GetUser(), otherUser))
	{
		stringstream ss;
		ss << "Contract or offer already exists between you and '" << otherUser->Username << "'.\r\n";
		sender->Send(ss.str());
		return 0;
	}

	Contract* contract = new Contract;
	contract->User1 = sender->GetUser()->Username;
	contract->User2 = otherUser->Username;
	contract->User1Contribution = myAmount;
	contract->User2Contribution = theirAmount;
	contract->Duration = time;
	_contracts.push_back(contract);

	sender->Send("Offer sent\r\n");

	Session* otherSession = World::Instance()->GetSession(other);
	if(0 != otherSession)
	{
		otherSession->Send("New offer received\r\n");
	}

	return contract;
}

bool Contracts::ContractExists( User* user1, User* user2 ) const
{
	if(0 == user1 || 0 == user2)
		return false;

	for(ContractVec::const_iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		Contract* ptr = *contract;
		if( (user1->Username == ptr->User1 && user2->Username == ptr->User2) ||
			(user1->Username == ptr->User2 && user2->Username == ptr->User1) )
		{
			return true;
		}
	}

	return false;
}

void Contracts::GetOffers( User* user, vector<Contract*>& offers )
{
	for(ContractVec::const_iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		if(((*contract)->User1 == user->Username || (*contract)->User2 == user->Username) && (*contract)->Pending)
			offers.push_back(*contract);
	}
}

void Contracts::GetContracts( User* user, vector<Contract*>& contracts )
{
	for(ContractVec::const_iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		if(((*contract)->User1 == user->Username || (*contract)->User2 == user->Username) && !(*contract)->Pending)
			contracts.push_back(*contract);
	}
}

void Contracts::AcceptOffer( Session* sender, const string& other )
{
	ContractVec offers;
	GetOffers(sender->GetUser(), offers);

	if(0 == offers.size())
	{
		sender->Send("No offers to accept.\r\n");
		return;
	}

	ContractVec contracts;
	GetContracts(sender->GetUser(), contracts);
	if(contracts.size() >= Settings::maxContracts)
	{
		sender->Send("You cannot accept new contracts until some of your existing ones expire.\r\n");
		return;
	}

	User* otherUser = Users::Instance()->GetUserByUsername(other);
	if(0 == otherUser)
	{
		sender->Send("No such user to accept an offer from.\r\n");
		return;
	}

	Contract *c = 0;
	for(ContractVec::iterator contract = offers.begin(); contract != offers.end(); ++contract)
	{
		if((*contract)->User1 == otherUser->Username)
		{
			c = *contract;
			break;
		}
	}

	if(0 == c)
	{
		stringstream ss;
		ss << "No contract sent by '" << otherUser->Username << "' to accept.\r\n";
		sender->Send(ss.str());
		return;
	}

	if(!c->Pending)
	{
		sender->Send("Can't accept a non-pending contract.\r\n");
		return;
	}

	contracts.clear();
	GetContracts(otherUser, contracts);
	if(contracts.size() >= Settings::maxContracts)
	{
		stringstream ss;
		ss << "'" << otherUser->Username << "' can't accept any more offers for now.\r\n";
		sender->Send(ss.str());
	}

	if(sender->GetUser()->Money < c->User2Contribution)
	{
		sender->Send("You cannot afford the offer!\r\n");
		return;
	}

	if(otherUser->Money < c->User1Contribution)
	{
		stringstream ss;
		ss << "'" << otherUser->Username << "' cannot afford the offer!\r\n";
		sender->Send(ss.str());
		return;
	}

	sender->GetUser()->Money -= c->User2Contribution;
	otherUser->Money -= c->User1Contribution;
	c->Pending = false;

	sender->Send("Contract accepted!\r\n");
	
	Session* otherSession = World::Instance()->GetSession(otherUser->Username);
	if(0 != otherSession)
	{
		stringstream ss;
		ss << "'" << sender->GetUser()->Username << "' accepted your offer!\r\n";
		otherSession->Send(ss.str());
	}
}

void Contracts::RejectOffer( Session* sender, const string& other )
{
	ContractVec offers;
	GetOffers(sender->GetUser(), offers);

	if(0 == offers.size())
	{
		sender->Send("No offers to reject.\r\n");
		return;
	}

	User* otherUser = Users::Instance()->GetUserByUsername(other);
	if(0 == otherUser)
	{
		sender->Send("No such user to reject an offer from.\r\n");
		return;
	}

	Contract *c = 0;
	for(ContractVec::iterator contract = offers.begin(); contract != offers.end(); ++contract)
	{
		if((*contract)->User1 == otherUser->Username)
		{
			c = *contract;
			break;
		}
	}

	if(0 == c)
	{
		stringstream ss;
		ss << "No contract sent by '" << otherUser->Username << "' to reject.\r\n";
		sender->Send(ss.str());
		return;
	}

	if(!c->Pending)
	{
		sender->Send("Can't reject a non-pending contract.\r\n");
		return;
	}

	sender->Send("Offer rejected.\r\n");

	Session* otherSession = World::Instance()->GetSession(otherUser->Username);
	if(0 != otherSession)
	{
		stringstream ss;
		ss << "'" << sender->GetUser()->Username << "' rejected your offer! Screw that guy!\r\n";
		otherSession->Send(ss.str());
	}

	delete c;
	ContractVec::iterator entry = find(_contracts.begin(), _contracts.end(), c);
	_contracts.erase(entry);
}

Contract::Contract()
	: User1Contribution(0)
	, User2Contribution(0)
	, Duration(0)
	, Pending(true)
{}
