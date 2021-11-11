//-----------------------------------------------------------------
//
//	STrinkler, Atari exe packer suited for 4KiB demo
//	Atari platform support by Leonard/Oxygene
//	( https://github.com/arnaud-carre/STrinkler )
//	
//	Use Shrinkler packing technology by Blueberry/Loonies
//	( https://github.com/askeksa/Shrinkler )
//
//-----------------------------------------------------------------
#pragma once

typedef	unsigned char u8;
typedef	unsigned short u16;
typedef	unsigned int u32;

struct Args 
{
	Args()
	{
		sInfile = NULL;
		sOutFile = NULL;
		data = false;
		verbose = false;
		mini = false;
		references = 100000;
	}

	const char*	sInfile;
	const char* sOutFile;
	bool data;
	bool verbose;
	bool mini;
	int references;
};
