#ifndef LOG_H
#define LOG_H

#include <sstream>
#include <iostream>
#include "Time.h"

#define Log(S) {std::stringstream ss; ss << "[Info] " << Time::LocalTimeString() << " " << S; std::cout << ss.str() << std::endl;}
#define LogWarning(S) {std::stringstream ss; ss << "[Warning] " << Time::LocalTimeString() << " " << S; std::cout << ss.str() << std::endl;}
#define LogError(S) {std::stringstream ss; ss << "[Error] " << Time::LocalTimeString() << " " << S; std::cout << ss.str() << std::endl;}

#endif
