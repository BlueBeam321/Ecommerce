#ifndef _MX_BUFFER_H_
#define _MX_BUFFER_H_
#include <inttypes.h>
// ****************************************************************
//                      MxBuffer Class
// ****************************************************************
struct _MxBuffer;

class MxBuffer
{
public:
					MxBuffer();

	virtual			~MxBuffer();
	
	void			Init();
	
	void			PutData(
						char*	inBuffer,
						int32_t		inSize,
						bool		bToLast = true);

	void			RemoveData(
						int32_t		inSize,
						bool		bFromLast = false);

	void			CopyData(
						char*	outBuffer,
						int32_t		inSize,
						int32_t*	outSize,
						int32_t		inOffset = 0);
	
	int32_t			GetDataSize();
	
	void			SetDataSize(
						int32_t		inSize);

	void			ClearData();
	
	// --------------------------------------------------------
	
	char*		GetBuffer();
	
	int32_t			GetBufferSize();
	
	void			IncreaseBuffer(
						int32_t		inIncSize);

	// --------------------------------------------------------

	int32_t			GetLimitSize();

	void			SetLimitSize(
						int32_t		inSize = -1);

	bool			IsFull();

	// --------------------------------------------------------
	
	void			SetGrowSize(
						int32_t		inGrowSize);
	
	void			SetRefCon(
						int64_t		inRefCon);
	
	int64_t			GetRefCon();
	
	void			SetRefCon1(
						int64_t		inRefCon1);
	
	int64_t			GetRefCon1();

private:
	_MxBuffer*		m_pMembers;
};

// ****************************************************************
//                      MxGrowBuffer Class
// ****************************************************************
struct _MxGrowBuffer;

class MxGrowBuffer 
{
public:
	
					MxGrowBuffer(
						int32_t		inBlockSize = 1024);

	virtual			~MxGrowBuffer();

	char*			Alloc(
						int32_t		inNewSize);

	void			Clear();

	char*			GetBuffer();
	int32_t			GetCapacity();
	int32_t			GetRealSize();
	
private:
	_MxGrowBuffer*	m_pMembers;
};

#endif
