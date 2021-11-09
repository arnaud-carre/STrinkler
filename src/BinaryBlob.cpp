#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BinaryBlob.h"

BinaryBlob::BinaryBlob()
{
	m_data = NULL;
	m_size = -1;
}

void	BinaryBlob::Release()
{
	free(m_data);
	m_data = NULL;
	m_size = -1;
}

BinaryBlob::~BinaryBlob()
{
	Release();
}

bool	BinaryBlob::LoadFromFile(const char* sFilename)
{
	bool ret = false;
	Release();
	FILE* h = fopen(sFilename, "rb");
	if (h)
	{
		fseek(h, 0, SEEK_END);
		m_size = ftell(h);
		fseek(h, 0, SEEK_SET);
		m_data = (u8*)malloc(m_size);
		fread(m_data, 1, m_size, h);
		fclose(h);
		ret = true;
	}
	return ret;
}

bool	BinaryBlob::LoadFromMemory(const void* data, int size)
{
	Release();
	assert(size > 0);
	m_size = size;
	m_data = (u8*)malloc(size);
	memcpy(m_data, data, size);
	return true;
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

bool	BinaryBlob::IsAtariExecutable() const
{
	if (r16(0) != 0x601a)
		return false;

	if (r32(18))
		return false;			// reserved long, should be 0

	u32 endOffset = 0x1c;
	endOffset += r32(2);		// text section
	endOffset += r32(6);	// data section
	endOffset += r32(14);	// symbol table size

	if (endOffset > m_size)
		return false;

	return true;
}
