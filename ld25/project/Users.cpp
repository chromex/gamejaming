#include "Users.h"
#include "Util.h"
#include "Log.h"

#include "LawlJSON.h"

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

					if(obj.end() != admin && admin->second.IsBoolean())
					{
						user->Admin = admin->second.boolean();
					}
					_users.push_back(user);
				}
			}
		}
	}
}

Users::~Users()
{
	// TODO write out users

	for(UserVec::iterator up = _users.begin(); up != _users.end(); ++up)
	{
		delete *up;
	}
}

User* Users::GetUserByUsername( const string& uname )
{
	for(UserVec::const_iterator up = _users.begin(); up != _users.end(); ++up)
	{
		if((*up)->Username == uname)
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

User::User()
	: Money(0)
	, Respect(0)
	, Admin(false)
{}
