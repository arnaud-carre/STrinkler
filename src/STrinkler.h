#pragma once

typedef	unsigned char u8;
typedef	unsigned short u16;
typedef	unsigned int u32;

struct Args 
{
	const char*	sInfile;
	const char* sOutFile;
	bool data;
	int preset;
	bool mini;
};
