// STrinkler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <stdlib.h>
#include "STrinkler.h"
#include "BinaryBlob.h"
#include "Shrinkler/Pack.h"

#define NUM_RELOC_CONTEXTS 256



bool	ShrinklerDataPack(const BinaryBlob& in, BinaryBlob& out, int preset)
{
	vector<unsigned> pack_buffer;
	assert((preset >= 1) && (preset <= 9));

	printf("Shrinkler packing %d bytes (using preset -%d)\n", in.GetSize() , preset);

	RangeCoder *range_coder = new RangeCoder(LZEncoder::NUM_CONTEXTS + NUM_RELOC_CONTEXTS, pack_buffer);

	// Crunch the data
	range_coder->reset();

	PackParams params;
	params.iterations = 1 * preset;
	params.length_margin = 1 * preset;
	params.skip_length = 1000 * preset;
	params.match_patience = 100 * preset;
	params.max_same_length = 10 * preset;

	RefEdgeFactory edge_factory(100000);

	packData(in.GetData(), in.GetSize(), 0, &params, range_coder, &edge_factory, false);
	range_coder->finish();
	out.LoadFromMemory(&pack_buffer[0], pack_buffer.size() * 4);
	delete range_coder;
	return true;
}

bool	ArgParsing(int argc, char* argv[])
{
	return true;
}

void	Usage()
{
	printf("Usage: STrinkler [option] <input file> <output file>\n\n");
	printf("Options:\n"
		"  -d            raw data mode\n"
		"  -1, ..., -9  compression level (low, best)\n");
}

int main(int argc, char* argv[])
{

	printf(	"STrinkler v0.1\n"
			"Shrinkler compression technology by Simon Christensen\n"
			"Atari Executable support by Leonard/Oxygene\n\n");

	if (!ArgParsing(argc, argv))
	{
		Usage();
		return 0;
	}

	BinaryBlob bin;
	BinaryBlob bout;

	if (bin.LoadFromFile("x:\\4kSOS\\4k-SOSR.prg"))
	{
		bool AtariExe = bin.IsAtariExecutable();
		printf("Atari EXE: %s\n", AtariExe ? "Yes" : "No");


		ShrinklerDataPack(bin, bout, 3);
		printf("Packed to %d bytes\n", bout.GetSize());
	}



	return 0;
}
