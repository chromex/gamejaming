#include "Contracts.h"
#include "LawlJSON.h"
#include "Util.h"
#include "Log.h"
#include "Session.h"
#include "Users.h"
#include "World.h"
#include "Settings.h"

#include <boost/date_time/posix_time/posix_time.hpp>

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
				LJObject::iterator startTime = obj.find("StartTime");
				LJObject::iterator done = obj.find("Done");
				LJObject::iterator user1profit = obj.find("User1Profit");
				LJObject::iterator user2profit = obj.find("User2Profit");
				LJObject::iterator evil1 = obj.find("Evil1");
				LJObject::iterator evil2 = obj.find("Evil2");
				LJObject::iterator rated1 = obj.find("Rated1");
				LJObject::iterator rated2 = obj.find("Rated2");

				if( obj.end() != user1 && user1->second.IsString() &&
					obj.end() != user2 && user2->second.IsString() &&
					obj.end() != user1contrib && user1contrib->second.IsNumber() &&
					obj.end() != user2contrib && user2contrib->second.IsNumber() &&
					obj.end() != duration && duration->second.IsNumber() &&
					obj.end() != pending && pending->second.IsBoolean() &&
					obj.end() != done && done->second.IsBoolean() &&
					obj.end() != user1profit && user1profit->second.IsNumber() &&
					obj.end() != user2profit && user2profit->second.IsNumber() &&
					obj.end() != evil1 && evil1->second.IsBoolean() &&
					obj.end() != evil2 && evil2->second.IsBoolean() &&
					obj.end() != rated1 && rated1->second.IsBoolean() &&
					obj.end() != rated2 && rated2->second.IsBoolean() )
				{
					Contract* contract = new Contract;
					contract->User1 = user1->second.string();
					contract->User2 = user2->second.string();
					contract->User1Contribution = (size_t)user1contrib->second.number();
					contract->User2Contribution = (size_t)user2contrib->second.number();
					contract->Duration = (size_t)duration->second.number();
					contract->Pending = pending->second.boolean();
					contract->Done = done->second.boolean();
					contract->User1Profit = (size_t)user1profit->second.number();
					contract->User2Profit = (size_t)user2profit->second.number();
					contract->Evil1 = evil1->second.boolean();
					contract->Evil2 = evil2->second.boolean();
					contract->Rated1 = rated1->second.boolean();
					contract->Rated2 = rated2->second.boolean();

					if(obj.end() != startTime && startTime->second.IsString())
					{
						contract->StartTime = boost::posix_time::time_from_string(startTime->second.string());
					}

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
		entry["Done"] = (*cp)->Done;
		entry["User1Profit"] = (double)(*cp)->User1Profit;
		entry["User2Profit"] = (double)(*cp)->User2Profit;
		entry["Evil1"] = (*cp)->Evil1;
		entry["Evil2"] = (*cp)->Evil2;
		entry["Rated1"] = (*cp)->Rated1;
		entry["Rated2"] = (*cp)->Rated2;
		if(!(*cp)->Pending)
			entry["StartTime"] = boost::posix_time::to_simple_string((*cp)->StartTime);
		arr.push_back(entry);
	}

	std::string results;
	Serialize(arr, results);

	Util::WriteFile("contracts.json", results.data(), results.length());
}

size_t Multiplier(size_t duration)
{
	size_t base = 1;

	while(duration > 1)
	{
		base *= 2;
		--duration;
	}

	return base;
}

void Contracts::Tick()
{
	boost::posix_time::ptime currentTime = boost::posix_time::second_clock::local_time();

	for(ContractVec::iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		Contract* ptr = *contract;

		if(ptr->Pending || ptr->Done)
			continue;

		if(currentTime > (ptr->StartTime + boost::posix_time::minutes(ptr->Duration)))
		{
			ptr->Done = true;

			size_t avg = (ptr->User1Contribution + ptr->User2Contribution) / 2;

			if(!ptr->Evil1 && !ptr->Evil2)
			{
				ptr->User1Profit = ptr->User1Contribution + Multiplier(ptr->Duration) * avg;
				ptr->User2Profit = ptr->User2Contribution + Multiplier(ptr->Duration) * avg;
			}
			else if(ptr->Evil1 && ptr->Evil2)
			{
				ptr->User1Profit = 0;
				ptr->User2Profit = 0;
			}
			else if(ptr->Evil1)
			{
				ptr->User1Profit = ptr->User1Contribution + Multiplier(ptr->Duration) * avg + ptr->User2Contribution + Multiplier(ptr->Duration) * avg;
				ptr->User2Profit = 0;
			}
			else if(ptr->Evil2)
			{
				ptr->User1Profit = 0;
				ptr->User2Profit = ptr->User1Contribution + Multiplier(ptr->Duration) * avg + ptr->User2Contribution + Multiplier(ptr->Duration) * avg;
			}

			User* user1 = Users::Instance()->GetUserByUsername(ptr->User1);
			User* user2 = Users::Instance()->GetUserByUsername(ptr->User2);

			if(0 == user1)
				LogError("'" << ptr->User1 << "' for contract could not be found!")
			else
				user1->Money += ptr->User1Profit;

			if(0 == user2)
				LogError("'" << ptr->User2 << "' for contract could not be found!")
			else
				user2->Money += ptr->User2Profit;

			Session* session1 = World::Instance()->GetSession(ptr->User1);
			Session* session2 = World::Instance()->GetSession(ptr->User2);

			Log("A contracted completed between " << ptr->User1 << " and " << ptr->User2 << " with " << ptr->User1Profit << " and " << ptr->User2Profit << " profit numbers");

			if(0 != session1)
			{
				std::stringstream ss;
				ss << "You made $" << ptr->User1Profit << " from your contract with '" << ptr->User2 << "'!\r\n";
				session1->Send(ss.str());
			}

			if(0 != session2)
			{
				std::stringstream ss;
				ss << "You made $" << ptr->User2Profit << " from your contract with '" << ptr->User1 << "'!\r\n";
				session2->Send(ss.str());
			}
		}
	}
}

Contract* Contracts::CreateContract( Session* sender, int myAmount, const std::string& other, int theirAmount, int time, bool evil )
{
	User* otherUser = Users::Instance()->GetUserByUsername(other);
	if(0 == otherUser)
	{
		std::stringstream ss;
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
		std::stringstream ss;
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
	contract->Evil1 = evil;
	_contracts.push_back(contract);

	if(evil)
		sender->Send("Offer sent, you evil bastard!\r\n");
	else
		sender->Send("Offer sent.\r\n");

	Log("Contract created between " << contract->User1 << " and " << contract->User2 << " with " << contract->User1Contribution << " and " << contract->User2Contribution << " contributions and evil: " << evil);

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
			if(!ptr->Done)
				return true;
		}
	}

	return false;
}

void Contracts::GetOffers( User* user, std::vector<Contract*>& offers )
{
	for(ContractVec::const_iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		if(((*contract)->User1 == user->Username || (*contract)->User2 == user->Username) && (*contract)->Pending)
			offers.push_back(*contract);
	}
}

void Contracts::GetContracts( User* user, std::vector<Contract*>& contracts )
{
	for(ContractVec::const_iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		if(((*contract)->User1 == user->Username || (*contract)->User2 == user->Username) && !(*contract)->Pending && !(*contract)->Done)
			contracts.push_back(*contract);
	}
}

void Contracts::GetFinished( User* user, std::vector<Contract*>& finished )
{
	for(ContractVec::const_iterator contract = _contracts.begin(); contract != _contracts.end(); ++contract)
	{
		if(((*contract)->User1 == user->Username || (*contract)->User2 == user->Username) && !(*contract)->Pending && (*contract)->Done)
			finished.push_back(*contract);
	}
}

void Contracts::AcceptOffer( Session* sender, const std::string& other, bool evil )
{
	ContractVec offers;
	GetOffers(sender->GetUser(), offers);

	if(0 == offers.size())
	{
		sender->Send("No offers to accept.\r\n");
		return;
	}

	User* otherUser = Users::Instance()->GetUserByUsername(other);
	if(0 == otherUser)
	{
		sender->Send("No such user to accept an offer from.\r\n");
		return;
	}

	if(otherUser == sender->GetUser())
	{
		sender->Send("Can't accept offers with yourself!\r\n");
		return;
	}

	ContractVec contracts;
	GetContracts(sender->GetUser(), contracts);
	if(contracts.size() >= Settings::maxContracts)
	{
		sender->Send("You cannot accept new contracts until some of your existing ones expire.\r\n");
		return;
	}

	if(otherUser->Done)
	{
		sender->Send("I can't let you do that, the other user is already out of time.\r\n");
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
		std::stringstream ss;
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
		std::stringstream ss;
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
		std::stringstream ss;
		ss << "'" << otherUser->Username << "' cannot afford the offer!\r\n";
		sender->Send(ss.str());
		return;
	}

	sender->GetUser()->Money -= c->User2Contribution;
	otherUser->Money -= c->User1Contribution;
	c->Pending = false;
	c->StartTime = boost::posix_time::second_clock::local_time();
	c->Evil2 = evil;

	if(!sender->GetUser()->Started)
	{
		sender->GetUser()->Started = true;
		sender->GetUser()->StartTime = Time::TimeFromLocalTime();
		Log(sender->GetUser()->Username << "'s life has begun!");
	}
	if(!otherUser->Started)
	{
		otherUser->Started = true;
		otherUser->StartTime = Time::TimeFromLocalTime();
		Log(otherUser->Username << "'s life has begun!");
	}

	if(evil)
		sender->Send("Contract evilly accepted! May you live in infamy!\r\n");
	else
		sender->Send("Contract accepted!\r\n");

	Log(sender->GetUser()->Username << " accepted " << otherUser->Username << "'s offer with evil: " << evil);
	
	Session* otherSession = World::Instance()->GetSession(otherUser->Username);
	if(0 != otherSession)
	{
		std::stringstream ss;
		ss << "'" << sender->GetUser()->Username << "' accepted your offer!\r\n";
		otherSession->Send(ss.str());
	}
}

void Contracts::RejectOffer( Session* sender, const std::string& other )
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
		std::stringstream ss;
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

	Log(sender->GetUser()->Username << " rejected " << otherUser->Username << "'s offer");

	Session* otherSession = World::Instance()->GetSession(otherUser->Username);
	if(0 != otherSession)
	{
		std::stringstream ss;
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
	, Done(false)
	, User1Profit(0)
	, User2Profit(0)
	, Evil1(false)
	, Evil2(false)
	, Rated1(false)
	, Rated2(false)
{}
