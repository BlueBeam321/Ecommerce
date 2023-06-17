
#ifndef __CQueue_VOD_H__
#define __CQueue_VOD_H__


    class CQueue;

    class CQueueElem
    {
    public:
        CQueueElem(void* enclosingObject = 0) : m_Next(0), m_Prev(0), m_Queue(0),
            m_EnclosingObject(enclosingObject) {}
        virtual ~CQueueElem() { }

        bool IsMember(const CQueue& inQueue) { return bool(&inQueue == m_Queue); }
        bool IsMemberOfAnyQueue() 	{ return bool(m_Queue != 0); }
        void* GetEnclosingObject() 	{ return m_EnclosingObject; }
        void SetEnclosingObject(void* inObj) { m_EnclosingObject = inObj; }

        CQueueElem* Next() { return m_Next; }
        CQueueElem* Prev() { return m_Prev; }
        CQueue*		InQueue()	{ return m_Queue; }
        inline void	  Remove();

    private:

        CQueueElem*		m_Next;
        CQueueElem*		m_Prev;
        CQueue *		 m_Queue;
        void*			  m_EnclosingObject;

        friend class 	CQueue;
    };

    class CQueue
    {
    public:
        CQueue();
        virtual ~CQueue();

        void			  EnQueue(CQueueElem* object);
        CQueueElem*		DeQueue();

        CQueueElem* 	GetHead() { if (m_Length > 0) return m_Sentinel.m_Prev; return 0; }
        CQueueElem* 	GetTail() { if (m_Length > 0) return m_Sentinel.m_Next; return 0; }
        unsigned int 			  GetLength() { return m_Length; }

        void 			  Remove(CQueueElem* object);

    protected:

        CQueueElem		m_Sentinel;
        unsigned int 			  m_Length;
    };

    class CQueueIter
    {
    public:
        CQueueIter(CQueue* inQueue) : m_QueueP(inQueue), m_CurrentElemP(inQueue->GetHead()) {}
        CQueueIter(CQueue* inQueue, CQueueElem* startElemP) : m_QueueP(inQueue)
        {
            if (startElemP)
            {
                m_CurrentElemP = startElemP;

            }
            else
                m_CurrentElemP = 0;
        }
        ~CQueueIter() {}

        void			Reset() { m_CurrentElemP = m_QueueP->GetHead(); }

        CQueueElem*	GetCurrent() { return m_CurrentElemP; }
        void			Next();

        bool			IsDone() { return m_CurrentElemP == 0; }

    private:
        CQueue*		m_QueueP;
        CQueueElem*	m_CurrentElemP;
    };

    void CQueueElem::Remove()
    {
        if (m_Queue != 0)
            m_Queue->Remove(this);
    }


#endif // __CQueue_VOD_H__