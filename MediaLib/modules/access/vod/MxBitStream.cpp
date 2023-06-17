#include "MxBitStream.h"


// ****************************************************************
//                      MxBitStream Class
// ****************************************************************

MxBitStream::MxBitStream(
	void*		pBuffer,
	int32_t		buffSize)
{
	m_BitPos	= 0;
	m_BuffSize	= buffSize;
	m_TotalBits	= buffSize * 8;
	m_Buff		= (uint8_t*)pBuffer;
}

MxBitStream::~MxBitStream()
{
}

uint8_t	MxBitStream::U8()
{
	int32_t		i_pos;
	int32_t		i_tail;
	uint8_t		i_data;
	uint8_t*	buff;
	
	if(m_BitPos + 8 > m_TotalBits)
		return 0;

	i_pos	= m_BitPos / 8;
	i_tail	= m_BitPos % 8;
	buff	= &m_Buff[i_pos];
	
	if(i_tail == 0)
	{
		i_data = *buff;
	}
	else
	{
		i_tail = 8 - i_tail;
		i_data = buff[0] & ((1 << i_tail) - 1);
		i_data <<= (8 - i_tail);
		i_data |= buff[1] >> i_tail;
	}
	
	m_BitPos += 8;
	
	return i_data;
}

uint8_t	MxBitStream::U8(uint8_t i_bits) // i_bits : [1, 8]
{
	int32_t		i_left, i_mask;
	uint8_t		i_data;
	uint8_t*	buff;
	
	if(m_BitPos + i_bits > m_TotalBits)
		return 0;

	i_left	= 8 - (m_BitPos % 8);
	buff	= &m_Buff[m_BitPos / 8];
	
	if(i_left >= i_bits)
	{
		i_mask = (1 << i_bits) - 1;
		i_data = (uint8_t)((buff[0] >> (i_left - i_bits)) & i_mask);
	}
	else
	{
		i_data = buff[0] & ((1 << i_left) - 1);
		i_data <<= (i_bits - i_left);
		i_data |= buff[1] >> (8 - (i_bits - i_left));
	}
	
	m_BitPos += i_bits;
	
	return i_data;
}

uint16_t MxBitStream::U16()
{
	uint16_t	i_data;
	
	i_data = U8() << 8;
	i_data |= U8();
	
	return i_data;
}

uint32_t MxBitStream::U24()
{
	uint32_t	i_data;
	
	i_data = U16() << 8;
	i_data |= U8();
	
	return i_data;
}

uint32_t MxBitStream::U32()
{
	uint32_t	i_data;
	
	i_data = U16() << 16;
	i_data |= U16();
	
	return i_data;
}

uint8_t	MxBitStream::U1()
{
	uint8_t	i_data;
	int32_t	i_tail;

	if(m_BitPos + 1 > m_TotalBits)
		return 0;
	
	i_tail = 8 - (m_BitPos % 8);
	i_data = (m_Buff[m_BitPos / 8] >> (i_tail - 1)) & 1;

	m_BitPos++;

	return i_data;
}

void MxBitStream::SkipBit(int32_t i_bit)
{
	m_BitPos = (m_BitPos + i_bit < m_TotalBits) ? m_BitPos + i_bit : m_TotalBits;
	m_BitPos = (m_BitPos <= 0) ? 0 : m_BitPos;
}

uint32_t MxBitStream::U(uint8_t i_bits, bool b_tell)
{
	int32_t		i_bitpos;
	uint32_t	i_data;
	
	if(b_tell)
	{
		i_bitpos = m_BitPos;
		i_data	 = read_bits(i_bits);
		m_BitPos = i_bitpos;
	}
	else
	{
		i_data	 = read_bits(i_bits);
	}
	
	return i_data;
}

uint32_t MxBitStream::GetPos()
{
	return (uint32_t)(m_BitPos / 8);
}

int32_t MxBitStream::GetBitPos()
{
	return m_BitPos;
}

void MxBitStream::SetPos(int32_t i_pos)
{
	m_BitPos = (i_pos < m_BuffSize) ? i_pos * 8 : m_TotalBits;
	if(m_BitPos < 0)
		m_BitPos = 0;
}

void MxBitStream::SetLast()
{
	m_BitPos = m_TotalBits;
}

int32_t MxBitStream::GetSize()
{
	return m_BuffSize;
}

void MxBitStream::ByteAlign()
{
	if(m_BitPos % 8)
	{
		m_BitPos /= 8;
		m_BitPos = (m_BitPos + 1) * 8;
	}
}

uint32_t MxBitStream::NextBytes(uint8_t i_bytes)
{
	int32_t		i, pos;
	uint8_t*	buff;
	uint32_t	i_data = 0;
	
	pos		= m_BitPos / 8;
	buff	= &m_Buff[pos];

	for(i = 0; i < i_bytes; i++)
	{
		i_data <<= 8;
		if(pos + i < m_BuffSize)
			i_data |= buff[i];
	}

	return i_data;
}

uint32_t MxBitStream::read_bits(int32_t bits)
{
	uint32_t	i_data = 0;
	
	for(int32_t i = 0; i < bits; i++)
	{
		i_data = (i_data << 1) | U1();
	}
	
	return i_data;
}

uint32_t MxBitStream::UEV()
{
	uint8_t		b;
	uint32_t	codeNum;
	int32_t		leadingZeroBits = -1;
	
	for(b = 0; !b;)
	{
		if(m_BitPos < m_TotalBits)
			b = U1();
		else
			b = 1;

		leadingZeroBits++;
	}
	
	codeNum = (1 << leadingZeroBits) - 1 + read_bits(leadingZeroBits);
	
	return codeNum;
}

int32_t MxBitStream::SEV()
{
	int32_t		i_data;
	uint32_t	i_data1;
	
	i_data1	= UEV();
	i_data	= i_data1 / 2;
	i_data	+= (i_data1 % 2);

	if((i_data1 % 2) == 0)
		i_data *= -1;

	return i_data;
}

bool MxBitStream::MoreData(int32_t i_bits)
{
	if(m_BitPos >= m_TotalBits || m_BitPos + i_bits > m_TotalBits)
		return false;
	return true;
}

uint8_t* MxBitStream::GetBuffer(int32_t i_start)
{
	if(i_start == -1)
		return &m_Buff[m_BitPos / 8];
	return &m_Buff[i_start];
}

bool MxBitStream::SearchCode32(uint32_t i_code, uint32_t i_mask, int32_t i_start_pos)
{
	int32_t		pos;
	uint32_t	code;
	
	if(i_start_pos == -1)
		ByteAlign();
	else
		SetPos(i_start_pos);

	pos	 = GetPos();
	code = (MoreData(32)) ? U32() : 0xFFFFFFFF;

	while(MoreData(8) && (code & i_mask) != i_code)
	{
		code <<= 8; code |= U8();
	}
	if((code & i_mask) == i_code)
	{
		SkipBit(-32);
		return true;
	}
	SetPos(pos);

	return false;
}

int32_t MxBitStream::RemainBits()
{
	return m_TotalBits - m_BitPos;//19_09_2016_alex
}


