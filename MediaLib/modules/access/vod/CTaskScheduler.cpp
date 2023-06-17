#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_mtime.h>

#include "CTaskScheduler.h"
#include "CUsageEnvironment.h"


    static const int MILLION = 1000000;

    ///// Timeval /////
    int
        CTimeval::operator>=(const CTimeval& inArg2) const
    {
        return seconds() > inArg2.seconds()
            || (seconds() == inArg2.seconds()
            && useconds() >= inArg2.useconds());
    }

    void
        CTimeval::operator+=(const CDelayInterval& inArg2)
    {
        secs() += inArg2.seconds();
        usecs() += inArg2.useconds();
        if (usecs() >= MILLION)
        {
            usecs() -= MILLION;
            ++secs();
        }
    }

    void
        CTimeval::operator-=(const CDelayInterval& inArg2)
    {
        secs() -= inArg2.seconds();
        usecs() -= inArg2.useconds();
        if (usecs() < 0)
        {
            usecs() += MILLION;
            --secs();
        }
        if (secs() < 0)
            secs() = usecs() = 0;
    }

    CDelayInterval operator-(const CTimeval& inArg1, const CTimeval& inArg2)
    {
        long secs = inArg1.seconds() - inArg2.seconds();
        long usecs = inArg1.useconds() - inArg2.useconds();

        if (usecs < 0)
        {
            usecs += MILLION;
            --secs;
        }
        if (secs < 0)
            return ZERO;
        else
            return CDelayInterval(secs, usecs);
    }

    ///// DelayInterval /////
    CDelayInterval operator*(short inArg1, const CDelayInterval& inArg2)
    {
        long result_seconds = inArg1*inArg2.seconds();
        long result_useconds = inArg1*inArg2.useconds();

        long carry = result_useconds / MILLION;
        result_useconds -= carry*MILLION;
        result_seconds += carry;

        return CDelayInterval(result_seconds, result_useconds);
    }

#ifndef INT_MAX
#define INT_MAX	0x7FFFFFFF
#endif

    const CDelayInterval ZERO(0, 0);
    const CDelayInterval SECOND(1, 0);
    const CDelayInterval ETERNITY(INT_MAX, MILLION - 1);


    ///// CDelayQueueEntry /////
    long CDelayQueueEntry::sTokenCounter = 0;

    CDelayQueueEntry::CDelayQueueEntry(CDelayInterval inDelay)
        : m_DeltaTimeRemaining(inDelay)
    {
        m_Next = m_Prev = this;
        m_Token = ++sTokenCounter;
    }

    CDelayQueueEntry::~CDelayQueueEntry()
    {
    }

    void
        CDelayQueueEntry::handleTimeout()
    {
        delete this;
    }

    ///// CDelayQueue /////
    CDelayQueue::CDelayQueue()
        : CDelayQueueEntry(ETERNITY)
    {
        m_LastSyncTime = CTimeNow();
    }

    CDelayQueue::~CDelayQueue()
    {
        while (m_Next != this) removeEntry(m_Next);
    }

    void
        CDelayQueue::addEntry(CDelayQueueEntry* inNewEntry)
    {
        synchronize();

        CDelayQueueEntry* cur = head();
        while (inNewEntry->m_DeltaTimeRemaining >= cur->m_DeltaTimeRemaining)
        {
            inNewEntry->m_DeltaTimeRemaining -= cur->m_DeltaTimeRemaining;
            cur = cur->m_Next;
        }

        cur->m_DeltaTimeRemaining -= inNewEntry->m_DeltaTimeRemaining;

        // Add "inNewEntry" to the queue, just before "cur":
        inNewEntry->m_Next = cur;
        inNewEntry->m_Prev = cur->m_Prev;
        cur->m_Prev = inNewEntry->m_Prev->m_Next = inNewEntry;
    }

    void
        CDelayQueue::updateEntry(CDelayQueueEntry* inEntry, CDelayInterval inNewDelay)
    {
        if (inEntry == NULL) return;

        removeEntry(inEntry);
        inEntry->m_DeltaTimeRemaining = inNewDelay;
        addEntry(inEntry);
    }

    void
        CDelayQueue::updateEntry(long inTokenToFind, CDelayInterval inNewDelay)
    {
        CDelayQueueEntry* theEntry = findEntryByToken(inTokenToFind);
        updateEntry(theEntry, inNewDelay);
    }

    void
        CDelayQueue::removeEntry(CDelayQueueEntry* inEntry)
    {
        if (inEntry == NULL || inEntry->m_Next == NULL) return;

        inEntry->m_Next->m_DeltaTimeRemaining += inEntry->m_DeltaTimeRemaining;
        inEntry->m_Prev->m_Next = inEntry->m_Next;
        inEntry->m_Next->m_Prev = inEntry->m_Prev;
        inEntry->m_Next = inEntry->m_Prev = NULL;
        // in case we should try to remove it again
    }

    CDelayQueueEntry*
        CDelayQueue::removeEntry(long inTokenToFind)
    {
        CDelayQueueEntry* theEntry = findEntryByToken(inTokenToFind);
        removeEntry(theEntry);
        return theEntry;
    }

    CDelayInterval const&
        CDelayQueue::timeToNextAlarm()
    {
        if (head()->m_DeltaTimeRemaining == ZERO) return ZERO; // a common case

        synchronize();
        return head()->m_DeltaTimeRemaining;
    }

    void
        CDelayQueue::handleAlarm()
    {
        if (head()->m_DeltaTimeRemaining != ZERO) synchronize();

        if (head()->m_DeltaTimeRemaining == ZERO)
        {
            // This event is due to be handled:
            CDelayQueueEntry* toRemove = head();
            removeEntry(toRemove); // do this first, in case handler accesses queue

            toRemove->handleTimeout();
        }
    }

    CDelayQueueEntry*
        CDelayQueue::findEntryByToken(long inTokenToFind)
    {
        CDelayQueueEntry* theCur = head();
        while (theCur != this)
        {
            if (theCur->token() == inTokenToFind) return theCur;
            theCur = theCur->m_Next;
        }

        return NULL;
    }

    void
        CDelayQueue::synchronize()
    {
        // First, figure out how much time has elapsed since the last sync:
        CEventTime timeNow = CTimeNow();
        CDelayInterval timeSinceLastSync = timeNow - m_LastSyncTime;
        m_LastSyncTime = timeNow;

        // Then, adjust the delay queue for any entries whose time is up:
        CDelayQueueEntry* curEntry = head();
        while (timeSinceLastSync >= curEntry->m_DeltaTimeRemaining)
        {
            timeSinceLastSync -= curEntry->m_DeltaTimeRemaining;
            curEntry->m_DeltaTimeRemaining = ZERO;
            curEntry = curEntry->m_Next;
        }
        curEntry->m_DeltaTimeRemaining -= timeSinceLastSync;
    }

    ///// CEventTime /////
    CEventTime CTimeNow()
    {
        struct timeval tvNow;

        gettimeofday(&tvNow, NULL);

        return CEventTime(tvNow.tv_sec, tvNow.tv_usec);
    }

    CDelayInterval TimeRemainingUntil(const CEventTime& inFutureEvent)
    {
        return inFutureEvent - CTimeNow();
    }

    //const CEventTime THE_END_OF_TIME(INT_MAX);

    ///////////////////////////CTaskScheduler//////////////////////////
    CTaskScheduler*
        CTaskScheduler::Create(CUsageEnvironment* inEnv)
    {
        return new CTaskScheduler(inEnv);
    }

    void
        CTaskScheduler::Close()
    {
        delete this;
    }

    CTaskScheduler::CTaskScheduler(CUsageEnvironment* inEnv)
        : m_UsageEnvironment(inEnv)
    {
    }

    CTaskScheduler::~CTaskScheduler()
    {
    }

    short
        CTaskScheduler::SingleStep(unsigned inMaxDelayTime)
    {
        CDelayInterval const& timeToDelay = m_DelayQueue.timeToNextAlarm();
        struct timeval tv_timeToDelay;

        tv_timeToDelay.tv_sec = timeToDelay.seconds();
        tv_timeToDelay.tv_usec = timeToDelay.useconds();

        const long MAX_TV_SEC = MILLION;
        if (tv_timeToDelay.tv_sec > MAX_TV_SEC)
        {
            tv_timeToDelay.tv_sec = MAX_TV_SEC;
        }

        if (inMaxDelayTime > 0 &&
            (tv_timeToDelay.tv_sec > (long)inMaxDelayTime / MILLION ||
            (tv_timeToDelay.tv_sec == (long)inMaxDelayTime / MILLION &&
            tv_timeToDelay.tv_usec > (long)inMaxDelayTime%MILLION)))
        {
            tv_timeToDelay.tv_sec = inMaxDelayTime / MILLION;
            tv_timeToDelay.tv_usec = inMaxDelayTime%MILLION;
        }

        int mSecondsToGo = tv_timeToDelay.tv_sec * 1000 + tv_timeToDelay.tv_usec / 1000;
        if (mSecondsToGo > 0)
            msleep(mSecondsToGo * 1000);

        m_DelayQueue.handleAlarm();

        return  0;
    }

    TaskToken
        CTaskScheduler::scheduleDelayedTask(int inMicroseconds, TaskFunc* inProc, void* inClientData)
    {
        if (inMicroseconds < 0) inMicroseconds = 0;
        CDelayInterval timeToDelay(inMicroseconds / 1000000, inMicroseconds % 1000000);
        CAlarmHandler* alarmHandler = new CAlarmHandler(inProc, inClientData, timeToDelay);
        m_DelayQueue.addEntry(alarmHandler);

        return (void*)(alarmHandler->token());
    }

    void
        CTaskScheduler::unscheduleDelayedTask(TaskToken& inPrevTask)
    {
        CDelayQueueEntry* alarmHandler = m_DelayQueue.removeEntry((long)inPrevTask);
        inPrevTask = NULL;
        delete alarmHandler;
    }

    void
        CTaskScheduler::rescheduleDelayedTask(TaskToken& inTask, int inMicroseconds, TaskFunc* inProc, void* inClientData)
    {
        unscheduleDelayedTask(inTask);
        inTask = scheduleDelayedTask(inMicroseconds, inProc, inClientData);
    }
