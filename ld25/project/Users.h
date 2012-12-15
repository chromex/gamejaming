#ifndef USERS_H
#define USERS_H

#include <string>
#include <vector>
using namespace std;

#include <boost/uuid/uuid.hpp>

struct User
{
	string Username;
	string Password;
	int Money;
	int Respect;
	boost::uuids::uuid ID;
};

class Users
{
public:
	Users();
	~Users();

	static Users* Instance();

	User* GetUserByUsername(const string& uname);

private:
	typedef vector<User*> UserVec;
	UserVec _users;
};

#endif