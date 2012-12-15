#include "Users.h"

#include <boost/uuid/uuid_generators.hpp>

Users::Users()
{
	// TODO read in users
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
