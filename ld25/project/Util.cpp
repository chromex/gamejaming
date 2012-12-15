#include "Util.h"
#include "Log.h"

#include <stdio.h>

char * Util::LoadFile(const char* filename) {
	char *buffer;
	size_t fsize;

	// open
	FILE *fin = fopen(filename, "rb");
	if(!fin)
	{
		LogError("Failed to open file for reading: " << filename);
		return 0;
	}

	// compute size
	fseek(fin, 0, SEEK_END);
	fsize = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	buffer = new char[fsize + 1];

	if(fsize != fread(buffer, sizeof(char), fsize, fin))
	{
		delete[] buffer;
		buffer = 0;
	}
	else
	{
		buffer[fsize] = 0;
	}
	fclose(fin);

	return buffer;
}

void Util::WriteFile(const char* filename, const char* content, size_t nBytes) {
	FILE *fout = fopen(filename, "wb");
	if(!fout)
	{
		LogError("Failed to open file for writing: " << filename);
	}

	size_t n = fwrite(content, 1, nBytes, fout);
	fclose(fout);

	if(n != nBytes)
	{
		LogError("Failed to write total contents to file: " << filename);
	}
}