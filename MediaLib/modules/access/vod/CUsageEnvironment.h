
#ifndef __CUsageEnvironment_VOD_H__
#define __CUsageEnvironment_VOD_H__

#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
	#include <pthread.h>
#endif

#include "VODNetLib.h"
#include "CQueue.h"

#define RESULT_MSG_BUFFER_MAX 1000

    class CUsageEnvironment
    {
    public:
        static CUsageEnvironment* Create(HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, void* inHookParams = 0);

        CUsageEnvironment(HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, void* inHookParams);
        virtual ~CUsageEnvironment();

        void				  Close();
        CQueue& GetEventQueue() { return m_EventQueue; }

        void	EnqueueEvent(short inEventKind, long inParams = 0);
        short	DequeueEvent(VODNet_Event* outEvent, unsigned long inTimeoutMiliSecs = 0);

        typedef const char* MsgString;
        void setResultMsg(long inErrCode, MsgString msg);
        void setResultMsg(long inErrCode, MsgString msg1, MsgString msg2);
        void setResultMsg(long inErrCode, MsgString msg1, MsgString msg2, MsgString msg3);
        void setResultMsgData(long inErrCode, MsgString msg1, long inMsgSize);

        char* GetLastError(long& outErrCode);

        virtual CUsageEnvironment& operator<<(char const* str);
        virtual CUsageEnvironment& operator<<(int i);
        virtual CUsageEnvironment& operator<<(unsigned u);
        virtual CUsageEnvironment& operator<<(double d);
        virtual CUsageEnvironment& operator<<(void* p);

        void* m_LiveMediaPriv;
        bool		IsStopingFlag();
        long		m_bStreamStopFlag;

    private:
        void reset();
        virtual void appendToResultMsg(MsgString msg);

        char		m_ResultMsgBuffer[RESULT_MSG_BUFFER_MAX];//ðkðÔóÅõjõã Eöð 
        unsigned	m_CurBufferSize;
        unsigned	m_BufferMaxSize;
        long		m_ErrorCode;

        HookGetVODError m_HookGetVODErrorFunc;
        HookIsStoping		m_StopingFunc;
        void*				m_HookParams;

#ifdef _WIN32
        HANDLE  m_hMutex;
#else
        pthread_mutex_t	m_hMutex;
#endif
        CQueue	m_EventQueue;
    };

#endif // __CUsageEnvironment_VOD_H__
