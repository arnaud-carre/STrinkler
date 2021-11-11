//-----------------------------------------------------------------
//
//	STrinkler, Atari exe packer suited for 4KiB demo
//	Atari platform support by Leonard/Oxygene
//	( https://github.com/arnaud-carre/STrinkler )
//	
//	Use Shrinkler packing technology by Simon Christensen
//	( https://github.com/askeksa/Shrinkler )
//
//-----------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "STrinkler.h"
#include "BinaryBlob.h"
#include "Shrinkler/Pack.h"

#define NUM_RELOC_CONTEXTS 256

static const u16 sMiniHeader[] = 
{
	#include "bootstrap/mini.h"
};



bool	ShrinklerDataPack(const BinaryBlob& in, BinaryBlob& out, int preset)
{
	printf("Shrinkler packing %d bytes (level %d)...\n", in.GetSize(), preset);
	vector<unsigned> pack_buffer;
	assert((preset >= 1) && (preset <= 9));
	out.Release();
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
	out.LoadFromW32(&pack_buffer[0], pack_buffer.size());
	delete range_coder;
	printf("  Packed to %d bytes!\n", out.GetSize());
	return true;
}

bool	ArgParsing(int argc, char* argv[], Args& args)
{
	args.preset = 2;
	args.data = false;
	args.mini = false;

	int fileCount = 0;
	for (int i = 1; i < argc; i++)
	{
		if ('-' == argv[i][0])
		{
			if ('d' == argv[i][1])
			{
				args.data = true;
			}
			else if (0 == strcmp(argv[i],"-mini"))
			{
				args.mini = true;
			}
			else
			{
				int p = atoi(argv[i]+1);
				if ((p >= 1) && (p <= 9))
				{
					args.preset = p;
				}
				else
				{
					printf("ERROR: Unknown option \"%s\"\n", argv[i]);
					return false;
				}
			}
		}
		else
		{
			if (0 == fileCount)
				args.sInfile = argv[i];
			else if (1 == fileCount)
				args.sOutFile = argv[i];
			else
				return false;
			fileCount++;
		}
	}
	return (2 == fileCount);
}

void	Usage()
{
	printf("Usage: STrinkler [option] <input file> <output file>\n\n");
	printf("Options:\n"
		"  -1, ..., -9   compression level (low, best)\n"
		"  -mini         minimal PRG size, no relocation table, less compatibility\n"
		"  -d            raw data mode\n"
	);
}

static bool	OutputMiniVersion(const BinaryBlob& bin, const Args& args, BinaryBlob& bout)
{
	bool ret = false;
	if (!bin.IsRelocationTable())
	{
		assert(!bin.IsRelocationTable());
		BinaryBlob codePack;
		if (ShrinklerDataPack(bin, codePack, args.preset))
		{
			codePack.Align(2);

			int packedText = 4 + sizeof(sMiniHeader) + codePack.GetSize();
			int depackedBss = bin.GetSize() + bin.GetBssSectionSize();

		// write PRG header
			bout.w16(0x601a);
			bout.w32(packedText);
			bout.w32(0);
			bout.w32(depackedBss);
			bout.w32(0);
			bout.w32(0);
			bout.w32(0);
			bout.w16(0xffff);		// no relocation table

			int packedDataOffset = 4 + sizeof(sMiniHeader) + codePack.GetSize(); // +4 for first lea n(pc),a5
			if (packedDataOffset < 32768)
			{
				bout.w16(0x4bfa);		// lea n(pc),a5
				bout.w16(packedDataOffset - 2);
				bout.AppendW16(sMiniHeader, sizeof(sMiniHeader) / sizeof(short));
				bout.Append(codePack.GetData(), codePack.GetSize());
				ret = true;
			}
			else
			{
				printf("ERROR: packed data too large for -mini mode ( < 32KiB )\n");
			}

		}
	}
	else
	{
		printf("ERROR: -mini mode doesn't support EXE with relocation table\n");
	}
	return ret;
}


int main(int argc, char* argv[])
{
	printf(	"STrinkler v0.1 - Atari 4KiB exe packer\n"
			"Shrinkler compression technology by Simon Christensen\n"
			"Atari platform support by Leonard/Oxygene\n\n");

	int ret = 0;

	Args args;
	if (!ArgParsing(argc, argv, args))
	{
		Usage();
		return -1;
	}

	BinaryBlob bin;
	BinaryBlob bout;

	printf("Loading input file \"%s\"\n", args.sInfile);
	if (bin.LoadFromFile(args.sInfile))
	{
		bool outOk = false;

		if (args.data)
		{
			outOk = ShrinklerDataPack(bin, bout, args.preset);
		}
		else
		{
			if (bin.IsAtariExecutable())
			{
				bin.AtariRelocParse();
				bin.AtariCodeShrink();
				bin.SaveFile("x:\\4ksos\\raw.bin");
			}
			else
			{
				printf("ERROR: Input file is not an ATARI executable\n");
				return -1;
			}

			if (args.mini)
			{
				outOk = OutputMiniVersion(bin, args, bout);
			}
			else
			{
				printf("ERROR: This version only supports -mini mode for EXE\n");
				return -1;
			}
		}

		if (outOk)
		{
			printf("Saving \"%s\" (%d bytes)\n", args.sOutFile, bout.GetSize());
			bout.SaveFile(args.sOutFile);
		}
	}
	else
	{
		printf("ERROR: Unable to load \"%s\"\n", args.sInfile);
		ret = -1;
	}

	return ret;
}
