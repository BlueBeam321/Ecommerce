
#include "CUsageEnvironment.h"

#include "SocketHelper.h"
#include "ErrorCode.h"
#include "CMedia.h"


    CUsageEnvironment*
        CUsageEnvironment::Create(HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, void* inHookParams)
    {
        return new CUsageEnvironment(inHookGetVODErrorFunc, inStopingFunc, inHookParams);
    }

    void
        CUsageEnvironment::Close()
    {
        if (m_LiveMediaPriv == NULL)
            delete this;
        else
        {
            setResultMsg(11000115, "m_LiveMediaPriv!=NULL Error!");
            return;
        }
    }

    CUsageEnvironment::CUsageEnvironment(HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, void* inHookParams)
        : m_LiveMediaPriv(NULL), m_BufferMaxSize(RESULT_MSG_BUFFER_MAX)
    {
        m_HookGetVODErrorFunc = inHookGetVODErrorFunc;
        m_StopingFunc = inStopingFunc;
        m_bStreamStopFlag = 0;
#ifdef _WIN32
        m_hMutex = ::CreateMutex(NULL, false, NULL);
#else
        pthread_mutex_init(&m_hMutex,NULL);
#endif

        reset();

        m_ErrorCode = 0;

        m_HookParams = inHookParams;

        long theErrCode = 0;
        long theAppErrCode = 0;
        if ((theErrCode = initializeWinsockIfNecessary()) != 0)
        {
            AnalysisSocketErrorCode(theErrCode, theAppErrCode);
            setResultMsg(theAppErrCode, "Failed to initialize 'winsock': ");
            return;
        }
    }

    CUsageEnvironment::~CUsageEnvironment()
    {
#ifdef _WIN32
        ::CloseHandle(m_hMutex);
#else
        pthread_mutex_destroy(&m_hMutex);
#endif

        if (m_LiveMediaPriv != NULL)
            delete (C_Tables*)m_LiveMediaPriv;
    }

    void
        CUsageEnvironment::EnqueueEvent(short inEventKind, long inParams)
    {
        VODNet_Event* theEvent = NULL;

#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        pthread_mutex_lock(&m_hMutex);
#endif

        theEvent = (VODNet_Event*)new char[sizeof(VODNet_Event)];
        theEvent->eventKind = inEventKind;
        theEvent->eventTime = 0;
        theEvent->eventParams = inParams;

        CQueueElem* theQueueElem = new CQueueElem(theEvent);

        m_EventQueue.EnQueue(theQueueElem);

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        pthread_mutex_unlock( &m_hMutex );
#endif
    }

    short CUsageEnvironment::DequeueEvent(VODNet_Event* outEvent, unsigned long inTimeoutMiliSecs)
    {
        if (outEvent == NULL) return -1;

#ifdef _WIN32
        DWORD theResult = ::WaitForSingleObject(m_hMutex, inTimeoutMiliSecs);
#else
        pthread_mutex_lock(&m_hMutex);
#endif

#ifdef _WIN32
        if (theResult == WAIT_OBJECT_0)
#endif
        {
            CQueueElem* theQueueElem = NULL;
            theQueueElem = m_EventQueue.DeQueue();
            if (theQueueElem == NULL)
            {
#ifdef _WIN32
                ::ReleaseMutex(m_hMutex);
#else
                pthread_mutex_unlock( &m_hMutex );
#endif
                return -1;
            }

            VODNet_Event* theEvent = (VODNet_Event*)theQueueElem->GetEnclosingObject();
            if (theEvent == NULL)
            {
#ifdef _WIN32
                ::ReleaseMutex(m_hMutex);
#else
                pthread_mutex_unlock( &m_hMutex );
#endif
                return -1;
            }
            outEvent->eventKind = theEvent->eventKind;
            outEvent->eventTime = theEvent->eventTime;
            outEvent->eventParams = theEvent->eventParams;
            delete[] theEvent;
            delete theQueueElem;
        }
#ifdef _WIN32
        else if (theResult == WAIT_TIMEOUT)
        {
            ::ReleaseMutex(m_hMutex);
            return -1;
        }
        else
        {
            ::ReleaseMutex(m_hMutex);
            return -1;
        }
#endif

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        pthread_mutex_unlock( &m_hMutex );
#endif
        return 0;
    }

    void CUsageEnvironment::reset()
    {
        m_CurBufferSize = 0;
        m_ResultMsgBuffer[m_CurBufferSize] = '\0';
        m_ErrorCode = 0;
    }

    void CUsageEnvironment::setResultMsg(long inErrCode, MsgString msg)
    {
        reset();
        appendToResultMsg(msg);
        m_ErrorCode = inErrCode;
        (*m_HookGetVODErrorFunc)("VODNetLib", inErrCode, m_ResultMsgBuffer, strlen(m_ResultMsgBuffer), m_HookParams);
    }

    void CUsageEnvironment::setResultMsg(long inErrCode, MsgString msg1, MsgString msg2)
    {
        reset();
        appendToResultMsg(msg1);
        appendToResultMsg(msg2);
        m_ErrorCode = inErrCode;
        (*m_HookGetVODErrorFunc)("VODNetLib", inErrCode, m_ResultMsgBuffer, strlen(m_ResultMsgBuffer), m_HookParams);
    }

    void
        CUsageEnvironment::setResultMsg(long inErrCode, MsgString msg1, MsgString msg2, MsgString msg3)
    {
        reset();
        appendToResultMsg(msg1);
        appendToResultMsg(msg2);
        appendToResultMsg(msg3);
        m_ErrorCode = inErrCode;
        (*m_HookGetVODErrorFunc)("VODNetLib", inErrCode, m_ResultMsgBuffer, strlen(m_ResultMsgBuffer), m_HookParams);
    }

    void
        CUsageEnvironment::setResultMsgData(long inErrCode, MsgString msg1, long inMsgSize)
    {
        //(*m_HookGetVODErrorFunc)("VODNetLib",inErrCode, msg1, inMsgSize, m_HookParams);
    }

    void
        CUsageEnvironment::appendToResultMsg(MsgString msg)
    {
        char* curPtr = &m_ResultMsgBuffer[m_CurBufferSize];
        unsigned spaceAvailable = m_BufferMaxSize - m_CurBufferSize;
        unsigned msgLength = strlen(msg);

        if (msgLength > spaceAvailable - 1)
        {
            msgLength = spaceAvailable - 1;
        }

        memmove(curPtr, (char*)msg, msgLength);
        m_CurBufferSize += msgLength;
        m_ResultMsgBuffer[m_CurBufferSize] = '\0';
    }

    CUsageEnvironment&
        CUsageEnvironment::operator<<(char const* /*str*/)
    {
        return *this;
    }

    CUsageEnvironment&
        CUsageEnvironment::operator<<(int i)
    {
        char theIstr[100];
        sprintf(theIstr, "%d", i);
        return *this;
    }

    CUsageEnvironment&
        CUsageEnvironment::operator<<(unsigned u)
    {
        char theIstr[100];
        sprintf(theIstr, "%u", u);
        return *this;
    }

    CUsageEnvironment&
        CUsageEnvironment::operator<<(double d)
    {
        char theIstr[100];
        sprintf(theIstr, "%f", d);
        return *this;
    }

    CUsageEnvironment&
        CUsageEnvironment::operator<<(void* p)
    {
        char theIstr[100];
        sprintf(theIstr, "%p", p);
        return *this;
    }

    char*
        CUsageEnvironment::GetLastError(long& outErrCode)
    {
        outErrCode = m_ErrorCode;
        return m_ResultMsgBuffer;
    }

    bool
        CUsageEnvironment::IsStopingFlag()
    {
        if (m_StopingFunc == NULL)
            return false;
        return m_StopingFunc(m_HookParams);
    }
