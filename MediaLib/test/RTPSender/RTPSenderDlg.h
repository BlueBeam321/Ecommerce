
// RTPSenderDlg.h : header file
//

#pragma once
#include "afxcmn.h"


// CRTPSenderDlg dialog
class CRTPSenderDlg : public CDialogEx
{
// Construction
public:
	CRTPSenderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_RTPSENDER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
    virtual void OnCancel();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnClickedButtonBrowse();
    afx_msg void OnClickedButtonStart();
    afx_msg void OnClickedButtonStop();
    afx_msg void OnRadioCapture();
    afx_msg void OnRadioFile();
    afx_msg void OnRadioScreen();

    DWORD m_iDestIP;
    int m_iDestPort;
    CString m_szFileName;
    int m_iFramerate;
    CProgressCtrl m_prgSeek;
};
