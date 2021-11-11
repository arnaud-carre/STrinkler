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
#include "STrinkler.h"

class BinaryBlob
{

public:
	BinaryBlob();
	~BinaryBlob();
	bool LoadFromFile(const char* sFilename);
	bool LoadFromW32(const u32* p, int count);
	int GetSize() const { return m_size; }
	u8* GetData() const { return m_data; }
	void	Reserve(int size);
	bool	SaveFile(const char* sFilename);

	bool	IsAtariExecutable() const;
	void	AtariCodeShrink();
	bool	AtariRelocParse(bool verbose);
	bool	IsRelocationTable() const { return m_relocSize > 0; }

	void	Append(const void* p, int size);
	void	AppendW16(const u16* p, int count);
	void	Align(int align);

	void	w32(u32 v);
	void	w16(u16 v);
	void	w8(u8 v);
	void	Release();

	int		GetTextSectionSize() const { return m_codeSize; }
	int		GetBssSectionSize() const { return m_bssSize; }

private:
	u32		r32(int offset) const;
	u16		r16(int offset) const;
	u8		r8(int offset) const;


	u8*		m_data;
	u32		m_size;
	u32		m_reserve;

	int		m_codeSize;
	int		m_bssSize;

	int*	m_relocTable;
	int		m_relocSize;
};
