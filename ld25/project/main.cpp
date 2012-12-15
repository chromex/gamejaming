#include <iostream>
using namespace std;

#include "Log.h"
#include "Server.h"
#include "Settings.h"

int main(int argc, char** argv)
{
	Log("Starting LD25: Prisoner's Dilemma Server");

	Server(Settings::port).Run();

	Log("Stopping LD25");

	system("pause");

	return 0;
}