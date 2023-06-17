#ifndef _MX_BLOCK_QUEUE_H_
#define _MX_BLOCK_QUEUE_H_

// ****************************************************************
//                      MxBlockQueue Class
// ****************************************************************

struct _MxBlockQueue;
class MxBlockQueue
{
public:
					MxBlockQueue(
						int32_t		inBlockSize,
						int32_t		inFreeSize = 1);

	virtual			~MxBlockQueue();
	
	void			Push(
						void*		inBuffer,
						int32_t		inSize);
	
	int32_t			Pop(
						void*		outBuffer,
						int32_t		inMaxSize);
	
	void			Clear();
	
	int32_t			GetSize();

	bool			ReadFirst(
						char*	outBuffer,
						int32_t		inCount);

	bool			ReadLast(
						char*	outBuffer,
						int32_t		inCount);

private:
	_MxBlockQueue*	m_pMembers;
};


#endif
