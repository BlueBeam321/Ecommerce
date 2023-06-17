
// RTPSenderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RTPSender.h"
#include "RTPSenderDlg.h"
#include "afxdialogex.h"

#include <vlc/vlc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define APP_SECTION     _T("Settings")
#define APP_DEST_IP     _T("DestIP")
#define APP_DEST_PORT   _T("DestPort")
#define APP_SRC_TYPE    _T("SrcType")
#define APP_SRC_PATH    _T("SrcPath")
#define APP_TRANSCODE   _T("Transcode")
#define APP_FRAMERATE   _T("Framerate")

static libvlc_instance_t* s_pVLC = NULL;
static libvlc_media_list_player_t* s_pMediaPlayer = NULL;

// CRTPSenderDlg dialog



CRTPSenderDlg::CRTPSenderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRTPSenderDlg::IDD, pParent)
    , m_iDestIP(0)
    , m_iDestPort(0)
    , m_szFileName(_T(""))
    , m_iFramerate(1)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRTPSenderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_IPAddress(pDX, IDC_IPADDRESS_DEST, m_iDestIP);
    DDX_Text(pDX, IDC_EDIT_PORT, m_iDestPort);
    DDX_Text(pDX, IDC_EDIT_FILENAME, m_szFileName);
    DDX_Control(pDX, IDC_PROGRESS_SEEK, m_prgSeek);
    DDX_Text(pDX, IDC_EDIT_FPS, m_iFramerate);
	DDV_MinMaxInt(pDX, m_iFramerate, 1, 120);
}

BEGIN_MESSAGE_MAP(CRTPSenderDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CRTPSenderDlg::OnClickedButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_START, &CRTPSenderDlg::OnClickedButtonStart)
    ON_BN_CLICKED(IDC_BUTTON_STOP, &CRTPSenderDlg::OnClickedButtonStop)
    ON_COMMAND(IDC_RADIO_CAPTURE, &CRTPSenderDlg::OnRadioCapture)
    ON_COMMAND(IDC_RADIO_FILE, &CRTPSenderDlg::OnRadioFile)
    ON_COMMAND(IDC_RADIO_SCREEN, &CRTPSenderDlg::OnRadioScreen)
END_MESSAGE_MAP()


// CRTPSenderDlg message handlers

BOOL CRTPSenderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    // TODO: Add extra initialization here
    CWinApp* pApp = AfxGetApp();
    //free((void*)pApp->m_pszProfileName);
    //pApp->m_pszProfileName = _tcsdup(L"app");

    m_iDestIP = pApp->GetProfileIntW(APP_SECTION, APP_DEST_IP, 0xE6010101);
    m_iDestPort = pApp->GetProfileIntW(APP_SECTION, APP_DEST_PORT, 9000);

    int type = pApp->GetProfileIntW(APP_SECTION, APP_SRC_TYPE, 0);
    CButton* pButton;
    if (type == 0)
    {
        pButton = (CButton*)GetDlgItem(IDC_RADIO_CAPTURE);
        pButton->SetCheck(TRUE);
        OnRadioCapture();
    }
    else if (type == 1)
    {
        pButton = (CButton*)GetDlgItem(IDC_RADIO_FILE);
        pButton->SetCheck(TRUE);
        OnRadioFile();

        m_szFileName = pApp->GetProfileStringW(APP_SECTION, APP_SRC_PATH, _T(""));

        pButton = (CButton*)GetDlgItem(IDC_CHECK_TRANSCODE);
        pButton->SetCheck(pApp->GetProfileIntW(APP_SECTION, APP_TRANSCODE, 0) != 0);
    }
    else if (type == 2)
    {
        pButton = (CButton*)GetDlgItem(IDC_RADIO_SCREEN);
        pButton->SetCheck(TRUE);
        OnRadioScreen();

        m_iFramerate = pApp->GetProfileIntW(APP_SECTION, APP_FRAMERATE, 1);
    }
    UpdateData(FALSE);

    GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

    s_pVLC = libvlc_new();

	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CRTPSenderDlg::OnCancel()
{
    // TODO: Add your specialized code here and/or call the base class
    if (s_pMediaPlayer)
    {
        KillTimer(0);
        libvlc_media_list_player_stop(s_pMediaPlayer);
        libvlc_media_list_player_release(s_pMediaPlayer);
        s_pMediaPlayer = NULL;
    }

    if (s_pVLC)
        libvlc_release(s_pVLC);
    s_pVLC = NULL;

    CDialogEx::OnCancel();
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRTPSenderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRTPSenderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRTPSenderDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (s_pMediaPlayer)
    {
        /* Update Time */
        libvlc_media_player_t* player = libvlc_media_list_player_get_media_player(s_pMediaPlayer);
        if (player)
        {
            libvlc_time_t allSec = libvlc_media_player_get_length(player);
            libvlc_time_t curSec = libvlc_media_player_get_time(player);
            allSec /= 1000;
            curSec /= 1000;
            m_prgSeek.SetRange(0, (short)allSec);
            m_prgSeek.SetPos((int)curSec);
            libvlc_media_player_release(player);
        }
    }
}


void CRTPSenderDlg::OnClickedButtonBrowse()
{
    UpdateData();

    CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"All Media Files|*.avi;*.mp4;*.mov;*.mkv;*.3gp;*.webm;*.mpg|All Files|*.*|", this);
    if (dlg.DoModal() == IDOK)
    {
        m_szFileName = dlg.GetPathName();
        UpdateData(FALSE);
    }
}


void CRTPSenderDlg::OnClickedButtonStart()
{
    if (!UpdateData())
        return;

    libvlc_media_list_t* pMediaList = NULL;
    libvlc_media_t* pMedia = NULL;
    BOOL bTranscode = TRUE;
    CButton* pCaptureButton = (CButton*)GetDlgItem(IDC_RADIO_CAPTURE);
    CButton* pFileButton = (CButton*)GetDlgItem(IDC_RADIO_FILE);
    CButton* pScreenButton = (CButton*)GetDlgItem(IDC_RADIO_SCREEN);    
    int type = 0;
    if (pCaptureButton->GetCheck())
        type = 0;
    else if (pFileButton->GetCheck())
        type = 1;
    else if (pScreenButton->GetCheck())
        type = 2;

    if (type == 0) // Camera/Microphone
        pMedia = libvlc_media_new_location(s_pVLC, "dshow://");
    else if (type == 1) // File
    {
        char filename[256] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, m_szFileName, m_szFileName.GetLength(), filename, 256, NULL, NULL);
        pMedia = libvlc_media_new_path(s_pVLC, filename);
        
        CButton* pTranscodeButton = (CButton*)GetDlgItem(IDC_CHECK_TRANSCODE);
        bTranscode = pTranscodeButton->GetCheck();
    }
    else if (type == 2) // Desktop
        pMedia = libvlc_media_new_location(s_pVLC, "screen://");

    if (!pMedia)
    {
        MessageBox(L"Can't open media!!!", NULL, MB_ICONWARNING | MB_OK);
        return;
    }

    char str[256];
    if (bTranscode)
    {
        sprintf(str, 
            "sout=#transcode{vcodec=h264,vb=4000,acodec=mpga,ab=128,channels=2,samplerate=44100,scodec=none}"
            ":rtp{dst=%d.%d.%d.%d,port=%d,mux=ts}",
            (int)((m_iDestIP >> 24) & 0xFF),
            (int)((m_iDestIP >> 16) & 0xFF),
            (int)((m_iDestIP >> 8) & 0xFF),
            (int)((m_iDestIP >> 0) & 0xFF),
            m_iDestPort);
    }
    else
    {
        sprintf(str, "sout=#rtp{dst=%d.%d.%d.%d,port=%d,mux=ts}",
            (int)((m_iDestIP >> 24) & 0xFF),
            (int)((m_iDestIP >> 16) & 0xFF),
            (int)((m_iDestIP >> 8) & 0xFF),
            (int)((m_iDestIP >> 0) & 0xFF),
            m_iDestPort);
    }
    libvlc_media_add_option(pMedia, str);
    libvlc_media_add_option(pMedia, "no-sout-all");
    libvlc_media_add_option(pMedia, "sout-keep");
    libvlc_media_add_option(pMedia, "loop");
    libvlc_media_add_option(pMedia, "repeat");
    if (type == 2)
    {
        sprintf(str, "screen-fps=%f", (float)m_iFramerate);
        libvlc_media_add_option(pMedia, str);
    }

    pMediaList = libvlc_media_list_new(s_pVLC);
    libvlc_media_list_add_media(pMediaList, pMedia);

    s_pMediaPlayer = libvlc_media_list_player_new(s_pVLC);
    libvlc_media_list_player_set_media_list(s_pMediaPlayer, pMediaList);
    libvlc_media_list_player_set_playback_mode(s_pMediaPlayer, libvlc_playback_mode_loop);
    

    libvlc_state_t state;
    //libvlc_media_list_player_play(s_pMediaPlayer);
    libvlc_media_list_player_play_item(s_pMediaPlayer, pMedia);
    //libvlc_media_list_player_play_item_at_index(s_pMediaPlayer, 0);
    
    libvlc_media_release(pMedia);
    libvlc_media_list_release(pMediaList);

    do {
        state = libvlc_media_list_player_get_state(s_pMediaPlayer);
    } while (state != libvlc_Playing && state != libvlc_Error && state != libvlc_Ended);
    
    state = libvlc_media_list_player_get_state(s_pMediaPlayer);
    if (state == libvlc_Playing)
    {
        CWinApp* pApp = AfxGetApp();
        pApp->WriteProfileInt(APP_SECTION, APP_DEST_IP, m_iDestIP);
        pApp->WriteProfileInt(APP_SECTION, APP_DEST_PORT, m_iDestPort);
        pApp->WriteProfileInt(APP_SECTION, APP_SRC_TYPE, type);
        if (type == 1)
        {
            pApp->WriteProfileStringW(APP_SECTION, APP_SRC_PATH, m_szFileName);
            pApp->WriteProfileInt(APP_SECTION, APP_TRANSCODE, bTranscode ? 1 : 0);
        }
        else if (type == 2)
            pApp->WriteProfileInt(APP_SECTION, APP_FRAMERATE, m_iFramerate);

        GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);
        SetTimer(0, 1000, NULL);
    }
    else
    {
        libvlc_media_list_player_stop(s_pMediaPlayer);
        libvlc_media_list_player_release(s_pMediaPlayer);
        s_pMediaPlayer = NULL;

        MessageBox(L"Can't play media!!!", NULL, MB_ICONWARNING | MB_OK);
    }
}


void CRTPSenderDlg::OnClickedButtonStop()
{
    if (s_pMediaPlayer)
    {
        KillTimer(0);
        libvlc_media_list_player_stop(s_pMediaPlayer);
        libvlc_media_list_player_release(s_pMediaPlayer);
        s_pMediaPlayer = NULL;
    }

    GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
    m_prgSeek.SetPos(0);
}


void CRTPSenderDlg::OnRadioCapture()
{
    // TODO: Add your command handler code here
    GetDlgItem(IDC_EDIT_FILENAME)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_TRANSCODE)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT_FPS)->EnableWindow(FALSE);
}


void CRTPSenderDlg::OnRadioFile()
{
    // TODO: Add your command handler code here
    GetDlgItem(IDC_EDIT_FILENAME)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK_TRANSCODE)->EnableWindow(TRUE);
    GetDlgItem(IDC_EDIT_FPS)->EnableWindow(FALSE);
}


void CRTPSenderDlg::OnRadioScreen()
{
    // TODO: Add your command handler code here
    GetDlgItem(IDC_EDIT_FILENAME)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_TRANSCODE)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT_FPS)->EnableWindow(TRUE);
}
