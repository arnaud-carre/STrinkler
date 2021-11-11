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

static void SetPreset(PackParams& params, int preset)
{
	params.iterations = 1 * preset;
	params.length_margin = 1 * preset;
	params.skip_length = 1000 * preset;
	params.match_patience = 100 * preset;	// effort
	params.max_same_length = 10 * preset;
}


bool	ShrinklerDataPack(const BinaryBlob& in, BinaryBlob& out, const Args& args, PackParams& packParams)
{
	vector<unsigned> pack_buffer;

	out.Release();
	RangeCoder *range_coder = new RangeCoder(LZEncoder::NUM_CONTEXTS + NUM_RELOC_CONTEXTS, pack_buffer);

	// Crunch the data
	range_coder->reset();

	if (args.verbose)
	{
		printf("Shrinkler compressor parameters:\n"
			"  iterations.....: %d\n"
			"  length_margin..: %d\n"
			"  skip_length....: %d\n"
			"  match_patience.: %d\n"
			"  max_same_length: %d\n"
			"  references.....: %d\n",
			packParams.iterations,
			packParams.length_margin,
			packParams.skip_length,
			packParams.match_patience,
			packParams.max_same_length,
			args.references);
	}

	printf("Shrinkler packing %d bytes...\n", in.GetSize());
	RefEdgeFactory edge_factory(args.references);

	packData(in.GetData(), in.GetSize(), 0, &packParams, range_coder, &edge_factory, false);
	range_coder->finish();
	out.LoadFromW32(&pack_buffer[0], pack_buffer.size());
	delete range_coder;
	printf("  Packed to %d bytes!\n", out.GetSize());
	return true;
}


static int	SetPresetOption(int argc, char* argv[], PackParams& packParams)
{
	for (int i = 1; i < argc; i++)
	{
		if ('-' == argv[i][0])
		{
			int p = atoi(argv[i] + 1);
			if ((p >= 1) && (p <= 9))
			{
				SetPreset(packParams, p);
				return i;
			}
		}
	}
	return -1;
}

bool	ArgParsing(int argc, char* argv[], Args& args, PackParams& packParams)
{
	int fileCount = 0;
	int presetArg = SetPresetOption(argc, argv, packParams);
	for (int i = 1; i < argc; i++)
	{
		if (i == presetArg)
			continue;

		if ('-' == argv[i][0])
		{
			if (0 == strcmp(argv[i], "-d"))
			{
				args.data = true;
			}
			else if (0 == strcmp(argv[i], "-v"))
			{
				args.verbose = true;
			}
			else if (0 == strcmp(argv[i], "-i"))
			{
				packParams.iterations = atoi(argv[i + 1]);
				i += 1;
			}
			else if (0 == strcmp(argv[i], "-l"))
			{
				packParams.length_margin = atoi(argv[i + 1]);
				i += 1;
			}
			else if (0 == strcmp(argv[i], "-a"))
			{
				packParams.max_same_length = atoi(argv[i + 1]);
				i += 1;
			}
			else if (0 == strcmp(argv[i], "-e"))
			{
				packParams.match_patience = atoi(argv[i + 1]);
				i += 1;
			}
			else if (0 == strcmp(argv[i], "-s"))
			{
				packParams.skip_length = atoi(argv[i + 1]);
				i += 1;
			}
			else if (0 == strcmp(argv[i], "-r"))
			{
				args.references = atoi(argv[i + 1]);
				i += 1;
			}
			else if (0 == strcmp(argv[i],"-mini"))
			{
				args.mini = true;
			}
			else if (0 == strcmp(argv[i], "-pads"))
			{
				args.padSize = atoi(argv[i + 1]);
				args.padText = argv[i + 2];
				i += 2;
			}
			else if (0 == strcmp(argv[i], "-padr"))
			{
				args.padSize = atoi(argv[i + 1]);
				args.padText = NULL;
				i += 1;
			}
			else
			{
				printf("ERROR: Unknown option \"%s\"\n", argv[i]);
				return false;
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
	printf("Usage: STrinkler [options] <input file> <output file>\n\n");
	printf("Options:\n"
		"  -1, ..., -9   compression level (low, best) (default=2)\n"
		"  -mini         minimal PRG size, no relocation table, less compatibility\n"
		"  -d            raw data mode\n"
		"  -pads <size> \"text\"  Pad till <size> using repeated <text>\n"
		"  -padr <size>           Pad till <size> using random bytes\n"
		"Advanced options:\n"
		"  -i <n>        Number of iterations for the compression (2)\n"
		"  -l <n>        Number of shorter matches considered for each match (2)\n"
		"  -a <n>        Number of matches of the same length to consider (20)\n"
		"  -e <n>        Perseverance in finding multiple matches (200)\n"
		"  -s <n>        Minimum match length to accept greedily (2000)\n"
		"  -r <n>        Number of reference edges to keep in memory (100000)\n"
	);
}

static bool	OutputMiniVersion(const BinaryBlob& bin, const Args& args, BinaryBlob& bout, PackParams& packParams)
{
	bool ret = false;
	if (!bin.IsRelocationTable())
	{
		assert(!bin.IsRelocationTable());
		BinaryBlob codePack;
		if (ShrinklerDataPack(bin, codePack, args, packParams))
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

				const int addedBytes = 0x1c + 4 + sizeof(sMiniHeader);
				printf("Adding \"mini\" bootstrap header (%d bytes)...\n", addedBytes);

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
			"Atari platform support by Leonard/Oxygene\n"
			"Shrinkler compression technology by Blueberry/Loonies\n"
			"\n");

	int ret = 0;

	PackParams packParams;
	SetPreset(packParams, 2);		// default level=2

	Args args;
	if (!ArgParsing(argc, argv, args, packParams))
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
			outOk = ShrinklerDataPack(bin, bout, args, packParams);
		}
		else
		{
			if (bin.IsAtariExecutable())
			{
				if (bin.AtariRelocParse(args.verbose))
					bin.AtariCodeShrink();
				else
				{
					printf("ERROR: Atari EXE file relocation table corrupted\n");
					return -1;
				}
			}
			else
			{
				printf("ERROR: Input file is not an ATARI executable\n");
				return -1;
			}

			if (args.mini)
			{
				outOk = OutputMiniVersion(bin, args, bout, packParams);
			}
			else
			{
				printf("ERROR: This version only supports -mini mode for EXE\n");
				return -1;
			}
		}

		if (outOk)
		{
			if (args.padSize > 0)
				bout.Pad(args.padSize, args.padText);

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
