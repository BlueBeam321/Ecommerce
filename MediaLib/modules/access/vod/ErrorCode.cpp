
#include "ErrorCode.h"
#include "SocketHelper.h"

#ifndef _WIN32
#include "MxSocketConfig.h"
#endif


    VODERRORTBL MmErrors[] =
    {
        /*Object*/
        { 11000108, 0, "Object is not a RTCP instance!" },
        { 11000113, 0, "m_LiveMediaPriv!=NULL Error!" },
        { 11000115, 0, "create clientsession object failed!" },

        /*RTSP Command*/
        { 11000209, 0, "Client port number unknown!" },
        { 11000210, 0, "\"Session:\" header is missing in the response!" },
        { 11000211, 0, "URL's form is not valid!" },
        { 11000212, 0, "URL's too long!" },
        { 11000213, 0, "No port number follows ':'!" },
        { 11000214, 0, "Bad port number!" },
        { 11000215, 0, "We received a response not ending with <CR><LF><CR><LF>!" },
        { 11000216, 0, "No response code in line!" },
        { 11000220, 0, "Send Request RTSP failed!" },
        /*Operation*/
        { 11000401, 0, "This sink is already being played!" },
        { 11000402, 0, "CMediaSink::startPlaying(): source is not compatible!" },
        { 11000403, 0, "No RTSP session is currently in progress!" },
        { 11000404, 0, "getNextFrame(): attempting to read more than once at the same time!" },
        { 11000405, 0, "The total received frame size exceeds the client's buffer size!" },
        { 11000406, 0, "theClient->m_InPointer == theClient->m_OutPointer!" },
        { 11000407, 0, "Sleep()!" },
        { 11000408, 0, "EnqueueEvent due to LineFailed!" },
        { 11000409, 0, "Failed to get source port!" },
        { 11000410, 0, "Can not File Open!" },
        /*RTSP Commnad Processing*/
        { 11000503, 0, "Cannot handle response!" },
        { 11000504, 0, "RTSP response was truncated!" },
        { 11000505, 0, "BAD SDP lines!" },
        /*Socket*/
        { 11000301, WSANOTINITIALISED, "WSANOTINITIALISED!" },
        { 11000302, WSAENETDOWN, "WSAENETDOWN!" },
        { 11000303, WSAEACCES, "WSAEACCES!" },
        { 11000304, WSAEINPROGRESS, "WSAEINPROGRESS!" },
        { 11000305, WSAEFAULT, "WSAEFAULT!" },
        { 11000306, WSAENETRESET, "WSAENETRESET!" },
        { 11000307, WSAENOBUFS, "WSAENOBUFS!" },
        { 11000308, WSAENOTCONN, "WSAENOTCONN!" },
        { 11000309, WSAEWOULDBLOCK, "WSAEWOULDBLOCK!" },
        { 11000310, WSAEMSGSIZE, "WSAEMSGSIZE!" },
        { 11000311, WSAEHOSTUNREACH, "WSAEHOSTUNREACH!" },
        { 11000312, WSAEINVAL, "WSAEINVAL!" },
        { 11000313, WSAETIMEDOUT, "WSAETIMEDOUT!" },
        { 11000314, WSAENOTSOCK, "WSAENOTSOCK!" },
        { 11000315, WSAEAFNOSUPPORT, "WSAEAFNOSUPPORT!" },
        { 11000316, WSAEMFILE, "WSAEMFILE!" },
        { 11000317, WSAEPROTONOSUPPORT, "WSAEPROTONOSUPPORT!" },
        { 11000318, WSAEPROTOTYPE, "WSAEPROTOTYPE!" },
        { 11000319, WSAESOCKTNOSUPPORT, "WSAESOCKTNOSUPPORT!" },
        { 11000320, 0, "CIoSocket write failed!" },
        { 11000321, 0, "CIoSocket read failed!" },
        { 11000322, 0, "Failed to read response!" },
        { 11000323, 0, "CTaskScheduler::SingleStep(): select() fails!" },
        { 11000324, 0, "Udp Socket bind() error!" },
        { 11000325, 0, "Udp setsockopt(SO_REUSEADDR) error!" },
        { 11000326, 0, "Failed to initialize socket!" },
        { 11000327, 0, "Unable to create datagram socket!" },
        { 11000328, 0, "Unable to create stream socket!" },
        { 11000329, 0, "Tcp setsockopt(SO_REUSEADDR) error!" },
        { 11000330, 0, "Tcp Socket bind() error!" },
        { 11000331, 0, "Failed to make non-blocking!" },
        { 11000332, 0, "select() error!" },
        { 11000333, 0, "recvfrom() error!" },
        { 11000334, 0, "sendto() error!" },
        { 11000335, 0, "getBufferSize() error!" },
        { 11000336, 0, "setBufferTo() error!" },
        { 11000337, 0, "getsockname() error!" },
        { 11000338, 0, "connect() error!" },
        { 11000339, 0, "Client RTP Port is not event!" },
        { 11000340, 0, "long wouldblock error at recvfrom!" },
        /* Memory*/
        { 11000601, 0, "GlobalAlloc Error!" },
        { 11000602, 0, "m_KnownMembers or m_InBuf Alloc Error!" },
        { 11000603, 0, "Frame Buf Size Error!" },
        /*Authentication*/
        { 11000701, 0, "No Authentication Data!" },
        { 11000702, 0, "No Private Key Data!" },
        { 11000703, 0, "Client's Authentication Data Format Error!" },
        { 11000704, 0, "No Acceptable Client!" },
        /* TS Stream*/
        { 11000801, 0, "No Transport Stream sync byte in data!" },
        { 11000802, 0, "Transport Stream sync byte not at first!" },
        { 11000803, 0, "Missing sync byte!" },
        { 11000804, 0, "Input Source Closed!" },
        { 11000805, 0, "Packet Loss Occured!" },
        { 11000806, 0, "Invalid releaseUsedPacket!" },
        /* Server Operation*/
        { 11000901, 0, "No Media Data!" },
        { 11000902, 0, "Over Users!" },
        { 11000903, 0, "Internal Server Error!" },
        /*UnKnown*/
        { 11001001, 0, "UnKnown Error!" },
        { 0, 0, NULL },
    };

    bool AnalysisSocketErrorStr(int inSysErrCode, long& outAppErrCode, char*& outErrString);
    bool AnalysisSocketErrorStr(int inSysErrCode, long& outAppErrCode, char*& outErrString)
    {
        long i = 0;

        outAppErrCode = 0;

        for (i = 0; MmErrors[i].appErrCode != 0; ++i)
        {
            if (MmErrors[i].sysErrCode == inSysErrCode)
            {
                outAppErrCode = MmErrors[i].appErrCode;
                outErrString = strdup(MmErrors[i].errCauseStr);
                break;
            }
        }

        if (MmErrors[i].appErrCode == 0)
        {
            outAppErrCode = MmErrors[i - 1].appErrCode;
            outErrString = strdup(MmErrors[i].errCauseStr);
        }

        return true;
    }

    bool AnalysisSocketErrorCode(int inSysErrCode, long& outAppErrCode)
    {
        long i = 0;

        outAppErrCode = 0;

        for (i = 0; MmErrors[i].appErrCode != 0; ++i)
        {
            if (MmErrors[i].sysErrCode == inSysErrCode)
            {
                outAppErrCode = MmErrors[i].appErrCode;
                break;
            }
        }

        if (MmErrors[i].appErrCode == 0)
        {
            outAppErrCode = MmErrors[i - 1].appErrCode;
        }

        return true;
    }
