
// RTPSender.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CRTPSenderApp:
// See RTPSender.cpp for the implementation of this class
//

class CRTPSenderApp : public CWinApp
{
public:
	CRTPSenderApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CRTPSenderApp theApp;