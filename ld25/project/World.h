#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <map>
#include <string>

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
	void Broadcast(const std::string& message);
	void GetUsers(std::vector<User*>& users);
	Session* GetSession(std::string username);

private:
	typedef std::map<std::string, Session*> SessionMap;
	SessionMap _sessions;
};

#endif
