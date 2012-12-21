#ifndef TIME_H
#define TIME_H

#include <string>

class Time
{
public:
	static Time* TimeFromLocalTime();
	static Time* TimeFromString(const std::string& str);
	virtual ~Time() {}

	virtual std::string ToString() const = 0;
	virtual int MinutesFromLocal() const = 0;

	static std::string LocalTimeString();

protected:
	Time() {}
};

#endif