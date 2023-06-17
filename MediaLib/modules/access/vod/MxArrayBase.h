#ifndef _MxArrayBase_H_
#define _MxArrayBase_H_
#include <inttypes.h>


#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

typedef void (*ItemReleaseProc) (void* p_ref, void* p_obj);

// ****************************************************************
//                      MxArrayBase Class
// ****************************************************************
struct _MxArrayBase;

class  MxArrayBase
{
public:
				MxArrayBase(int32_t inTypeSize, int32_t inGrowCount = 10, int32_t inInitialAlloc = 0);
	virtual		~MxArrayBase();


	void		_SetName(char* inName);
	char*		_GetName();
	void		_PrintInfo();

	void		_Lock();
	void		_UnLock();

    void		_Clear(bool bLock = true);
	void		_SetMem(void* inT, int32_t inCount, bool bLock = true);

	void		_Append(void* inT, int32_t inCount = 1, bool bLock = true);
	void*		_GetAppendBuffer(int32_t inCount = 1, bool bLock = true);
	void		_Insert(int32_t inInsertPos, void* inT, int32_t inCount = 1, bool bLock = true);

	int32_t		_FetchAt(void* inT, bool bLock = true);

	void		_Remove(int32_t inIndex, int32_t inCount, bool bLock = true);
	void		_Remove(void* inT, bool bLock = true);
	void		_RemoveAll(bool bLock = true);

	void*		_GetBuffer(bool bLock = true);
	int32_t		_GetCount(bool bLock = true);
	void*		_Get(int32_t inIndex, bool bLock = true);

	void*		_GetHead(bool bLock = true);
	void*		_GetTail(bool bLock = true);

	void*		_OutBuffer(bool bLock = true);

	void		_ReduceExtra(bool bLock = true);
	
	void		_SetCount(int32_t inCount, bool bLock = true);

private:
	_MxArrayBase* m_pMembers;

	int32_t		mTypeSize;
	int32_t		mGrowCount;
	int32_t		mCount;
	int32_t		mMaxCount;
	char*	mBuf;
	vlc_mutex_t	mMutex;
};

// ****************************************************************
//                      MxPtrArrayBase Class
// ****************************************************************
struct _MxPtrArrayBase;

class MxPtrArrayBase
{
public:
				MxPtrArrayBase(int32_t inGrowCount = 10);
	virtual		~MxPtrArrayBase();
	
	void		_SetName(char* inName);
	char*		_GetName();
	void		_PrintInfo();

	void		_SetReleaseProc(ItemReleaseProc inProc, void* inRef);
	void		_Lock();
	void		_UnLock();
	
	void		_Clear(bool bLock = true);
	void		_SetMem(void** inT, int32_t inCount, bool bLock = true);

	void		_Append(void** inT, int32_t inCount = 1, bool bLock = true);
	void**		_GetAppendBuffer(int32_t inCount = 1, bool bLock = true);
	void		_Insert(int32_t inInsertPos, void** inT, int32_t inCount = 1, bool bLock = true);

	int32_t		_FetchAt(void* inT, bool bLock = true);

	void		_Remove(int32_t inIndex, int32_t inCount, bool bRelease = true, bool bLock = true);
	void		_Remove(void* inT, bool bRelease = true, bool bLock = true);
	void		_RemoveAll(bool bRelease = true, bool bLock = true);

	void**		_GetBuffer(bool bLock = true);
	int32_t		_GetCount(bool bLock = true);
	void**		_Get(int32_t inIndex, bool bLock = true);

	void**		_GetHead(bool bLock = true);
	void**		_GetTail(bool bLock = true);

	void**		_OutBuffer(bool bLock = true);

private:
	_MxPtrArrayBase* m_pMembers;

	int32_t		mGrowCount;
	int32_t		mCount;
	int32_t		mMaxCount;
	void**		mBuf;
	vlc_mutex_t	mMutex;

	ItemReleaseProc mReleaseProc;
	void*			mReleaseParam;
};


#endif

