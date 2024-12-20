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
#include <string.h>
#include "STrinkler.h"
#include "BinaryBlob.h"
#include "Shrinkler/Pack.h"
#include "Shrinkler/RangeDecoder.h"
#include "Shrinkler/LZDecoder.h"

#define NUM_RELOC_CONTEXTS 256

static const u16 sMiniHeader[] =
{
	#include "bootstrap/mini.h"
};

static const u16 sNormalHeader[] =
{
	#include "bootstrap/normal.h"
};

static void SetPreset(PackParams& params, int preset)
{
	params.iterations = 1 * preset;
	params.length_margin = 1 * preset;
	params.skip_length = 1000 * preset;
	params.match_patience = 100 * preset;	// effort
	params.max_same_length = 10 * preset;
}


bool	ShrinklerVerify(vector<unsigned>& pack_buffer, u8* unpackedData, int unpackedSize, int& safetyMargin)
{
	bool ret = false;
	RangeDecoder decoder(LZEncoder::NUM_CONTEXTS + NUM_RELOC_CONTEXTS, pack_buffer);
	LZDecoder lzd(&decoder);

	// Verify data
	LZVerifier verifier(0, unpackedData, unpackedSize, unpackedSize);
	decoder.reset();
	decoder.setListener(&verifier);
	if (lzd.decode(verifier))
	{
		// Check length
		if (verifier.size() == unpackedSize)
		{
			safetyMargin = verifier.front_overlap_margin + int(pack_buffer.size()) * 4 - unpackedSize;
			ret = true;
		}
		else
		{
			printf("ERROR: Data has incorrect length (%d, should have been %d)!\n", verifier.size(), (int)unpackedSize);
		}
	}
	else
	{
		printf("ERROR: Depacking check failed!\n");
	}
	return ret;
}


bool	ShrinklerDataPack(const BinaryBlob& in, BinaryBlob& out, const Args& args, PackParams& packParams, int& safetyMargin)
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
	out.LoadFromW32(&pack_buffer[0], int(pack_buffer.size()));
	delete range_coder;
	printf("  Packed to %d bytes!\n", out.GetSize());

	// verify
	safetyMargin = 0;
	if (ShrinklerVerify(pack_buffer, in.GetData(), in.GetSize(), safetyMargin))
	{
		if ( args.verbose )
			printf("  Verify safety margin=%d bytes\n", safetyMargin);
		return true;
	}

	return false;
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
		"  -mini         minimal PRG size, less compatibility (suited for 4KiB demo)\n"
		"  -d            raw data mode\n"
		"  -v            verbose\n"
		"  -padr <size>  Pad till <size> using random bytes\n"
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
		int safetyMargin;
		if (ShrinklerDataPack(bin, codePack, args, packParams, safetyMargin))
		{
			codePack.Align(2);

			int packedText = sizeof(sMiniHeader) + codePack.GetSize();
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

			int packedDataOffset = sizeof(sMiniHeader) + codePack.GetSize(); // +4 for first lea n(pc),a5
			if (packedDataOffset < 32768)
			{
				bout.AppendW16(sMiniHeader, sizeof(sMiniHeader) / sizeof(short));
				bout.Patch16(0x1c, 0x1234, packedDataOffset);
				bout.Append(codePack.GetData(), codePack.GetSize());
				ret = true;

				const int addedBytes = 0x1c + sizeof(sMiniHeader);
				printf("  Adding \"mini\" bootstrap header (%d bytes)...\n", addedBytes);

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

static bool	OutputNormalVersion(const BinaryBlob& bin, const Args& args, BinaryBlob& bout, PackParams& packParams)
{
	bool ret = false;

	BinaryBlob patchedInput;
	patchedInput.Append(bin.GetData(), bin.GetSize());
	patchedInput.Align(2);
	const int newRelocTableOffset = patchedInput.GetSize();
	int offset = 0;
	for (int i = 0; i < bin.GetRelocCount(); i++)
	{
		int delta = bin.GetRelocOffset(i) - offset;
		offset = bin.GetRelocOffset(i);
		while (delta > 32766)
		{
			patchedInput.w16(0);
			delta -= 32766;
		}
		patchedInput.w16(delta);
	}
	patchedInput.w16(0xffff);
	const int relocTableSize = patchedInput.GetSize() - newRelocTableOffset;

	BinaryBlob codePack;
	int safetyMargin;
	if (ShrinklerDataPack(patchedInput, codePack, args, packParams, safetyMargin))
	{
		codePack.Align(2);

		int packedText = sizeof(sNormalHeader) + codePack.GetSize();
		if (safetyMargin < 0)	// negative safety margin means end of packed could be before end of unpacked
			safetyMargin = 0;
		safetyMargin = (safetyMargin+3+1)&(-2);			// we're reading byte instead of 32bits in Atari version ( on purpose +4 instead of +3 )

		const int depackingBufferSize = sizeof(sNormalHeader) + patchedInput.GetSize() + safetyMargin;
		const int originalExeBufferSize = bin.GetSize() + bin.GetBssSectionSize();
		int bigBufferSize = std::max(depackingBufferSize, originalExeBufferSize);

		if (bigBufferSize > originalExeBufferSize)
		{
			const int addMem = (bigBufferSize - originalExeBufferSize + 1023) >> 10;
			printf("\"Depack in place\" will use %dKiB additional memory\n", addMem);
		}
		else
		{
			printf("\"Depack in place\" won't waste additional memory\n");
		}

		assert(packedText <= bigBufferSize);

		// write PRG header
		bout.w16(0x601a);
		bout.w32(packedText);
		bout.w32(0);
		bout.w32(bigBufferSize-packedText);
		bout.w32(0);
		bout.w32(0);
		bout.w32(0);
		bout.w16(0xffff);		// no relocation table (the real one is packed)

		// write header bootstrap
		bout.AppendW16(sNormalHeader, sizeof(sNormalHeader) / 2);
		int offs = 0x1c;

		offs = bout.Patch32(offs, 0x12345678, bin.GetTextSectionSize());
		offs = bout.Patch32(offs, 0x12345678, bin.GetBssSectionSize());
		offs = bout.Patch32(offs, 0x12345678, packedText);
		offs = bout.Patch32(offs, 0x12345678, bigBufferSize);
		bout.Patch16(offs, 0x1234, -relocTableSize);

		// packed data
		bout.Append(codePack.GetData(), codePack.GetSize());

		const int addedBytes = 0x1c + sizeof(sNormalHeader);
		printf("  Adding \"normal\" bootstrap header (%d bytes)...\n", addedBytes);

		ret = true;

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
			int safetyMargin;
			outOk = ShrinklerDataPack(bin, bout, args, packParams, safetyMargin);
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
				outOk = OutputNormalVersion(bin, args, bout, packParams);
			}
		}

		if (outOk)
		{
			printf("  Final EXE file size: %d bytes\n", bout.GetSize());
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
