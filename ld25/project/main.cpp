#ifndef LOG_H
#include "Log.h"
#endif

#ifndef SERVER_H
#include "Server.h"
#endif

#ifndef SETTINGS_H
#include "Settings.h"
#endif

#ifndef USERS_H
#include "Users.h"
#endif

#ifndef CONTRACTS_H
#include "Contracts.h"
#endif

int main(int argc, char** argv)
{
	Log("Starting LD25: Prisoner's Dilemma Server");

	Users::Instance();
	Contracts::Instance();

	Log("Data loaded");

	Server(Settings::port).Run();

	Log("Stopping LD25");

	return 0;
}
