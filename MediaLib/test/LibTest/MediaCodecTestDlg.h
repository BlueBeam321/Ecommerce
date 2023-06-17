// MediaCodecTestDlg.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMediaCodecTestDlg dialog
#include <vlc/vlc.h>
#include "afxwin.h"

class CMediaCodecTestDlg : public CDialogEx
{
// Construction
public:
	CMediaCodecTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MEDIACODECTEST_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnBrowse();
	afx_msg void OnBtnPlayStop();
	afx_msg void OnBtnPause();
	afx_msg void OnSeekBar(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSeekBarClicked(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSeekBarReleased();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBtnSave();
	afx_msg void OnTitleChange();
	afx_msg void OnChapterChange();
	afx_msg void OnAudioChange();
	afx_msg void OnSpuChange();
	afx_msg void OnSpuChange2();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSelchangeFilterModeList();
	afx_msg void OnCheckMute();
	afx_msg void OnCheckEqu();
	afx_msg void OnSpeedDown();
	afx_msg void OnSpeedNorm();
	afx_msg void OnSpeedUp();

	DECLARE_MESSAGE_MAP()

protected:
    void CreatePlayWindow();
    void SetUIControlInfo();

public:
    CProgressCtrl	m_levelSR;
    CProgressCtrl	m_levelSL;
    CProgressCtrl	m_levelR;
    CProgressCtrl	m_levelLFE;
    CProgressCtrl	m_levelL;
    CProgressCtrl	m_levelC;
    CProgressCtrl	m_levelBR;
    CProgressCtrl	m_levelBL;
    CComboBox		m_filerMode;
    CButton			m_equApply;
    CButton			m_mute;
    CSliderCtrl	m_equ600;
    CSliderCtrl	m_equ12K;
    CSliderCtrl	m_equ14K;
    CSliderCtrl	m_equ16K;
    CSliderCtrl	m_equ1K;
    CSliderCtrl	m_equ310;
    CSliderCtrl	m_equ3K;
    CSliderCtrl	m_equ6K;
    CSliderCtrl	m_equ170;
    CSliderCtrl	m_equ60;
    CSliderCtrl	m_preamp;
    CSliderCtrl	m_brightness;
    CSliderCtrl	m_Bass;
    CSliderCtrl	m_treble;
    CSliderCtrl	m_balance;
    CSliderCtrl	m_volume;
    CSliderCtrl	m_saturation;
    CSliderCtrl	m_Contrast;
    CSliderCtrl	m_hue;
    CSliderCtrl	m_gamma;
    CEdit		m_Information;
    CComboBox	m_SubPict;
    CComboBox	m_SubPict2;
    CComboBox	m_AudioTrack;
    CComboBox	m_Chapters;
    CComboBox	m_Titles;
    CListBox	m_msgList;
    CListBox	m_logList;
    CSliderCtrl	m_seekBar;

private:
    libvlc_instance_t* m_pVLC;
    libvlc_media_player_t* m_pPlayer;
    int m_nProgramCount;
    int m_nPrograms[32];
    int m_nAudioTrackCount;
    int m_nAudioTracks[256];
    int m_nSubtitleTrackCount;
    int m_nSubtitleTracks[256];
    int m_nChapterCount;

protected:
    virtual void OnCancel();
public:
    int m_nTitleCount;
    CComboBox m_cmbMediaList;
};
