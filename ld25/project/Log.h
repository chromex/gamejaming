#ifndef LOG_H
#define LOG_H

#include <sstream>
#include <string>
#include <iostream>

#define Log(S) {std::stringstream ss; ss << "[Info] " << S << std::endl; std::cout << ss.str();}
#define LogWarning(S) {std::stringstream ss; ss << "[Warning] " << S << std::endl; std::cout << ss.str();}
#define LogError(S) {std::stringstream ss; ss << "[Error] " << S << std::endl; std::cout << ss.str();}

#endif