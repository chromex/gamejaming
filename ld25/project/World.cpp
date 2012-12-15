#include "World.h"
#include "Log.h"
#include "Session.h"

World::World()
{

}

World::~World()
{
	for(SessionMap::iterator entry = _sessions.begin(); entry != _sessions.end(); ++entry)
	{
		entry->second->Stop();
	}
}

World* World::Instance()
{
	static World world;
	return &world;
}

void World::AddSession( Session* session )
{
	if(0 == session)
	{
		LogWarning("Null session received, ignoring");
		return;
	}

	if(0 == session->GetUser())
	{
		LogWarning("Session has no user, ignoring");
		return;
	}

	string username = session->GetUser()->Username;
	if(_sessions.find(username) == _sessions.end())
	{
		_sessions[username] = session;
	}
	else
	{
		_sessions[username]->Stop();
		_sessions[username] = session;
		session->Send("Existing session closed to open this session\r\n");
	}
}

void World::RemoveSession( Session* session )
{
	for(SessionMap::iterator entry = _sessions.begin(); entry != _sessions.end(); ++entry)
	{
		if(entry->second == session)
		{
			_sessions.erase(entry);
			return;
		}
	}
}

void World::Broadcast( const string& message )
{
	for(SessionMap::iterator entry = _sessions.begin(); entry != _sessions.end(); ++entry)
	{
		entry->second->Send(message);
	}
}

void World::GetUsers( vector<User*>& users )
{
	for(SessionMap::iterator entry = _sessions.begin(); entry != _sessions.end(); ++entry)
	{
		users.push_back(entry->second->GetUser());
	}
}

Session* World::GetSession( const string& username )
{
	SessionMap::iterator session = _sessions.find(username);
	if(session == _sessions.end())
		return 0;
	else
		return session->second;
}
