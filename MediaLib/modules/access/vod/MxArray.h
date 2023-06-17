#ifndef _MxArray_H_
#define _MxArray_H_

#include "MxArrayBase.h"

// ****************************************************************
//                      MxArray Class
// ****************************************************************

template <class T> class MxArray : protected MxArrayBase
{
public:
			MxArray(int32_t inGrowCount = 10, int32_t inInitialAlloc = 0);
	virtual	~MxArray();

	void	SetName(char* inName);
	char*	GetName();
	void	PrintInfo();
	
	void	Lock();
	void	UnLock();

	void	Clear(bool bLock = true);
	void	SetMem(T* inT, int32_t inCount, bool bLock = true);

	void	Append(T* inT, int32_t inCount = 1, bool bLock = true);
	T*		GetAppendBuffer(int32_t inCount = 1, bool bLock = true);
	void	Insert(int32_t inInsertPos, T* inT, int32_t inCount = 1, bool bLock = true);

	int32_t	FetchAt(T& inT, bool bLock = true);

	void	Remove(int32_t inIndex, int32_t inCount, bool bLock = true);
	void	Remove(T& inT, bool bLock = true);
	void	RemoveAll(bool bLock = true);

	T*		GetBuffer(bool bLock = true){return (T*)_GetBuffer(bLock);}
	int32_t	GetCount(bool bLock = true){return _GetCount(bLock);}
	T*		Get(int32_t inIndex, bool bLock = true);

	T*		GetHead(bool bLock = true){return (T*)_GetHead(bLock);}
	T*		GetTail(bool bLock = true){return (T*)_GetTail(bLock);}

	T*		OutBuffer(bool bLock = true);
	void	Add(const T& inT, bool bLock = true);
	
	void	ReduceExtra(bool bLock = true);
	
	void	SetCount(int32_t inCount, bool bLock = true);
};

template <class T>
MxArray<T>::MxArray(int32_t inGrowCount, int32_t inInitialAlloc) : MxArrayBase(sizeof(T), inGrowCount, inInitialAlloc)
{
}

template <class T>
MxArray<T>::~MxArray()
{
}

template <class T>
void MxArray<T>::SetName(char* inName)
{
	_SetName(inName);
}

template <class T>
char* MxArray<T>::GetName()
{
	return _GetName();
}

template <class T>
void MxArray<T>::PrintInfo()
{
	_PrintInfo();
}

template <class T>
void MxArray<T>::Lock()
{
	_Lock();
}

template <class T>
void MxArray<T>::UnLock()
{
	_UnLock();
}

template <class T>
void MxArray<T>::Clear(bool bLock)
{
	_Clear(bLock);
}

template <class T>
void MxArray<T>::SetMem(T* inT, int32_t inCount, bool bLock)
{
	_SetMem(inT, inCount, bLock);
}

template <class T>
void MxArray<T>::Add(const T& inT, bool bLock)
{
	Append((T*)&inT, 1, bLock);
}

template <class T>
void MxArray<T>::Append(T* inT, int32_t inCount, bool bLock)
{
	_Append(inT, inCount, bLock);
}

template <class T>
void MxArray<T>::ReduceExtra(bool bLock)
{
	_ReduceExtra(bLock);
}

template <class T>
T* MxArray<T>::GetAppendBuffer(int32_t inCount, bool bLock)
{
	return (T*)_GetAppendBuffer(inCount, bLock);
}

template <class T>
void MxArray<T>::SetCount(int32_t inCount, bool bLock)
{
	_SetCount(inCount, bLock);
}

template <class T>
void MxArray<T>::Insert(int32_t inInsertPos, T* inT, int32_t inCount, bool bLock)
{
	_Insert(inInsertPos, inT, inCount, bLock);
}

template <class T>
int32_t MxArray<T>::FetchAt(T& inT, bool bLock)
{
	return _FetchAt(&inT, bLock);
}

template <class T>
void MxArray<T>::Remove(int32_t inIndex, int32_t inCount, bool bLock)
{
	_Remove(inIndex, inCount, bLock);
}

template <class T>
void MxArray<T>::RemoveAll(bool bLock)
{
	_RemoveAll(bLock);
}

template <class T>
void MxArray<T>::Remove(T& inT, bool bLock)
{
	_Remove(&inT, bLock);
}

template <class T>
T* MxArray<T>::Get(int32_t inIndex, bool bLock)
{
	return (T*)_Get(inIndex, bLock);
}

template <class T>
T* MxArray<T>::OutBuffer(bool bLock)
{
	return (T*)_OutBuffer(bLock);
}


// ****************************************************************
//                      MxPtrArray Class
// ****************************************************************

template <class T> class MxPtrArray : protected MxPtrArrayBase
{
public:
				MxPtrArray();
	virtual		~MxPtrArray();
	
	void		SetName(char* inName);
	char*		GetName();
	void		PrintInfo();

	void		SetReleaseProc(ItemReleaseProc inProc, void* inRef);
	void		Lock();
	void		UnLock();
	
	void		Clear(bool bLock = true);
	void		SetMem(T** inT, int32_t inCount, bool bLock = true);

	void		Append(T** inT, int32_t inCount = 1, bool bLock = true);
	T**			GetAppendBuffer(int32_t inCount = 1, bool bLock = true);
	void		Insert(int32_t inInsertPos, T** inT, int32_t inCount = 1, bool bLock = true);

	int32_t		FetchAt(T* inT, bool bLock = true);

	void		Remove(int32_t inIndex, int32_t inCount, bool bRelease = true, bool bLock = true);
	void		Remove(T* inT, bool bRelease = true, bool bLock = true);
	void		RemoveAll(bool bRelease = true, bool bLock = true);

	T**			GetBuffer(bool bLock = true){return (T**)_GetBuffer(bLock);}
	int32_t		GetCount(bool bLock = true){return _GetCount(bLock);}
	T**			Get(int32_t inIndex, bool bLock = true);

	T**			GetHead(bool bLock = true){return (T**)_GetHead(bLock);}
	T**			GetTail(bool bLock = true){return (T**)_GetTail(bLock);}

	T**			OutBuffer(bool bLock = true);

	void		Add(T* inT, bool bLock = true);
};

template <class T>
MxPtrArray<T>::MxPtrArray() : MxPtrArrayBase()
{
}

template <class T>
MxPtrArray<T>::~MxPtrArray()
{
}

template <class T>
void MxPtrArray<T>::SetName(char* inName)
{
	_SetName(inName);
}

template <class T>
char* MxPtrArray<T>::GetName()
{
	return _GetName();
}

template <class T>
void MxPtrArray<T>::PrintInfo()
{
	_PrintInfo();
}

template <class T>
void MxPtrArray<T>::SetReleaseProc(ItemReleaseProc inProc, void* inRef)
{
	_SetReleaseProc(inProc, inRef);
}

template <class T>
void MxPtrArray<T>::Lock()
{
	_Lock();
}

template <class T>
void MxPtrArray<T>::UnLock()
{
	_UnLock();
}

template <class T>
void MxPtrArray<T>::Clear(bool bLock)
{
	_Clear(bLock);
}

template <class T>
void MxPtrArray<T>::SetMem(T** inT, int32_t inCount, bool bLock)
{
	_SetMem((void**)inT, inCount, bLock);
}

template <class T>
void MxPtrArray<T>::Add(T* inT, bool bLock)
{
	_Append((void**)&inT, 1, bLock);
}

template <class T>
void MxPtrArray<T>::Append(T** inT, int32_t inCount, bool bLock)
{
	_Append((void**)inT, inCount, bLock);
}

template <class T>
T** MxPtrArray<T>::GetAppendBuffer(int32_t inCount, bool bLock)
{
	return (T**)_GetAppendBuffer(inCount, bLock);
}

template <class T>
void MxPtrArray<T>::Insert(int32_t inInsertPos, T** inT, int32_t inCount, bool bLock)
{
	_Insert(inInsertPos, (void**)inT, inCount, bLock);
}

template <class T>
int32_t MxPtrArray<T>::FetchAt(T* inT, bool bLock)
{
	return _FetchAt((void*)inT, bLock);
}

template <class T>
void MxPtrArray<T>::Remove(int32_t inIndex, int32_t inCount, bool bRelease, bool bLock)
{
	_Remove(inIndex, inCount, bRelease, bLock);
}

template <class T>
void MxPtrArray<T>::Remove(T* inT, bool bRelease, bool bLock)
{
	_Remove((void*)inT, bRelease, bLock);
}

template <class T>
void MxPtrArray<T>::RemoveAll(bool bRelease, bool bLock)
{
	_RemoveAll(bRelease, bLock);
}

template <class T>
T** MxPtrArray<T>::Get(int32_t inIndex, bool bLock)
{
	return (T**)_Get(inIndex, bLock);
}

template <class T>
T** MxPtrArray<T>::OutBuffer(bool bLock)
{
	return (T**)_OutBuffer(bLock);
}

#endif
