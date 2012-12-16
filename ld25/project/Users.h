#ifndef USERS_H
#define USERS_H

#include <string>
#include <vector>
using namespace std;

#include <boost/date_time/posix_time/posix_time.hpp>

struct User
{
	User();

	string Username;
	string Password;
	string About;
	size_t Money;
	int Respect;
	bool Admin;
	bool Started;
	bool Done;
	boost::posix_time::ptime StartTime;
};

class Users
{
public:
	Users();
	~Users();

	static Users* Instance();

	void Save() const;
	void Tick();

	User* GetUserByUsername(string uname);
	User* CreateUser(const string& uname, const string& password);
	const vector<User*>& GetLeaders();

private:
	void ComputeLeaders(size_t nPositions);

	typedef vector<User*> UserVec;
	UserVec _users;
	UserVec _leaders;
};

#endif