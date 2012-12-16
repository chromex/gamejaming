#ifndef LOG_H
#define LOG_H

#include <sstream>
#include <string>
#include <iostream>

#include <boost/date_time/posix_time/posix_time.hpp>

#define Log(S) {std::stringstream ss; ss << "[Info] " << boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) << " " << S; std::cout << ss.str() << std::endl;}
#define LogWarning(S) {std::stringstream ss; ss << "[Warning] " << boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) << " " << S; std::cout << ss.str() << std::endl;}
#define LogError(S) {std::stringstream ss; ss << "[Error] " << boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) << " " << S; std::cout << ss.str() << std::endl;}

#endif
