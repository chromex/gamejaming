#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <map>
using namespace std;

class Session;
struct User;

class World
{
public:
	World();
	~World();

	static World* Instance();
	void AddSession(Session* session);
	void RemoveSession(Session* session);
	void Broadcast(const string& message);
	void GetUsers(vector<User*>& users);

private:
	typedef map<string, Session*> SessionMap;
	SessionMap _sessions;
};

#endif