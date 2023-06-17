#include <stdlib.h>
#include <string.h>

#include "MxBuffer.h"

// ****************************************************************
//                      MxBuffer Class
// ****************************************************************

struct _MxBuffer
{
	int32_t			cur_pos;
	int32_t			limit_size;
	int32_t			grow_size;
	int64_t			ref_con;
	int64_t			ref_con1;

	int32_t			buf_size;
	char*   		p_buffer;
}; 

MxBuffer::MxBuffer()
{
    m_pMembers = new _MxBuffer;
	m_pMembers->grow_size = 1024;
	m_pMembers->buf_size = 0;
	m_pMembers->p_buffer = NULL;
	
	Init();
}

MxBuffer::~MxBuffer()
{
    if (m_pMembers->p_buffer)
	    free(m_pMembers->p_buffer);
	m_pMembers->p_buffer = NULL;

	delete (m_pMembers);
}

void
MxBuffer::Init()
{
	m_pMembers->limit_size = -1;
	m_pMembers->cur_pos = 0;
	m_pMembers->ref_con = 0;
	m_pMembers->ref_con1 = 0;
}

void
MxBuffer::PutData(
	char*	inBuffer,
	int32_t		inSize,
	bool		bToLast)
{
	int32_t length;

	if(m_pMembers->limit_size == -1)
	{
		// auto grow
		if(m_pMembers->cur_pos + inSize > m_pMembers->buf_size)
		{
			m_pMembers->buf_size = m_pMembers->cur_pos + inSize + m_pMembers->grow_size;
			char* p_buf = (char*)malloc(m_pMembers->buf_size);

			if(m_pMembers->p_buffer)
			{
                memcpy(p_buf, m_pMembers->p_buffer, m_pMembers->cur_pos);
                free(m_pMembers->p_buffer);
			}
			m_pMembers->p_buffer = p_buf;
		}
		if(bToLast)
		{
			memcpy(m_pMembers->p_buffer + m_pMembers->cur_pos, inBuffer, inSize);
		}
		else
		{
			memmove(m_pMembers->p_buffer + inSize, m_pMembers->p_buffer, m_pMembers->cur_pos); // FIXME : how to speed up
			memcpy(m_pMembers->p_buffer, inBuffer, inSize);
		}
		m_pMembers->cur_pos += inSize;
	}
	else if(m_pMembers->limit_size > 0)
	{
		// fixed size
		if(m_pMembers->limit_size)
			length = (m_pMembers->cur_pos + inSize < m_pMembers->limit_size) ? inSize : m_pMembers->limit_size - m_pMembers->cur_pos;
		else
			length = (m_pMembers->cur_pos + inSize < m_pMembers->buf_size) ? inSize : 0;
		
		if(length)
		{
			memcpy(m_pMembers->p_buffer + m_pMembers->cur_pos, inBuffer, length);
			m_pMembers->cur_pos += length;
		}
	}
}

void
MxBuffer::RemoveData(
	int32_t		inSize,
	bool		bFromLast)
{
	if(inSize < 0)
		return;
	
	if(inSize >= m_pMembers->cur_pos)
		inSize = m_pMembers->cur_pos;
	
	if(bFromLast == false)
	{
		if(m_pMembers->cur_pos - inSize > 0)
		{
			memcpy(m_pMembers->p_buffer, m_pMembers->p_buffer + inSize, m_pMembers->cur_pos - inSize);
			m_pMembers->cur_pos -= inSize;
		}
		else
		{
			m_pMembers->cur_pos = 0;
		}
	}
	else
	{
		m_pMembers->cur_pos = inSize;
	}
}

void
MxBuffer::CopyData(
	char*	outBuffer,
	int32_t		inSize,
	int32_t*	outSize,
	int32_t		inOffset)
{
	int32_t i_copied = 0;

	if(inSize > 0 && inOffset >= 0 && inOffset < m_pMembers->cur_pos)
	{
		if(inOffset + inSize >= m_pMembers->cur_pos)
			inSize = m_pMembers->cur_pos - inOffset;

		memcpy(outBuffer, m_pMembers->p_buffer + inOffset, inSize);
        i_copied = inSize;
	}

	if(outSize)
		*outSize = i_copied;
}

int32_t
MxBuffer::GetDataSize()
{
	return m_pMembers->cur_pos;
}

void
MxBuffer::SetDataSize(
	int32_t		inSize)
{
	if(inSize > m_pMembers->buf_size)
		inSize = m_pMembers->buf_size;

	m_pMembers->cur_pos = inSize;
}

void
MxBuffer::ClearData()
{
	m_pMembers->cur_pos = 0;
}

// --------------------------------------------------------

char*
MxBuffer::GetBuffer()
{
	return m_pMembers->p_buffer;
}

int32_t
MxBuffer::GetBufferSize()
{
	return m_pMembers->buf_size;
}

void
MxBuffer::IncreaseBuffer(
	int32_t		inIncSize)
{
	if(m_pMembers->limit_size == -1)
	{
		m_pMembers->buf_size += inIncSize;
		char* p_buf = (char*)malloc(m_pMembers->buf_size);
		
		if(m_pMembers->p_buffer)
		{
            memcpy(p_buf, m_pMembers->p_buffer, m_pMembers->cur_pos);
            free(m_pMembers->p_buffer);
		}
		m_pMembers->p_buffer = p_buf;
	}
}

// --------------------------------------------------------

int32_t
MxBuffer::GetLimitSize()
{
	return m_pMembers->limit_size;
}

void
MxBuffer::SetLimitSize(
	int32_t		inSize)
{
	if(inSize != -1 && m_pMembers->buf_size < inSize)
	{
		m_pMembers->buf_size = inSize + m_pMembers->grow_size;
		free(m_pMembers->p_buffer);
		m_pMembers->p_buffer = (char*)malloc(m_pMembers->buf_size);
	}
	
	m_pMembers->limit_size = inSize;
	m_pMembers->cur_pos = 0;
}

bool
MxBuffer::IsFull()
{
	if(m_pMembers->limit_size > 0 && m_pMembers->cur_pos >= m_pMembers->limit_size)
		return true;
	return false;
}

// --------------------------------------------------------

void
MxBuffer::SetGrowSize(
	int32_t		inGrowSize)
{
	m_pMembers->grow_size = inGrowSize;
}

void
MxBuffer::SetRefCon(
	int64_t		inRefCon)
{
	m_pMembers->ref_con = inRefCon;
}

int64_t
MxBuffer::GetRefCon()
{
	return m_pMembers->ref_con;
}

void
MxBuffer::SetRefCon1(
	int64_t		inRefCon1)
{
	m_pMembers->ref_con1 = inRefCon1;
}
 
int64_t
MxBuffer::GetRefCon1()
{
	return m_pMembers->ref_con1;
}

// ****************************************************************
//                      MxGrowBuffer Class
// ****************************************************************

struct _MxGrowBuffer
{
	char*		ptr;
	int32_t		size;
	int32_t		real_size;
	int32_t		block;
}; 

MxGrowBuffer::MxGrowBuffer(
	int32_t		inBlockSize)
{
    m_pMembers = new _MxGrowBuffer;
	
	m_pMembers->ptr = NULL;
	m_pMembers->size = 0;
	m_pMembers->real_size = 0;
	m_pMembers->block = inBlockSize;
}

MxGrowBuffer::~MxGrowBuffer()
{
	free(m_pMembers->ptr);
	delete (m_pMembers);
}

char*
MxGrowBuffer::Alloc(
	int32_t		inNewSize)
{
	m_pMembers->real_size = inNewSize;
	inNewSize = (inNewSize + m_pMembers->block - 1) / m_pMembers->block * m_pMembers->block;
	if(m_pMembers->size < inNewSize)
	{
		free(m_pMembers->ptr);

		m_pMembers->ptr = (char*)malloc(inNewSize);
		m_pMembers->size = inNewSize;
	}
	return m_pMembers->ptr;
}

void
MxGrowBuffer::Clear()
{
	free(m_pMembers->ptr);
	m_pMembers->size = 0;
	m_pMembers->real_size = 0;
}

int32_t
MxGrowBuffer::GetRealSize()
{
	return m_pMembers->real_size;
}

int32_t
MxGrowBuffer::GetCapacity()
{
	return m_pMembers->size;
}

char*
MxGrowBuffer::GetBuffer()
{
	return m_pMembers->ptr;
}

