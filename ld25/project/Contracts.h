#ifndef CONTRACTS_H
#define CONTRACTS_H

#include <string>
#include <vector>
using namespace std;

struct Contract
{
	Contract();

	string User1;
	string User2;
	size_t User1Contribution;
	size_t User2Contribution;
	size_t Duration;
};

class Contracts
{
public:
	Contracts();
	~Contracts();

	static Contracts* Instance();

	void Save() const;

private:
	typedef vector<Contract*> ContractVec;
	ContractVec _contracts;
};

#endif