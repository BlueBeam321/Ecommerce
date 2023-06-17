
#ifndef __CTaskScheduler_VOD_H__
#define __CTaskScheduler_VOD_H__

#include "SocketHelper.h"

typedef void TaskFunc(void* inClientData);
typedef void* TaskToken;


    class CTimeval
    {
    public:
        long seconds() const { return m_Tv.tv_sec; }
        long useconds() const { return m_Tv.tv_usec; }

        int operator>=(CTimeval const& inArg2) const;
        int operator<=(CTimeval const& inArg2) const { return inArg2 >= *this; }
        int operator<(CTimeval const& inArg2) const { return !(*this >= inArg2); }
        int operator>(CTimeval const& inArg2) const { return inArg2 < *this; }
        int operator==(CTimeval const& inArg2) const { return *this >= inArg2 && inArg2 >= *this; }
        int operator!=(CTimeval const& inArg2) const { return !(*this == inArg2); }

        void operator+=(class CDelayInterval const& inArg2);
        void operator-=(class CDelayInterval const& inArg2);
        // returns ZERO iff arg2 >= arg1

    protected:
        CTimeval(long inSeconds, long inUseconds)
        {
            m_Tv.tv_sec = inSeconds; m_Tv.tv_usec = inUseconds;
        }

    private:
        long& secs() { return (long&)m_Tv.tv_sec; }
        long& usecs() { return (long&)m_Tv.tv_usec; }

        struct timeval m_Tv;
    };

    inline CTimeval maxTimeval(CTimeval const& inArg1, CTimeval const& inArg2)
    {
        return inArg1 >= inArg2 ? inArg1 : inArg2;
    }

    inline CTimeval minTimeval(CTimeval const& inArg1, CTimeval const& inArg2)
    {
        return inArg1 <= inArg2 ? inArg1 : inArg2;
    }

    class CDelayInterval operator-(CTimeval const& inArg1, CTimeval const& inArg2);
    // returns ZERO iff arg2 >= arg1

    class CDelayInterval : public CTimeval
    {
    public:
        CDelayInterval(long seconds, long useconds)
            :CTimeval(seconds, useconds) {}
    };

    CDelayInterval operator*(short inArg1, CDelayInterval const& inArg2);

    extern CDelayInterval const ZERO;
    extern CDelayInterval const SECOND;
    CDelayInterval const MINUTE = 60 * SECOND;
    CDelayInterval const HOUR = 60 * MINUTE;
    CDelayInterval const DAY = 24 * HOUR;

    class CEventTime : public CTimeval
    {
    public:
        CEventTime(unsigned inSecondsSinceEpoch = 0, unsigned inUsecondsSinceEpoch = 0)
            // We use the Unix standard epoch: January 1, 1970
            : CTimeval(inSecondsSinceEpoch, inUsecondsSinceEpoch) {}
    };

    CEventTime CTimeNow();

    CDelayInterval TimeRemainingUntil(CEventTime const& inFutureEvent);
    // Returns ZERO if "futureEvent" has already occurred.

    //extern CEventTime const THE_END_OF_TIME;

    class CDelayQueueEntry
    {
    public:
        virtual ~CDelayQueueEntry();

        long token() { return m_Token; }

    protected: // abstract base class
        CDelayQueueEntry(CDelayInterval inDelay);

        virtual void handleTimeout();

    private:
        friend class CDelayQueue;
        CDelayQueueEntry* m_Next;
        CDelayQueueEntry* m_Prev;
        CDelayInterval m_DeltaTimeRemaining;

        long m_Token;
        static long sTokenCounter;
    };

    ///// CDelayQueue /////
    class CDelayQueue : public CDelayQueueEntry
    {
    public:
        CDelayQueue();
        virtual ~CDelayQueue();

        void addEntry(CDelayQueueEntry* inNewEntry); // returns a token for the entry
        void updateEntry(CDelayQueueEntry* inEntry, CDelayInterval inNewDelay);
        void updateEntry(long inTokenToFind, CDelayInterval inNewDelay);
        void removeEntry(CDelayQueueEntry* inEntry); // but doesn't delete it
        CDelayQueueEntry* removeEntry(long inTokenToFind); // but doesn't delete it

        CDelayInterval const& timeToNextAlarm();
        void handleAlarm();

    private:
        CDelayQueueEntry* head() { return m_Next; }
        CDelayQueueEntry* findEntryByToken(long inToken);
        void synchronize(); // bring the 'time remaining' fields up-to-date

        CEventTime m_LastSyncTime;
    };


    class CAlarmHandler : public CDelayQueueEntry
    {
    public:
        CAlarmHandler(TaskFunc* inProc, void* inClientData, CDelayInterval inTimeToDelay)
            : CDelayQueueEntry(inTimeToDelay), m_Proc(inProc), m_ClientData(inClientData)
        {
        }

    private: // redefined virtual functions
        virtual void handleTimeout()
        {
            (*m_Proc)(m_ClientData);
            CDelayQueueEntry::handleTimeout();
        }

    private:
        TaskFunc* m_Proc;
        void* m_ClientData;
    };

    class CUsageEnvironment;

    class CTaskScheduler
    {
    public:
        CTaskScheduler(CUsageEnvironment* inUsageEnvironment);
        virtual ~CTaskScheduler();

        static CTaskScheduler* Create(CUsageEnvironment* inEnv);
        void				Close();

        short SingleStep(unsigned inMaxDelayTime = 0);

        TaskToken scheduleDelayedTask(int inMicroseconds, TaskFunc* inProc, void* inClientData);
        void unscheduleDelayedTask(TaskToken& inPrevTask);
        void rescheduleDelayedTask(TaskToken& inTask, int inMicroseconds, TaskFunc* inProc, void* inClientData);

        CUsageEnvironment* env() { return m_UsageEnvironment; }

    protected:
        CUsageEnvironment* m_UsageEnvironment;

        CDelayQueue m_DelayQueue;
    };


#endif //__CTaskScheduler_VOD_H__