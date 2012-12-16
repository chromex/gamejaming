#include "Users.h"
#include "Util.h"
#include "Log.h"
#include "Settings.h"
#include "LawlJSON.h"

#include <algorithm>

Users::Users()
{
	char* data = Util::LoadFile("users.json");
	if(0 == data)
	{
		LogError("Could not load user data from users.json");
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

				LJObject::iterator username = obj.find("Username");
				LJObject::iterator password = obj.find("Password");
				LJObject::iterator admin = obj.find("Admin");
				LJObject::iterator money = obj.find("Money");
				LJObject::iterator about = obj.find("About");
				LJObject::iterator respect = obj.find("Respect");

				if( obj.end() != username && username->second.IsString() &&
					obj.end() != password && password->second.IsString() &&
					obj.end() != money && money->second.IsNumber() &&
					obj.end() != respect && respect->second.IsNumber())
				{
					if(0 != GetUserByUsername(username->second.string()))
					{
						LogWarning("Duplicate user entry found, skipping");
						continue;
					}

					User* user = new User;
					user->Username = username->second.string();
					user->Password = password->second.string();
					user->Money = (size_t)money->second.number();
					user->Respect = (size_t)respect->second.number();

					if(obj.end() != about && about->second.IsString())
					{
						user->About = about->second.string();
					}

					if(obj.end() != admin && admin->second.IsBoolean())
					{
						user->Admin = admin->second.boolean();
					}
					_users.push_back(user);
				}
			}
		}
	}

	Tick();
}

Users::~Users()
{
	Save();

	for(UserVec::iterator up = _users.begin(); up != _users.end(); ++up)
	{
		delete *up;
	}
}

void Users::Save() const
{
	LJArray arr;

	for(UserVec::const_iterator up = _users.begin(); up != _users.end(); ++up)
	{
		LJObject entry;
		entry["Username"] = (*up)->Username;
		entry["Password"] = (*up)->Password;
		entry["About"] = (*up)->About;
		entry["Money"] = (double)((*up)->Money);
		entry["Respect"] = (double)((*up)->Respect);
		entry["Admin"] = (*up)->Admin;
		arr.push_back(entry);
	}

	string results;
	Serialize(arr, results);

	Util::WriteFile("users.json", results.data(), results.length());
}

User* Users::GetUserByUsername( string uname )
{
	std::transform(uname.begin(), uname.end(), uname.begin(), ::tolower);

	for(UserVec::const_iterator up = _users.begin(); up != _users.end(); ++up)
	{
		string other = (*up)->Username;
		std::transform(other.begin(), other.end(), other.begin(), ::tolower);
		if(other == uname)
		{
			return *up;
		}
	}

	return 0;
}

Users* Users::Instance()
{
	static Users db;
	return &db;
}

User* Users::CreateUser( const string& uname, const string& password )
{
	if(0 != GetUserByUsername(uname))
	{
		return 0;
	}

	User* user = new User;
	user->Username = uname;
	user->Password = password;
	user->Money = 2000;
	user->Respect = 100;
	user->About = "No about message set";
	user->Admin = _users.size() == 0;
	_users.push_back(user);

	Log("New user created: " << uname);

	return user;
}

bool CompareUser(User* a, User* b)
{
	return a->Money > b->Money;
}

const vector<User*>& Users::GetLeaders()
{
	return _leaders;
}

void Users::ComputeLeaders( size_t nPositions )
{
	_leaders.clear();

	for(UserVec::const_iterator up = _users.begin(); up != _users.end(); ++up)
	{
		if(_leaders.size() < nPositions)
		{
			_leaders.push_back(*up);

			if(_leaders.size() == nPositions)
			{
				sort(_leaders.begin(), _leaders.end(), CompareUser);
			}
		}
		else if((*up)->Money > _leaders[nPositions-1]->Money)
		{
			_leaders[nPositions-1] = *up;
			sort(_leaders.begin(), _leaders.end(), CompareUser);
		}
	}

	sort(_leaders.begin(), _leaders.end(), CompareUser);
}

void Users::Tick()
{
	ComputeLeaders(Settings::numLeaders);
}

User::User()
	: Money(0)
	, Respect(0)
	, Admin(false)
{}
