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
	size_t Money;
	size_t Respect;
	bool Admin;
};

class Users
{
public:
	Users();
	~Users();

	static Users* Instance();

	User* GetUserByUsername(const string& uname);
	User* CreateUser(const string& uname, const string& password);

private:
	typedef vector<User*> UserVec;
	UserVec _users;
};

#endif