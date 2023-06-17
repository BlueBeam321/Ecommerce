
#include "CQueue.h"


    CQueue::CQueue() : m_Length(0)
    {
        m_Sentinel.m_Next = &m_Sentinel;
        m_Sentinel.m_Prev = &m_Sentinel;
    }

    CQueue::~CQueue()
    {
        CQueueElem* theElem;
        char* theTmp;

        while ((theElem = DeQueue()) != 0)
        {
            theTmp = (char*)theElem->GetEnclosingObject();
            if (theTmp != 0)
                delete[] theTmp;
            delete theElem;
        }
    }

    void
        CQueue::EnQueue(CQueueElem* inElem)
    {
        if (inElem == 0) return;

        if (inElem->m_Queue == this)
            return;
        if (inElem->m_Queue != 0) return;

        inElem->m_Next = m_Sentinel.m_Next;
        inElem->m_Prev = &m_Sentinel;
        inElem->m_Queue = this;
        m_Sentinel.m_Next->m_Prev = inElem;
        m_Sentinel.m_Next = inElem;
        m_Length++;
    }

    CQueueElem*
        CQueue::DeQueue()
    {
        if (m_Length > 0)
        {
            CQueueElem* elem = m_Sentinel.m_Prev;

            if (m_Sentinel.m_Prev == &m_Sentinel) return 0;

            elem->m_Prev->m_Next = &m_Sentinel;
            m_Sentinel.m_Prev = elem->m_Prev;
            elem->m_Queue = 0;
            m_Length--;
            return elem;
        }
        else
            return 0;
    }

    void
        CQueue::Remove(CQueueElem* inElem)
    {

        if (inElem == 0) return;
        if (inElem == &m_Sentinel) return;

        if (inElem->m_Queue == this)
        {
            inElem->m_Next->m_Prev = inElem->m_Prev;
            inElem->m_Prev->m_Next = inElem->m_Next;
            inElem->m_Queue = 0;
            m_Length--;
        }
    }

    void
        CQueueIter::Next()
    {
        if (m_CurrentElemP == m_QueueP->GetTail())
            m_CurrentElemP = 0;
        else
            m_CurrentElemP = m_CurrentElemP->Prev();
    }
