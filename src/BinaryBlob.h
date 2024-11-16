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
	void	Pad(int padLimit, const char* padText);

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

	int		GetRelocCount() const { return m_relocSize; }
	int		GetRelocOffset(int i) const { return m_relocTable[i]; }

	int		Patch16(int offset, u16 marker, u16 value);
	int		Patch32(int offset, u32 marker, u32 value);

private:
	u32		r32(int offset) const;
	u16		r16(int offset) const;
	u8		r8(int offset) const;


	u8*		m_data;
	int		m_size;
	int		m_reserve;

	int		m_codeSize;
	int		m_bssSize;

	int*	m_relocTable;
	int		m_relocSize;
};
