#pragma once
#include "STrinkler.h"

class BinaryBlob
{

public:
	BinaryBlob();
	~BinaryBlob();
	bool LoadFromFile(const char* sFilename);
	bool LoadFromMemory(const void* data, int size);
	int GetSize() const { return m_size; }
	u8* GetData() const { return m_data; }

	bool	IsAtariExecutable() const;

private:
	void	Release();
	u32		r32(int offset) const;
	u16		r16(int offset) const;

	u8*		m_data;
	int		m_size;
};
