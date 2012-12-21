#include "Time.h"

#include <boost/date_time/posix_time/posix_time.hpp>

class BoostTime : public Time
{
public:
	BoostTime()
		: _time(boost::posix_time::second_clock::local_time())
	{}

	BoostTime(const std::string& str)
		: _time(boost::posix_time::time_from_string(str))
	{}

	virtual ~BoostTime()
	{}

	virtual std::string ToString() const
	{
		return boost::posix_time::to_simple_string(_time);
	}

	virtual int MinutesFromLocal() const
	{
		boost::posix_time::time_duration td = boost::posix_time::second_clock::local_time() - _time;
		return ((td.hours() * 60) + td.minutes());
	}

private:
	boost::posix_time::ptime _time;
};

Time* Time::TimeFromLocalTime()
{
	return new BoostTime;
}

Time* Time::TimeFromString(const std::string& str)
{
	return new BoostTime(str);
}

std::string Time::LocalTimeString()
{
	return boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time());
}