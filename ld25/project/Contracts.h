#ifndef CONTRACTS_H
#define CONTRACTS_H

#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time_types.hpp>

class Session;
struct User;

struct Contract
{
	Contract();

	std::string User1;
	std::string User2;
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
	bool Rated1;
	bool Rated2;
};

class Contracts
{
	typedef std::vector<Contract*> ContractVec;

public:
	Contracts();
	~Contracts();

	static Contracts* Instance();

	Contract* CreateContract(Session* sender, int myAmount, const std::string& other, int theirAmount, int time, bool evil);
	void GetOffers(User* user, ContractVec& offers);
	void GetContracts(User* user, ContractVec& contracts);
	void GetFinished(User* user, ContractVec& finished);
	void AcceptOffer(Session* sender, const std::string& other, bool evil);
	void RejectOffer(Session* sender, const std::string& other);

	void Save() const;
	void Tick();

private:
	bool ContractExists(User* user1, User* user2) const;

	ContractVec _contracts;
};

#endif