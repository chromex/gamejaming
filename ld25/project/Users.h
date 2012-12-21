#ifndef USERS_H
#define USERS_H

#include <string>
#include <vector>

#include "Time.h"

struct User
{
	User();
	~User();

	std::string Username;
	std::string Password;
	std::string About;
	size_t Money;
	int Respect;
	bool Admin;
	bool Started;
	bool Done;
	Time* StartTime;
};

class Users
{
	typedef std::vector<User*> UserVec;

public:
	Users();
	~Users();

	static Users* Instance();

	void Save() const;
	void Tick();

	User* GetUserByUsername(std::string uname);
	User* CreateUser(const std::string& uname, const std::string& password);
	const UserVec& GetLeaders();

private:
	void ComputeLeaders(size_t nPositions);

	UserVec _users;
	UserVec _leaders;
};

#endif