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

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BinaryBlob.h"

static const int	kMaxRelocEntries = 16 * 1024;

BinaryBlob::BinaryBlob()
{
	m_data = NULL;
	m_size = 0;
	m_relocTable = NULL;
	m_relocSize = 0;
	m_reserve = 0;
}

void	BinaryBlob::Align(int align)
{
	while (m_size%align)
		w8(0);
}

void	BinaryBlob::Release()
{
	free(m_data);
	free(m_relocTable);
	m_data = NULL;
	m_relocTable = NULL;
	m_size = 0;
	m_reserve = 0;
}

BinaryBlob::~BinaryBlob()
{
	Release();
}

bool	BinaryBlob::LoadFromFile(const char* sFilename)
{
	bool ret = false;
	FILE* h = fopen(sFilename, "rb");
	if (h)
	{
		Release();
		fseek(h, 0, SEEK_END);
		m_size = ftell(h);
		Reserve(m_size);
		fseek(h, 0, SEEK_SET);
		fread(m_data, 1, m_size, h);
		fclose(h);
		ret = true;
	}
	return ret;
}

bool	BinaryBlob::LoadFromW32(const u32* p, int count)
{
	Release();
	assert(count > 0);
	Reserve(count*4);
	for (int i = 0; i < count; i++)
		w32(p[i]);
	return true;
}

void	BinaryBlob::AppendW16(const u16* p, int count)
{
	for (int i = 0; i < count; i++)
		w16(p[i]);
}

void	BinaryBlob::Append(const void* p, int size)
{
	if (m_size + size > m_reserve)
	{
		m_reserve = m_size + size + 64*1024;
		m_data = (u8*)realloc(m_data, m_reserve);
	}
	memcpy(m_data + m_size, p, size);
	m_size += size;
}

void	BinaryBlob::Reserve(int size)
{
	m_reserve = size;
	m_data = (u8*)realloc(m_data, size);
}

void	BinaryBlob::w8(u8 v)
{
	if (m_size >= m_reserve)
		Reserve(m_reserve + 64 * 1024);
	assert(m_size < m_reserve);
	m_data[m_size++] = v;
}

void	BinaryBlob::w16(u16 v)
{
	w8(v >> 8);
	w8(v >> 0);
}

void	BinaryBlob::w32(u32 v)
{
	w8(v >> 24);
	w8(v >> 16);
	w8(v >> 8);
	w8(v >> 0);
}

u32	BinaryBlob::r32(int offset) const
{
	assert(offset + 4 <= m_size);
	u32 v = m_data[offset] << 24;
	v |= m_data[offset+1] << 16;
	v |= m_data[offset+2] << 8;
	v |= m_data[offset+3] << 0;
	return v;
}

u16	BinaryBlob::r16(int offset) const
{
	assert(offset + 2 <= m_size);
	u16 v = m_data[offset] << 8;
	v |= m_data[offset + 1] << 0;
	return v;
}

u8	BinaryBlob::r8(int offset) const
{
	assert(offset + 1 <= m_size);
	return m_data[offset];
}

bool	BinaryBlob::IsAtariExecutable() const
{
	if (r16(0) != 0x601a)
		return false;

	u32 endOffset = 0x1c;
	endOffset += r32(2);		// text section
	endOffset += r32(6);	// data section
	endOffset += r32(14);	// symbol table size

	if (endOffset > m_size)
	{
		printf("ERROR: PRG header CODE+DATA size doesn't match with file size\n");
		return false;
	}

	return true;
}

void	BinaryBlob::AtariCodeShrink()
{
	assert(IsAtariExecutable());
	int codeSize = r32(2);	// text section
	codeSize += r32(6);		// data section
	memmove(m_data, m_data + 0x1c, codeSize);
	m_size = codeSize;
}

bool	BinaryBlob::AtariRelocParse(bool verbose)
{
	assert(IsAtariExecutable());

	m_codeSize = r32(2) + r32(6);
	m_bssSize = r32(10);
	m_relocSize = 0;
	if (0 == r16(0x1a))		// if abs is non zero, then NO relocation table
	{
		m_relocTable = (int*)malloc(kMaxRelocEntries * sizeof(int));
		u32 readOffset = 0x1c;
		readOffset += r32(2);	// text section
		readOffset += r32(6);	// data section
		readOffset += r32(14);	// symbol table size
		if (readOffset+4 <= m_size)
		{
			int offset = r32(readOffset);
			if (offset != 0)		// classic first 0 offset means no relocation
			{
				if (offset + 4 > m_codeSize)			// 32bits reloc offset after code+data section!
					return false;
				m_relocTable[m_relocSize++] = offset;
				readOffset += 4;
				for (;;)
				{
					if (readOffset + 1 > m_size)
						return false;

					u8 c = r8(readOffset++);
					if (0 == c)
						break;
					if (1 == c)
						offset += 254;
					else
					{
						offset += c;
						if (m_relocSize >= kMaxRelocEntries)
							return false;
						if (offset + 4 > m_codeSize)			// 32bits reloc offset after code+data section!
							return false;
						m_relocTable[m_relocSize++] = offset;
					}
				}
			}
		}
		else
			return false;
	}
	if (verbose)
	{
		printf("ATARI exe infos:\n"
			"  code+data.: %d bytes\n"
			"  bss.......: %d bytes\n"
			"  relocation: %d entrie(s)\n"
			,m_codeSize, m_bssSize, m_relocSize);
	}
	return true;
}

bool	BinaryBlob::SaveFile(const char* sFilename)
{
	bool ret = false;
	FILE* h = fopen(sFilename, "wb");
	if (h)
	{
		fwrite(m_data, 1, m_size, h);
		fclose(h);
	}
	else
	{
		printf("ERROR: Unable to write file \"%s\"\n", sFilename);
	}
	return ret;
}

int	BinaryBlob::Patch16(int offset, u16 marker, u16 value)
{
	assert(offset >= 0);
	while (offset + 2 <= m_size)
	{
		if (r16(offset) == marker)
		{
			m_data[offset + 0] = value >> 8;
			m_data[offset + 1] = value&255;
			return offset + 2;
		}
		offset += 2;
	}
	printf("ERROR: STrinkler broken binary bootstraps!\n");
	return -1;
}

int	BinaryBlob::Patch32(int offset, u32 marker, u32 value)
{
	offset = Patch16(offset, marker >> 16, value >> 16);
	offset = Patch16(offset, marker&0xffff, value&0xffff);
	return offset;
}

void	BinaryBlob::Pad(int padLimit, const char* padText)
{
	if (m_size < padLimit)
	{
		printf("Padding output with %d bytes...\n", padLimit - m_size);
		if (NULL == padText)
		{
			srand(m_size);
			while (m_size < padLimit)
				w8(rand() & 255);
		}
		else
		{
			const int sl = strlen(padText);
			int p = 0;
			while (m_size < padLimit)
				w8(padText[(p++) % sl]);
		}
	}
	else
	{
		printf("Warning: Can't add padding data (no more space)\n");
	}
}