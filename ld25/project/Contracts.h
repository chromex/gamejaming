#ifndef CONTRACTS_H
#define CONTRACTS_H

#include <string>
#include <vector>
using namespace std;

class Session;
struct User;

struct Contract
{
	Contract();

	string User1;
	string User2;
	size_t User1Contribution;
	size_t User2Contribution;
	size_t Duration;
	bool Pending;
};

class Contracts
{
public:
	Contracts();
	~Contracts();

	static Contracts* Instance();

	Contract* CreateContract(Session* sender, int myAmount, const string& other, int theirAmount, int time);
	void GetOffers(User* user, vector<Contract*>& offers);
	void GetContracts(User* user, vector<Contract*>& contracts);
	void AcceptOffer(Session* sender, const string& other);
	void RejectOffer(Session* sender, const string& other);

	void Save() const;
	void Tick();

private:
	bool ContractExists(User* user1, User* user2) const;

	typedef vector<Contract*> ContractVec;
	ContractVec _contracts;
};

#endif