#include <stdlib.h>
#include <string.h>

#include "MxArrayBase.h"

#	include	<malloc.h>
#	define	BlockMoveData(s,d,l)		memmove(d,s,l)	//MoveMemory(d,s,l)



struct _MxArrayBase
{
	char	name[1024];
	mtime_t	last_time;
};

// ****************************************************************
//                      MxArrayBase Class
// ****************************************************************
MxArrayBase::MxArrayBase(int32_t inTypeSize, int32_t inGrowCount, int32_t inInitialAlloc)
{
    m_pMembers = new _MxArrayBase;
	memset(m_pMembers, 0, sizeof(_MxArrayBase));
	mTypeSize = inTypeSize;
	mGrowCount = inGrowCount;
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;
	vlc_mutex_init(&mMutex);
	strcpy(m_pMembers->name, "");
	m_pMembers->last_time = mdate();

	if (inInitialAlloc)
	{
		mMaxCount = inInitialAlloc;
		char* newBuf = (char*)::malloc(mMaxCount*mTypeSize);
		mBuf = newBuf;
	}
}

MxArrayBase::~MxArrayBase()
{
	if (mBuf)
	{
		::free( mBuf);
	}

	vlc_mutex_destroy(&mMutex);

	delete (m_pMembers);
}


void
MxArrayBase::_SetName(char* inName)
{
	strcpy(m_pMembers->name, inName);
}

char*
MxArrayBase::_GetName()
{
	return m_pMembers->name;
}

void
MxArrayBase::_PrintInfo()
{
	if(m_pMembers->name[0] && mdate() - m_pMembers->last_time > 5 * CLOCK_FREQ)
	{
		m_pMembers->last_time = mdate();

	}
}

void MxArrayBase::_Lock()
{
    vlc_mutex_lock(&mMutex);
}

void MxArrayBase::_UnLock()
{
    vlc_mutex_unlock(&mMutex);
}

void MxArrayBase::_Clear(bool bLock)
{
	if(bLock)
		_Lock();

	if (mBuf)
	{
		::free( mBuf);
	}
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;

	if(bLock)
		_UnLock();
}

void MxArrayBase::_SetMem(void* inT, int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	if (mBuf)
	{
		::free( mBuf);
	}
	mBuf = NULL;
	mMaxCount = mCount = 0;

	if (inT != NULL && inCount > 0)
	{
		mBuf = (char*)inT;
		mMaxCount = mCount = inCount;
	}

	if(bLock)
		_UnLock();
}

void MxArrayBase::_Append(void* inT, int32_t inCount, bool bLock)
{
	if (inT == NULL)
		return;

	if(bLock)
		_Lock();

	int32_t	newCount;
	
	newCount = mCount + inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		char* newBuf = (char*)malloc(mMaxCount*mTypeSize);
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * mTypeSize);
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	BlockMoveData(inT, mBuf+mCount*mTypeSize, inCount*mTypeSize);
	mCount = newCount;

	if(bLock)
		_UnLock();
}

void MxArrayBase::_ReduceExtra(bool bLock)
{
	if(bLock)
		_Lock();

	//int32_t	oldMaxCount = mMaxCount;
	int32_t	newCount = (mCount / mGrowCount + 1) * mGrowCount;

	if (newCount < mMaxCount)
	{
		mMaxCount = newCount;
		char* newBuf = (char*)malloc(mMaxCount*mTypeSize);
		
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * mTypeSize);
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}

	if(bLock)
		_UnLock();
}

void* MxArrayBase::_GetAppendBuffer(int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t	newCount;
	//int32_t	oldMaxCount = mMaxCount;
	
	newCount = mCount + inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		char* newBuf = (char*)malloc(mMaxCount*mTypeSize);
		
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * mTypeSize);
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	void* outBuffer = mBuf+mCount*mTypeSize;
	mCount = newCount;

	if(bLock)
		_UnLock();

	return outBuffer;
}

void MxArrayBase::_SetCount(int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t	newCount;
	//int32_t	oldMaxCount = mMaxCount;
	
	newCount = inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		char* newBuf = (char*)malloc(mMaxCount*mTypeSize);
		
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * mTypeSize);
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	mCount = newCount;

	if(bLock)
		_UnLock();
}

int32_t MxArrayBase::_FetchAt(void* inT, bool bLock)
{
	int32_t index = -1;
	void*	p_item;
	
	if(bLock)
		_Lock();
	
	if (mCount > 0)
	{
		int32_t i;
		for (i = 0; i < mCount; i++)
		{
			p_item = mBuf + i * mTypeSize;
			if(memcmp(p_item, inT, mTypeSize) == 0)
			{
				index = i;
				break;
			}
		}
	}

	if(bLock)
		_UnLock();
	
	return index;
}

void MxArrayBase::_Insert(int32_t inInsertPos, void* inT, int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t	newCount;
	//int32_t	oldMaxCount = mMaxCount;
	
	newCount = mCount + inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		char* newBuf = (char*)malloc(mMaxCount*mTypeSize);
		
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * mTypeSize);
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	if (inInsertPos < mCount-1)
	{
		BlockMoveData(mBuf+(inInsertPos+1)*mTypeSize, mBuf+(inInsertPos+1+inCount)*mTypeSize, (mCount-1-inInsertPos)*mTypeSize);
	}
	BlockMoveData(inT, mBuf+(inInsertPos+1)*mTypeSize, inCount*mTypeSize);
	mCount = newCount;

	if(bLock)
		_UnLock();
}

void MxArrayBase::_Remove(int32_t inIndex, int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	if (inIndex < mCount)
	{
		if (inIndex + inCount > mCount)
			inCount = mCount - inIndex;
		
		if (inIndex + inCount < mCount)
		{
			BlockMoveData(mBuf+(inIndex+inCount)*mTypeSize, mBuf+inIndex*mTypeSize, (mCount - inIndex - inCount) * mTypeSize);
		}
		
		mCount -= inCount;
	}

	if(bLock)
		_UnLock();
}

void MxArrayBase::_Remove(void* inT, bool bLock)
{
	if(bLock)
		_Lock();
	
	int32_t index = _FetchAt(inT, false);
	if (index != -1)
		_Remove(index, 1, false);
	
	if(bLock)
		_UnLock();
}

void MxArrayBase::_RemoveAll(bool bLock)
{
	if(bLock)
		_Lock();

	_Remove(0, mCount, false);

	if(bLock)
		_UnLock();
}

void* MxArrayBase::_GetBuffer(bool bLock)
{
	if(bLock)
		_Lock();

	void* p_buf = mBuf;

	if(bLock)
		_UnLock();

	return p_buf;
}

int32_t MxArrayBase::_GetCount(bool bLock)
{
	if(bLock)
		_Lock();

	int32_t count = mCount;

	if(bLock)
		_UnLock();

	return count;
}

void* MxArrayBase::_Get(int32_t inIndex, bool bLock)
{
	if(bLock)
		_Lock();

	void* p_item = (inIndex < mCount) ? mBuf + inIndex * mTypeSize : NULL;

	if(bLock)
		_UnLock();

	return p_item;
}

void*
MxArrayBase::_GetHead(bool bLock)
{
	if(bLock)
		_Lock();

	void* p_item = (mCount != 0) ? mBuf : NULL;

	if(bLock)
		_UnLock();
	
	return p_item;
}

void*
MxArrayBase::_GetTail(bool bLock)
{
	if(bLock)
		_Lock();

	void* p_item = (mCount != 0) ? mBuf + (mCount-1) * mTypeSize : NULL;

	if(bLock)
		_UnLock();
	
	return p_item;
}

void* MxArrayBase::_OutBuffer(bool bLock)
{
	if(bLock)
		_Lock();

	void* outBuf = mBuf;
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;

	if(bLock)
		_UnLock();

	return outBuf;
}

// ****************************************************************
//                      MxPtrArrayBase Class
// ****************************************************************

struct _MxPtrArrayBase
{
	char	name[1024];
	mtime_t	last_time;
};

MxPtrArrayBase::MxPtrArrayBase(int32_t inGrowCount)
{
    m_pMembers = new _MxPtrArrayBase;
    memset(m_pMembers, 0, sizeof(_MxPtrArrayBase));
    
	mGrowCount = inGrowCount;
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;
	mReleaseProc = NULL;
	mReleaseParam = NULL;
    
    vlc_mutex_init(&mMutex);
}

MxPtrArrayBase::~MxPtrArrayBase()
{
    _Lock();
	if (mBuf)
	{
		if(mReleaseProc)
		{
			for (int32_t i=0; i<mCount; i++)
			{
				mReleaseProc(mReleaseParam, mBuf[i]);
			}
		}
		::free( mBuf);
	}
    _UnLock();

	vlc_mutex_destroy(&mMutex);

	delete (m_pMembers);
}

void
MxPtrArrayBase::_SetName(char* inName)
{
	strcpy(m_pMembers->name, inName);
}

char*
MxPtrArrayBase::_GetName()
{
	return m_pMembers->name;
}

void
MxPtrArrayBase::_PrintInfo()
{
	if(m_pMembers->name[0] && mdate() - m_pMembers->last_time > 1 * CLOCK_FREQ)
	{
		m_pMembers->last_time = mdate();
	}
}

void
MxPtrArrayBase::_SetReleaseProc(ItemReleaseProc inProc, void* inRef)
{
    _Lock();
	mReleaseProc = inProc;
	mReleaseParam = inRef;
    _UnLock();
}

void MxPtrArrayBase::_Lock()
{
    vlc_mutex_lock(&mMutex);
}

void MxPtrArrayBase::_UnLock()
{
    vlc_mutex_unlock(&mMutex);
}

void MxPtrArrayBase::_Clear(bool bLock)
{
	if(bLock)
		_Lock();

	if (mBuf)
	{
		if(mReleaseProc)
		{
			for (int32_t i=0; i<mCount; i++)
			{
				mReleaseProc(mReleaseParam, mBuf[i]);
			}
		}
		::free( mBuf);
	}
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;

	if(bLock)
		_UnLock();
}

void MxPtrArrayBase::_SetMem(void** inT, int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	if (mBuf)
	{
		if(mReleaseProc)
		{
			for (int32_t i=0; i<mCount; i++)
			{
				mReleaseProc(mReleaseParam, mBuf[i]);
			}
		}
		::free( mBuf);
	}
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;

	if (inT != NULL && inCount > 0)
	{
		mBuf = inT;
		mMaxCount = mCount = inCount;
	}

	if(bLock)
		_UnLock();
}

void MxPtrArrayBase::_Append(void** inT, int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t	newCount;
	
	newCount = mCount + inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		void** newBuf = (void**)::malloc(mMaxCount*sizeof(void*));
		
		memset(newBuf, 0, mMaxCount*sizeof(void*));
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * sizeof(void*));
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	BlockMoveData(inT, mBuf+mCount, inCount*sizeof(void*));
	mCount = newCount;

	if(bLock)
		_UnLock();
}

void** MxPtrArrayBase::_GetAppendBuffer(int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t	newCount;
	
	newCount = mCount + inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		void** newBuf = (void**)::malloc(mMaxCount*sizeof(void*));
		
		memset(newBuf, 0, mMaxCount*sizeof(void*));
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * sizeof(void*));
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	void**	outBuffer = mBuf+mCount;
	mCount = newCount;

	if(bLock)
		_UnLock();

	return outBuffer;
}

void MxPtrArrayBase::_Insert(int32_t inInsertPos, void** inT, int32_t inCount, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t	newCount;
	
	newCount = mCount + inCount;
	if (newCount > mMaxCount)
	{
		mMaxCount = (newCount / mGrowCount + 1) * mGrowCount;
		void** newBuf = (void**)::malloc(mMaxCount*sizeof(void*));
		
		memset(newBuf, 0, mMaxCount*sizeof(void*));
		
		if (mCount > 0)
		{
			BlockMoveData(mBuf, newBuf, mCount * sizeof(void*));
		}
		if (mBuf)
		{
			::free( mBuf);
		}
		
		mBuf = newBuf;
	}
	
	if (inInsertPos < mCount-1)
	{
		BlockMoveData(mBuf+(inInsertPos+1), mBuf+(inInsertPos+1+inCount), (mCount-1-inInsertPos)*sizeof(void*));
	}
	BlockMoveData(inT, mBuf+(inInsertPos+1), inCount*sizeof(void*));
	mCount = newCount;

	if(bLock)
		_UnLock();
}

int32_t MxPtrArrayBase::_FetchAt(void* inT, bool bLock)
{
	int32_t index = -1;

	if(bLock)
		_Lock();
	
	if (mCount > 0)
	{
		int32_t i;
		for (i = 0; i < mCount; i++)
		{
			if (mBuf[i] == inT)
			{
				index = i;
				break;
			}
		}
	}
	if(bLock)
		_UnLock();

	return index;
}

void MxPtrArrayBase::_Remove(int32_t inIndex, int32_t inCount, bool bRelease, bool bLock)
{
	if(bLock)
		_Lock();

	if (inIndex < mCount)
	{
		if (inIndex + inCount > mCount)
			inCount = mCount - inIndex;
		
		if (inCount > 0 && mReleaseProc && bRelease)
		{
			for (int32_t i=inIndex; i<inIndex+inCount; i++)
			{
				mReleaseProc(mReleaseParam, mBuf[i]);
			}
		}
		if (inIndex + inCount < mCount)
		{
			BlockMoveData(mBuf+(inIndex+inCount), mBuf+inIndex, (mCount - inIndex - inCount) * sizeof(void*));
		}
		
		mCount -= inCount;
		if (inCount > 0)
			memset(mBuf+mCount, 0, inCount*sizeof(void*));
	}

	if(bLock)
		_UnLock();
}

void MxPtrArrayBase::_Remove(void* inT, bool bRelease, bool bLock)
{
	if(bLock)
		_Lock();

	int32_t index = _FetchAt(inT, false);
	if (index != -1)
		_Remove(index, 1, bRelease, false);

	if(bLock)
		_UnLock();
}

void MxPtrArrayBase::_RemoveAll(bool bRelease, bool bLock)
{
	if(bLock)
		_Lock();
	
	_Remove(0, mCount, bRelease, false);
	
	if(bLock)
		_UnLock();
}

void** MxPtrArrayBase::_GetBuffer(bool bLock)
{
	if(bLock)
		_Lock();

	void** p_buf = mBuf;

	if(bLock)
		_UnLock();
	
	return p_buf;
}

int32_t MxPtrArrayBase::_GetCount(bool bLock)
{
	if(bLock)
		_Lock();

	int32_t count = mCount;

	if(bLock)
		_UnLock();
	
	return count;
}

void** MxPtrArrayBase::_Get(int32_t inIndex, bool bLock)
{
	if(bLock)
		_Lock();

	void** p_item = (inIndex < mCount) ? mBuf + inIndex : NULL;

	if(bLock)
		_UnLock();

	return p_item;
}

void** MxPtrArrayBase::_GetHead(bool bLock)
{
	if(bLock)
		_Lock();

	void** p_item = (mCount != 0) ? mBuf : NULL;

	if(bLock)
		_UnLock();
	
	return p_item;
}

void** MxPtrArrayBase::_GetTail(bool bLock)
{
	if(bLock)
		_Lock();

	void** p_item = (mCount != 0) ? mBuf + (mCount-1) : NULL;

	if(bLock)
		_UnLock();
	
	return p_item;
}

void** MxPtrArrayBase::_OutBuffer(bool bLock)
{
	if(bLock)
		_Lock();

	void** outBuf = mBuf;
	mCount = 0;
	mMaxCount = 0;
	mBuf = NULL;

	if(bLock)
		_UnLock();

	return outBuf;
}

