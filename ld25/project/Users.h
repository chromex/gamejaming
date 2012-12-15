#ifndef USERS_H
#define USERS_H

#include <string>
#include <vector>
using namespace std;

struct User
{
	User();

	string Username;
	string Password;
	int Money;
	int Respect;
	bool Admin;
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