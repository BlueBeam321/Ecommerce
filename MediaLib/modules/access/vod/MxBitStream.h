#ifndef _MxBitStream_h_
#define _MxBitStream_h_

#include <inttypes.h>


// ****************************************************************
//                      MxBitStream Class
// ****************************************************************

class MxBitStream
{
public:
				MxBitStream(void* pBuffer,int32_t buffSize);
				~MxBitStream();
	
	uint32_t	U(uint8_t i_bits, bool b_tell = false);
	uint32_t	NextBytes(uint8_t i_bytes);
	uint8_t		U8();
	uint8_t		U8(uint8_t i_bits);
	uint16_t	U16();
	uint32_t	U24();
	uint32_t	U32();
	uint8_t		U1();
	uint32_t	UEV();
	int32_t		SEV();
	void		SkipBit(int32_t i_bit);
	bool		MoreData(int32_t i_bits = 0);
	uint32_t	GetPos();
	int32_t		GetBitPos();

	void		ByteAlign();
	void		SetPos(int32_t i_pos);
	void		SetLast();
	int32_t		GetSize();
	uint8_t*	GetBuffer(int32_t i_start = 0);
	bool		SearchCode32(uint32_t i_code, uint32_t i_mask = 0xFFFFFFFF, int32_t i_start_pos = -1);
	int32_t		RemainBits();

protected:
	uint32_t	read_bits(int32_t bits);
	
	uint8_t*  	m_Buff;
	int32_t		m_BuffSize;
	int32_t		m_BitPos;
	int32_t		m_TotalBits;
};


#endif
