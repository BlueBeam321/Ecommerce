#include <inttypes.h>

#include "MxBlockQueue.h"
#include "MxArray.h"


// ****************************************************************
//                      _MxBlock struct
// ****************************************************************

struct _MxBlock
{
	char*	p_buf;
	int32_t		i_buf_size;

	int32_t		i_head;
	int32_t		i_tail;
	int32_t		i_free_size;
	
	_MxBlock(int32_t inSize, int32_t inFreeSize)
	{
		i_head =
		i_tail = 0;
		i_buf_size = inSize;
		i_free_size = inFreeSize;
		//p_buf = (char*)::malloc(i_buf_size + i_free_size);
		p_buf = (char*)malloc(i_buf_size + i_free_size);
	}
	
	~_MxBlock()
	{
		//if(p_buf)
		//	::free(p_buf);
		free(p_buf);
		p_buf = NULL;
	}
	
	int32_t Push(char* inBuffer, int32_t inSize)
	{
		int32_t copysize = GetLeftSize();

		if(copysize > inSize)
			copysize = inSize;
		
		int32_t	newTail = i_tail + copysize;

		if(newTail > i_buf_size + i_free_size)
		{
			int32_t	size = (i_buf_size + i_free_size) - i_tail;

			::memcpy(p_buf + i_tail, inBuffer, size);
			::memcpy(p_buf, inBuffer + size, copysize - size);
		}
		else
		{
			::memcpy(p_buf + i_tail, inBuffer, copysize);
		}

		i_tail = newTail;
		if(i_tail >= i_buf_size + i_free_size)
			i_tail -= (i_buf_size + i_free_size);
		
		return copysize;
	}
	
	int32_t	Pop(char* inBuffer, int32_t inSize)
	{
		int32_t	copysize = GetSize();

		if(copysize > inSize)
			copysize = inSize;
		
		if(copysize > 0)
		{
			int32_t	newHead = i_head + copysize;

			if(newHead > i_buf_size + i_free_size)
			{
				int32_t	size = (i_buf_size + i_free_size) - i_head;

				if(inBuffer)
				{
					::memcpy(inBuffer, p_buf + i_head, size);
					::memcpy(inBuffer + size, p_buf, copysize - size);
				}
			}
			else
			{
				if(inBuffer)
				{
					::memcpy(inBuffer, p_buf + i_head, copysize);
				}
			}

			i_head = newHead;
			if(i_head >= i_buf_size + i_free_size)
				i_head -= (i_buf_size + i_free_size);
		}
		
		return copysize;
	}
	
	int32_t	GetSize()
	{
		int32_t size;

		if(i_tail < i_head)
			size = i_buf_size + i_free_size - i_head + i_tail;
		else
			size = i_tail - i_head;

		return size;
	}
	
	int32_t	GetLeftSize()
	{
		return i_buf_size - GetSize();
	}

	int32_t Read(int32_t inOffset, char* inBuffer, int32_t inSize)
	{
		if (inOffset + inSize > GetSize())
			return 0;
		
		int32_t pos = i_head + inOffset;

		if (pos >= i_buf_size + i_free_size)
			pos -= i_buf_size + i_free_size;
		
		int32_t end = pos + inSize;

		if (end > i_buf_size + i_free_size)
		{
			int32_t size = (i_buf_size + i_free_size) - pos;

			if (inBuffer)
			{
				memcpy(inBuffer, p_buf + pos, size);
				memcpy(inBuffer + size, p_buf, inSize - size);
			}
		}
		else
		{
			if (inBuffer)
			{
				memcpy(inBuffer, p_buf + pos, inSize);
			}
		}

		return inSize;
	}
};

// ****************************************************************
//                      MxBlockQueue class
// ****************************************************************

struct _MxBlockQueue
{
	int32_t	i_block_size;
	int32_t	i_total_size;
	int32_t	i_free_size;

	MxPtrArray<_MxBlock>* p_list;
};

void _MxBlockQueueRelease(void* inParam, void* inObject)
{
	_MxBlock* p_block = (_MxBlock*)inObject;

	if(p_block)
		delete p_block;
}

MxBlockQueue::MxBlockQueue(
	int32_t		inBlockSize,
	int32_t		inFreeSize)
{
    m_pMembers = new _MxBlockQueue;
    memset(m_pMembers, 0, sizeof(*m_pMembers));
	m_pMembers->i_block_size = inBlockSize;
	m_pMembers->i_free_size = inFreeSize;
	m_pMembers->i_total_size = 0;

	m_pMembers->p_list = new MxPtrArray<_MxBlock>;
	{
		m_pMembers->p_list->SetReleaseProc(_MxBlockQueueRelease, NULL);
	}
}

MxBlockQueue::~MxBlockQueue()
{
	delete (m_pMembers->p_list);
	delete (m_pMembers);
}

void
MxBlockQueue::Push(
	void*		inBuffer,
	int32_t		inSize)
{
	_MxBlock* block;
	int32_t copysize;

	while(inSize > 0)
	{
		if(m_pMembers->p_list->GetCount() == 0)
		{
			block = new _MxBlock(m_pMembers->i_block_size, m_pMembers->i_free_size);
			m_pMembers->p_list->Add(block);
		}
		else
		{
			block = *(m_pMembers->p_list->GetTail());
		}

		if(block->GetLeftSize() == 0)
		{
			block = new _MxBlock(m_pMembers->i_block_size, m_pMembers->i_free_size);
			m_pMembers->p_list->Add(block);
		}

		copysize = block->Push((char*)inBuffer, inSize);

		inBuffer = (char*)inBuffer + copysize;
		inSize -= copysize;
		m_pMembers->i_total_size += copysize;
	}
}

int32_t
MxBlockQueue::Pop(
	void*		outBuffer,
	int32_t		inMaxSize)
{
	_MxBlock* block;
	int32_t copysize, size;

	size = 0;
	if(inMaxSize > m_pMembers->i_total_size)
		inMaxSize = m_pMembers->i_total_size;
	
	while(inMaxSize > 0)
	{
		if (m_pMembers->p_list->GetCount() == 0)
			break;

		block = *(m_pMembers->p_list->GetHead());

		copysize = block->Pop((char*)outBuffer, inMaxSize);

		outBuffer = (char*)outBuffer + copysize;
		inMaxSize -= copysize;
		m_pMembers->i_total_size -= copysize;

		size += copysize;

		if (block->GetSize() == 0 && m_pMembers->p_list->GetCount() > 1)
			m_pMembers->p_list->Remove(0, 1);
	}

	return size;
}

void
MxBlockQueue::Clear()
{
	m_pMembers->p_list->RemoveAll();
	m_pMembers->i_total_size = 0;
}

int32_t
MxBlockQueue::GetSize()
{
	return m_pMembers->i_total_size;
}

bool
MxBlockQueue::ReadFirst(
	char*	outBuffer,
	int32_t		inSize)
{
	if (m_pMembers->i_total_size < inSize)
		return false;
	
	_MxBlock* block;
	int32_t i, block_count;
	int32_t block_size, copy_size;
	
	block_count = m_pMembers->p_list->GetCount();
	for (i = 0; inSize > 0 && i < block_count; i++)
	{
		block = *(m_pMembers->p_list->Get(i));
		block_size = block->GetSize();
		
		copy_size = inSize;
		if (copy_size > block_size)
			copy_size = block_size;
		
		block->Read(0, outBuffer, copy_size);
		outBuffer += copy_size;
		inSize -= copy_size;
	}
	
	return (inSize == 0);
}

bool
MxBlockQueue::ReadLast(
	char*	outBuffer,
	int32_t		inSize)
{
	if (m_pMembers->i_total_size < inSize)
		return false;

	outBuffer += inSize;
	
	_MxBlock* block;
	int32_t i, block_count;
	int32_t block_size, copy_size;

	block_count = m_pMembers->p_list->GetCount();

	for (i = block_count - 1; inSize > 0 && i >= 0; i--)
	{
		block = *(m_pMembers->p_list->Get(i));
		block_size = block->GetSize();
		
		copy_size = inSize;
		if (copy_size > block_size)
			copy_size = block_size;
		
		block->Read(block_size - copy_size, outBuffer - copy_size, copy_size);
		outBuffer -= copy_size;
		inSize -= copy_size;
	}
	
	return (inSize == 0);
}

