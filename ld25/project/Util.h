#ifndef UTIL_H
#define UTIL_H

#include <string>
using namespace std;

namespace Util
{
	char * LoadFile(const char* filename);
	void WriteFile(const char* filename, const char* content, size_t nBytes);
}

#endif
