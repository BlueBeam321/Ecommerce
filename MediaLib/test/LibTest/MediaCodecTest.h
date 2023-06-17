// MediaCodecTest.h : main header file for the MEDIACODECTEST application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMediaCodecTestApp:
// See MediaCodecTest.cpp for the implementation of this class
//

class CMediaCodecTestApp : public CWinApp
{
public:
	CMediaCodecTestApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CMediaCodecTestApp theApp;

