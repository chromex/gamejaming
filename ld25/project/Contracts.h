#ifndef CONTRACTS_H
#define CONTRACTS_H

#include <string>
#include <vector>
using namespace std;

#include <boost/date_time/posix_time/posix_time.hpp>

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
	bool Done;
	boost::posix_time::ptime StartTime;
	size_t User1Profit;
	size_t User2Profit;
	bool Evil1;
	bool Evil2;
};

class Contracts
{
public:
	Contracts();
	~Contracts();

	static Contracts* Instance();

	Contract* CreateContract(Session* sender, int myAmount, const string& other, int theirAmount, int time, bool evil);
	void GetOffers(User* user, vector<Contract*>& offers);
	void GetContracts(User* user, vector<Contract*>& contracts);
	void GetFinished(User* user, vector<Contract*>& finished);
	void AcceptOffer(Session* sender, const string& other, bool evil);
	void RejectOffer(Session* sender, const string& other);

	void Save() const;
	void Tick();

private:
	bool ContractExists(User* user1, User* user2) const;

	typedef vector<Contract*> ContractVec;
	ContractVec _contracts;
};

#endif