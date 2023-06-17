
#ifndef __ErrorCode_VOD_H__
#define __ErrorCode_VOD_H__


    typedef struct tagVODERRORTBL
    {
        long	appErrCode;
        long	sysErrCode;
        const char* errCauseStr;
    } VODERRORTBL, *PVODERRORTBL;

    bool AnalysisSocketError(int inSysErrCode, long& outAppErrCode, char*& outErrString);
    bool AnalysisSocketErrorCode(int inSysErrCode, long& outAppErrCode);

#endif// __ErrorCode_VOD_H__