#include "Contracts.h"
#include "LawlJSON.h"
#include "Util.h"
#include "Log.h"

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

				if( obj.end() != user1 && user1->second.IsString() &&
					obj.end() != user2 && user2->second.IsString() &&
					obj.end() != user1contrib && user1contrib->second.IsNumber() &&
					obj.end() != user2contrib && user2contrib->second.IsNumber() &&
					obj.end() != duration && duration->second.IsNumber() )
				{
					Contract* contract = new Contract;
					contract->User1 = user1->second.string();
					contract->User2 = user2->second.string();
					contract->User1Contribution = (size_t)user1contrib->second.number();
					contract->User2Contribution = (size_t)user2contrib->second.number();
					contract->Duration = (size_t)duration->second.number();
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
		arr.push_back(entry);
	}

	string results;
	Serialize(arr, results);

	Util::WriteFile("contracts.json", results.data(), results.length());
}

Contract::Contract()
	: User1Contribution(0)
	, User2Contribution(0)
	, Duration(0)
{}
