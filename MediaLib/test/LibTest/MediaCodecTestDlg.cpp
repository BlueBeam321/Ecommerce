// MediaCodecTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MediaCodecTest.h"
#include "MediaCodecTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

char* szMediaList[] = {
    "01_MPG1",
    "02_MP2",
    "03_DVM2",
    "04_DVH2",
    "04_DVH2_2",
    "05_H4T",
    "06_MOV",
    "07_MP4",
    "08_AVI",
    "09_3GP",
    "10_WEB",
    "11_MKV1",
    "11_MKV2",
    "12_DVM",
    "13_DVM",
    "15_STAR",
};


wchar_t* wzMediaList[] = {
    L"01_MPG1",
    L"02_MP2",
    L"03_DVM2",
    L"04_DVH2",
    L"04_DVH2_2",
    L"05_H4T",
    L"06_MOV",
    L"07_MP4",
    L"08_AVI",
    L"09_3GP",
    L"10_WEB",
    L"11_MKV1",
    L"11_MKV2",
    L"12_DVM",
    L"13_DVM",
    L"15_STAR",
};

/////////////////////////////////////////////////////////////////////////////
// CMediaCodecTestDlg dialog

HWND g_dispWnd = NULL;

CMediaCodecTestDlg::CMediaCodecTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMediaCodecTestDlg::IDD, pParent)
    , m_nTitleCount(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    m_pVLC = NULL;
    m_pPlayer = NULL;
    m_nProgramCount = 0;
    m_nAudioTrackCount = 0;
    m_nSubtitleTrackCount = 0;
    m_nChapterCount = 0;
}

void CMediaCodecTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LEVEL_SR, m_levelSR);
    DDX_Control(pDX, IDC_LEVEL_SL, m_levelSL);
    DDX_Control(pDX, IDC_LEVEL_R, m_levelR);
    DDX_Control(pDX, IDC_LEVEL_LFE, m_levelLFE);
    DDX_Control(pDX, IDC_LEVEL_L, m_levelL);
    DDX_Control(pDX, IDC_LEVEL_C, m_levelC);
    DDX_Control(pDX, IDC_LEVEL_BR, m_levelBR);
    DDX_Control(pDX, IDC_LEVEL_BL, m_levelBL);
    DDX_Control(pDX, IDC_FilterModeList, m_filerMode);
    DDX_Control(pDX, IDC_CHECK_EQU, m_equApply);
    DDX_Control(pDX, IDC_CHECK_MUTE, m_mute);
    DDX_Control(pDX, IDC_Equ600, m_equ600);
    DDX_Control(pDX, IDC_Equ12K, m_equ12K);
    DDX_Control(pDX, IDC_Equ14K, m_equ14K);
    DDX_Control(pDX, IDC_Equ16K, m_equ16K);
    DDX_Control(pDX, IDC_Equ1K, m_equ1K);
    DDX_Control(pDX, IDC_Equ310, m_equ310);
    DDX_Control(pDX, IDC_Equ3K, m_equ3K);
    DDX_Control(pDX, IDC_Equ6K, m_equ6K);
    DDX_Control(pDX, IDC_Equ170, m_equ170);
    DDX_Control(pDX, IDC_Equ60, m_equ60);
    DDX_Control(pDX, IDC_Preamp, m_preamp);
    DDX_Control(pDX, IDC_Brightness, m_brightness);
    DDX_Control(pDX, IDC_Bass, m_Bass);
    DDX_Control(pDX, IDC_Treble, m_treble);
    DDX_Control(pDX, IDC_Balance, m_balance);
    DDX_Control(pDX, IDC_Volume, m_volume);
    DDX_Control(pDX, IDC_Saturation, m_saturation);
    DDX_Control(pDX, IDC_Contrast, m_Contrast);
    DDX_Control(pDX, IDC_Hue, m_hue);
    DDX_Control(pDX, IDC_Gamma, m_gamma);
    DDX_Control(pDX, IDC_INFORMATION, m_Information);
    DDX_Control(pDX, IDC_SPU, m_SubPict);
    DDX_Control(pDX, IDC_SPU2, m_SubPict2);
    DDX_Control(pDX, IDC_AUDIO, m_AudioTrack);
    DDX_Control(pDX, IDC_CHAPTER, m_Chapters);
    DDX_Control(pDX, IDC_TITLE, m_Titles);
    DDX_Control(pDX, IDC_MSGLIST, m_msgList);
    DDX_Control(pDX, IDC_LOGLIST, m_logList);
    DDX_Control(pDX, IDC_SeekBar, m_seekBar);
    DDX_Text(pDX, IDC_TXT_Title, m_nTitleCount);
    DDX_Control(pDX, IDC_MEDIA_LIST, m_cmbMediaList);
}

BEGIN_MESSAGE_MAP(CMediaCodecTestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_Browse, OnBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_PLAY, OnBtnPlayStop)
    ON_BN_CLICKED(IDC_BTN_CONT, OnBtnPause)
    //ON_BN_CLICKED(IDC_SeekBar, OnSeekBarClicked)
    ON_NOTIFY(NM_CLICK, IDC_SeekBar, OnSeekBarClicked)
    ON_NOTIFY(NM_DBLCLK, IDC_SeekBar, OnSeekBarClicked)
    ON_NOTIFY(NM_SETFOCUS, IDC_SeekBar, OnSeekBarClicked)
    ON_NOTIFY(NM_KILLFOCUS, IDC_SeekBar, OnSeekBarClicked)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SeekBar, OnSeekBar)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_SAVE, OnBtnSave)
	ON_CBN_SELCHANGE(IDC_TITLE, OnTitleChange)
	ON_CBN_SELCHANGE(IDC_CHAPTER, OnChapterChange)
	ON_CBN_SELCHANGE(IDC_AUDIO, OnAudioChange)
	ON_CBN_SELCHANGE(IDC_SPU, OnSpuChange)
	ON_CBN_SELCHANGE(IDC_SPU2, OnSpuChange2)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(IDC_FilterModeList, OnSelchangeFilterModeList)
	ON_BN_CLICKED(IDC_CHECK_MUTE, OnCheckMute)
	ON_BN_CLICKED(IDC_CHECK_EQU, OnCheckEqu)
	ON_BN_CLICKED(IDC_BTN_RATE_DOWN, OnSpeedDown)
	ON_BN_CLICKED(IDC_BTN_RATE_NOR, OnSpeedNorm)
	ON_BN_CLICKED(IDC_BTN_RATE_UP, OnSpeedUp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMediaCodecTestDlg message handlers

BOOL CMediaCodecTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
    m_filerMode.SetCurSel(6);

    CreatePlayWindow();
    SetUIControlInfo();

    m_pVLC = libvlc_new();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMediaCodecTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMediaCodecTestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMediaCodecTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

static LRESULT PlayWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void CMediaCodecTestDlg::CreatePlayWindow()
{
    RECT rect_window;
    WNDCLASSEX wc;
    COLORREF backcolor = RGB(10, 10, 10);
	
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)PlayWinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(backcolor);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("_MediaCodecTestVideoEx_");
    wc.hIconSm = NULL;
    if (!RegisterClassEx (&wc))
	{
		DWORD ret = GetLastError();
        if (ret != ERROR_CLASS_ALREADY_EXISTS)
		{
            fprintf(stderr, "Can not register window class\n");
            return;
		}
    }
	
    long ScreenX, ScreenY;
    long width, height;
	
    ScreenX = GetSystemMetrics(SM_CXFULLSCREEN);
    ScreenY = GetSystemMetrics(SM_CYFULLSCREEN);
	width = 1024;//1920;
	height = 768;//1080;
	
    rect_window.left = (ScreenX - width) / 2;
    rect_window.top = (ScreenY - height) / 2;
	//rect_window.top = 20;
    rect_window.right = rect_window.left + width;
    rect_window.bottom = rect_window.top + height;
	
    DWORD Exstyle, style;
	
    Exstyle = WS_EX_WINDOWEDGE;
    style = WS_OVERLAPPED | WS_CAPTION;// | WS_SYSMENU | WS_THICKFRAME | WS_SIZEBOX;
	
    HINSTANCE hinst = AfxGetInstanceHandle();

    g_dispWnd = CreateWindowEx(Exstyle, _T("_MediaCodecTestVideoEx_"), _T("MediaCodecTestVideo"), style,
        rect_window.left, rect_window.top,
        rect_window.right - rect_window.left,
        rect_window.bottom - rect_window.top,
        NULL, NULL, AfxGetInstanceHandle(), NULL);

	if (g_dispWnd == NULL)
	{
		fprintf (stderr, "Can not create window\n");
		return;
	}
	
    SetWindowLong(g_dispWnd, GWLP_USERDATA, (LONG)this);
	
	return;
}

void CMediaCodecTestDlg::OnBtnBrowse() 
{
    UpdateData(TRUE);
#if 0
    WCHAR szFilter[] = _T("All Files(*.*)|*.*|MP2(*.mp2)|*.mp2|MPG(*.mpg)|*.mpg|");
    CFileDialog	dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, szFilter);
    if (dlg.DoModal() != IDOK)
        return;

    CString filePath = dlg.GetPathName();
    char filename[256] = { 0 };
    SetDlgItemText(IDC_TXT_FilePath, filePath);
    UpdateData(FALSE);

    WideCharToMultiByte(CP_UTF8, 0, filePath, filePath.GetLength(), filename, 256, NULL, NULL);

    libvlc_media_t* media = libvlc_media_new_path(m_pVLC, filename);
#if 0 // Test stream output
    if (media)
    {
        //libvlc_media_add_option(media, "sout=#rtp{dst=192.168.1.140,port=8000,mux=ts}");
        //libvlc_media_add_option(media, "sout=#duplicate{dst=rtp{dst=192.168.1.140,port=8000,mux=ts},dst=display}");
        libvlc_media_add_option(media, "sout=#rtp{sdp=rtsp://:8554/test}");
        libvlc_media_add_option(media, "no-sout-all");
        libvlc_media_add_option(media, "sout-keep");
    }
#endif
#elif 0
    char filename[256] = { 0 };
    int index = m_cmbMediaList.GetCurSel();
    sprintf(filename, "vod://192.168.1.71:9113/0\tUser\t1\t%s\t12\t194\t1\t192.2.2.156\tCont1.00@/MP4", szMediaList[index]);
    m_cmbMediaList.SetCurSel(++index);
    UpdateData(false);
    libvlc_media_t* media = libvlc_media_new_location(m_pVLC, filename);
#elif 1
    char filename[256] = { 0 };
    //strcpy(filename, "rtp://238.4.10.10:39876");
    strcpy(filename, "rtp://235.5.5.1:39876");
    //strcpy(filename, "rtp://230.1.1.1:8000");
    //strcpy(filename, "rtp://192.168.1.140:9500");
    //strcpy(filename, "rtp://127.0.0.1:8000");
    //strcpy(filename, "rtsp://192.168.1.140:8554/live");
    libvlc_media_t* media = libvlc_media_new_location(m_pVLC, filename);
#if 1 // Parse MPEG-2 TS program
    if (media)
    {
        libvlc_media_program_parser_start(media);
        while (!libvlc_media_program_parser_is_completed(media))
            Sleep(20);

        int i_programs, i;
        libvlc_program_t** pp_programs = NULL;
        WCHAR programName[256];
        i_programs = libvlc_media_program_parser_get(media, &pp_programs);
        libvlc_media_program_parser_stop(media);
        for (i = 0; i < i_programs; i++)
        {
            memset(programName, 0, sizeof(WCHAR) * 256);
            MultiByteToWideChar(CP_UTF8, 0, pp_programs[i]->psz_name, strlen(pp_programs[i]->psz_name), programName, 256);
        }
        libvlc_programs_release(pp_programs, i_programs);
        libvlc_media_release(media);

        return;
    }
#endif
#endif

    if (!media)
        return;

    libvlc_state_t state;
    m_pPlayer = libvlc_media_player_new_from_media(media);

    libvlc_media_release(media);
    libvlc_media_player_set_hwnd(m_pPlayer, g_dispWnd);
    ::ShowWindow(g_dispWnd, SW_SHOW);

    libvlc_media_player_set_scramble(m_pPlayer, true);
    libvlc_media_player_set_vod_title_count(m_pPlayer, m_nTitleCount);

    libvlc_media_player_play(m_pPlayer);
    do {
        state = libvlc_media_player_get_state(m_pPlayer);
    } while (state != libvlc_Playing && state != libvlc_Error && state != libvlc_Ended);
    state = libvlc_media_player_get_state(m_pPlayer);
    if (state == libvlc_Playing)
    {
        //libvlc_media_player_set_video_title_display(m_pPlayer, libvlc_position_bottom, 5000);
        //libvlc_video_set_aspect_ratio(m_pPlayer, "");

        m_nChapterCount = 0;
        m_nAudioTrackCount = 0;
        m_nSubtitleTrackCount = 0;

#if 0
        libvlc_title_description_t **pp_tittles;
        int i_title = libvlc_media_player_get_full_title_descriptions(m_pPlayer, &pp_tittles);
        for (int i = 0; i < i_title; i++)
        {
            WCHAR tmp[256];
            memset(tmp, 0, 256 * sizeof(WCHAR));
            MultiByteToWideChar(CP_UTF8, 0, pp_tittles[i]->psz_name, strlen(pp_tittles[i]->psz_name), tmp, 256);
            m_Titles.AddString(tmp);
        }
        if (pp_tittles)
            libvlc_title_descriptions_release(pp_tittles, i_title);
#endif
        SetTimer(1, 1000, NULL);
    }
    else
    {
        libvlc_media_player_stop(m_pPlayer);
        libvlc_media_player_release(m_pPlayer);
        m_pPlayer = NULL;

        ::ShowWindow(g_dispWnd, SW_HIDE);
        MessageBox(_T("Open fail!!!"), NULL, MB_ICONWARNING | MB_OK);
    }
}

void CMediaCodecTestDlg::SetUIControlInfo()
{
	m_hue.SetRange(0, 360);
	m_hue.SetPos(180);
	m_Contrast.SetRange(0, 200);
	m_Contrast.SetPos(100);
	m_saturation.SetRange(0, 200);
	m_saturation.SetPos(100);
	m_brightness.SetRange(0, 200);
	m_brightness.SetPos(100);
	m_gamma.SetRange(1, 1000);
	m_gamma.SetPos(100);

	m_volume.SetRange(0, 100);
	m_volume.SetPos(50);
	m_balance.SetRange(0, 100);
	m_balance.SetPos(50);
	m_treble.SetRange(0, 100);
	m_treble.SetPos(50);
	m_Bass.SetRange(0, 100);
	m_Bass.SetPos(50);
	m_preamp.SetRange(0, 100);
	m_preamp.SetPos(50);

	m_equ600.SetRange(0, 400);
	m_equ600.SetPos(200);
	m_equ12K.SetRange(0, 400);
	m_equ12K.SetPos(200);
	m_equ14K.SetRange(0, 400);
	m_equ14K.SetPos(200);
	m_equ16K.SetRange(0, 400);
	m_equ16K.SetPos(200);
	m_equ1K.SetRange(0, 400);
	m_equ1K.SetPos(200);
	m_equ310.SetRange(0, 400);
	m_equ310.SetPos(200);
	m_equ3K.SetRange(0, 400);
	m_equ3K.SetPos(200);
	m_equ6K.SetRange(0, 400);
	m_equ6K.SetPos(200);
	m_equ170.SetRange(0, 400);
	m_equ170.SetPos(200);
	m_equ60.SetRange(0, 400);
	m_equ60.SetPos(200);

	m_levelL.SetRange(0, 250);
	m_levelL.SetPos(0);
	m_levelC.SetRange(0, 250);
	m_levelC.SetPos(0);
	m_levelR.SetRange(0, 250);
	m_levelR.SetPos(0);
	m_levelSL.SetRange(0, 250);
	m_levelSL.SetPos(0);
	m_levelSR.SetRange(0, 250);
	m_levelSR.SetPos(0);
	m_levelBL.SetRange(0, 250);
	m_levelBL.SetPos(0);
	m_levelBR.SetRange(0, 250);
	m_levelBR.SetPos(0);
	m_levelLFE.SetRange(0, 250);
	m_levelLFE.SetPos(0);

    for (int i = 0; i < sizeof(wzMediaList) / sizeof(wchar_t*); i++)
    {
        m_cmbMediaList.AddString(wzMediaList[i]);
    }

    m_cmbMediaList.SetCurSel(0);
}

void CMediaCodecTestDlg::OnBtnPlayStop() 
{
	// TODO: Add your control notification handler code here
    if (m_pPlayer)
    {
        KillTimer(0);
        libvlc_media_player_stop(m_pPlayer);
        libvlc_media_player_release(m_pPlayer);
        m_pPlayer = NULL;
        m_nProgramCount = 0;
        m_nAudioTrackCount = 0;
        m_nSubtitleTrackCount = 0;
    }
    if (m_pVLC)
    {
        libvlc_release(m_pVLC);
        m_pVLC = NULL;
    }
    ::ShowWindow(g_dispWnd, SW_HIDE);
    m_Titles.ResetContent();
    m_AudioTrack.ResetContent();
    m_SubPict.ResetContent();
    m_SubPict2.ResetContent();
    m_Chapters.ResetContent();
}

void CMediaCodecTestDlg::OnBtnPause() 
{
	// TODO: Add your control notification handler code here
    //static int aspect = 0;
    //if (m_pPlayer)
    //{
    //    aspect = (aspect + 1) % 4;
    //    if (aspect == 0) // Auto
    //        libvlc_video_set_aspect_ratio(m_pPlayer, "");
    //    else if (aspect == 1) // 16:9
    //        libvlc_video_set_aspect_ratio(m_pPlayer, "16:9");
    //    else if (aspect == 2) // 4:3
    //        libvlc_video_set_aspect_ratio(m_pPlayer, "4:3");
    //    else if (aspect == 3) // 1:1
    //        libvlc_video_set_aspect_ratio(m_pPlayer, "1:1");
    //}
}

void CMediaCodecTestDlg::OnSpeedDown() 
{
	
}

void CMediaCodecTestDlg::OnSpeedNorm() 
{
	
}

void CMediaCodecTestDlg::OnSpeedUp()
{
	
}

static LRESULT PlayWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc (hwnd, message, wParam, lParam);
}


void CMediaCodecTestDlg::OnSeekBar(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void CMediaCodecTestDlg::OnTimer(UINT_PTR nIDEvent) 
{
    if (m_pPlayer)
    {
#if 1
        int programCount = libvlc_media_player_get_program_count(m_pPlayer);
        if (m_nProgramCount != programCount)
        {
            int i, id;
            char name[256];
            WCHAR w_name[256];
            m_Titles.ResetContent();
            for (i = 0; i < programCount; i++)
            {
                memset(name, 0, 256);
                memset(w_name, 0, 256 * sizeof(WCHAR));
                libvlc_media_player_get_program_info(m_pPlayer, i, &id, name);
                m_nPrograms[i] = id;
                MultiByteToWideChar(CP_UTF8, 0, name, strlen(name), w_name, 256);
                m_Titles.AddString(w_name);
            }
            if (programCount > 0)
                libvlc_media_player_change_program(m_pPlayer, m_nPrograms[0]);
            m_nProgramCount = programCount;
        }
#endif

        /* Refresh Chapter */
        libvlc_chapter_description_t** pp_chapters;
        int chapterCount = libvlc_media_player_get_full_chapter_descriptions(m_pPlayer, -1, &pp_chapters);
        if (m_nChapterCount != chapterCount)
        {
            m_Chapters.ResetContent();
            if (chapterCount > 0)
            {
                WCHAR tmp[256];
                for (int i = 0; i < chapterCount; i++)
                {
                    memset(tmp, 0, 256 * sizeof(WCHAR));
                    if (pp_chapters[i]->psz_name)
                        MultiByteToWideChar(CP_UTF8, 0, pp_chapters[i]->psz_name, strlen(pp_chapters[i]->psz_name), tmp, 256);
                    else
                        wsprintf(tmp, L"Chapter_%d", i);
                    m_Chapters.AddString(tmp);
                }
            }
            m_nChapterCount = chapterCount;
        }
        if (chapterCount > 0)
            libvlc_chapter_descriptions_release(pp_chapters, chapterCount);

        /* Refresh Audio */
        int audioTrackCount = 0;
        libvlc_track_description_t* p_tracks = libvlc_audio_get_track_description(m_pPlayer);
        libvlc_track_description_t* p_track = p_tracks;
        while (p_track)
        {
            audioTrackCount++;
            p_track = p_track->p_next;
        }
        if (m_nAudioTrackCount != audioTrackCount)
        {
            m_AudioTrack.ResetContent();
            int audioTrackId = libvlc_audio_get_track(m_pPlayer);
            if (p_tracks)
            {
                int activeTrack = 0, i = 0;
                WCHAR tmp[256];
                
                p_track = p_tracks;
                while (p_track)
                {
                    memset(tmp, 0, 256 * sizeof(WCHAR));
                    MultiByteToWideChar(CP_UTF8, 0, p_track->psz_name, strlen(p_track->psz_name), tmp, 256);
                    m_AudioTrack.AddString(tmp);

                    if (p_track->i_id == audioTrackId)
                        activeTrack = i;
                    m_nAudioTracks[i++] = p_track->i_id;
                    p_track = p_track->p_next;
                }
                if (audioTrackCount > 0)
                    m_AudioTrack.SetCurSel(activeTrack);
            }
            m_nAudioTrackCount = audioTrackCount;
        }
        if (p_tracks)
            libvlc_track_description_list_release(p_tracks);

        /* Refresh Subtitle */
        int subtitleTrackCount = 0;
        p_tracks = libvlc_video_get_spu_description(m_pPlayer);
        p_track = p_tracks;
        while (p_track)
        {
            subtitleTrackCount++;
            p_track = p_track->p_next;
        }
        if (m_nSubtitleTrackCount != subtitleTrackCount)
        {
            m_SubPict.ResetContent();
            m_SubPict2.ResetContent();
            int mainSubtitleTrackId = libvlc_video_get_spu(m_pPlayer, false);
            int subSubtitleTrackId = libvlc_video_get_spu(m_pPlayer, true);
            if (p_tracks)
            {
                int activeTrack = 0, activeTrack2 = 0, i = 0;
                WCHAR tmp[256];
                libvlc_track_description_t* p_track = p_tracks;
                while (p_track)
                {
                    memset(tmp, 0, 256 * sizeof(WCHAR));
                    MultiByteToWideChar(CP_UTF8, 0, p_track->psz_name, strlen(p_track->psz_name), tmp, 256);
                    m_SubPict.AddString(tmp);
                    m_SubPict2.AddString(tmp);

                    if (p_track->i_id == mainSubtitleTrackId)
                        activeTrack = i;
                    if (p_track->i_id == subSubtitleTrackId)
                        activeTrack2 = i;
                    m_nSubtitleTracks[i++] = p_track->i_id;
                    p_track = p_track->p_next;
                }
                if (subtitleTrackCount > 0)
                {
                    m_SubPict.SetCurSel(activeTrack);
                    m_SubPict2.SetCurSel(activeTrack2);
                }
            }
            m_nSubtitleTrackCount = subtitleTrackCount;
        }
        if (p_tracks)
            libvlc_track_description_list_release(p_tracks);

        /* Update Time */
        libvlc_time_t allSec = libvlc_media_player_get_length(m_pPlayer);
        allSec /= 1000;
        m_seekBar.SetRange(0, allSec);

        int32_t hour = allSec / 3600;
        int32_t minute = (allSec % 3600) / 60;
        int32_t second = allSec % 60;
        CString str;
        str.Format(_T("%02d:%02d:%02d"), hour, minute, second);
        SetDlgItemText(IDC_TotalTime, str);

        libvlc_time_t curSec = libvlc_media_player_get_time(m_pPlayer);
        curSec /= 1000;
        m_seekBar.SetPos(curSec);

        hour = curSec / 3600;
        minute = (curSec % 3600) / 60;
        second = curSec % 60;
        str.Format(_T("%02d:%02d:%02d"), hour, minute, second);
        SetDlgItemText(IDC_CurTime, str);
    }
    /*if (m_pPlayer)
    {
        static int i_decoded = 0;
        libvlc_media_t* media = libvlc_media_player_get_media(m_pPlayer);
        libvlc_media_stats_t stats;
        libvlc_media_get_stats(media, &stats);
        libvlc_media_release(media);
        if (stats.i_decoded_video != i_decoded)
        {
            i_decoded = stats.i_decoded_video;
            OutputDebugStringA("New video decoded\n");
        }
    }*/
	CDialog::OnTimer(nIDEvent);
}

void CMediaCodecTestDlg::OnBtnSave() 
{
}

void CMediaCodecTestDlg::OnTitleChange() 
{
	// TODO: Add your control notification handler code here
#if 1
    int index = m_Titles.GetCurSel();
    if (index >= 0 && index < m_nProgramCount && m_pPlayer)
        libvlc_media_player_change_program(m_pPlayer, m_nPrograms[index]);
#else
    int index = m_Titles.GetCurSel();
    libvlc_media_player_set_title(m_pPlayer, index);
#endif
}

void CMediaCodecTestDlg::OnChapterChange() 
{
    int index = m_Chapters.GetCurSel();
    if (index >= 0 && m_pPlayer)
        libvlc_media_player_set_chapter(m_pPlayer, index);
}

void CMediaCodecTestDlg::OnAudioChange() 
{
    int index = m_AudioTrack.GetCurSel();
    if (index >= 0 && index < m_nAudioTrackCount && m_pPlayer)
        libvlc_audio_set_track(m_pPlayer, m_nAudioTracks[index]);
}

void CMediaCodecTestDlg::OnSpuChange() 
{
    int index = m_SubPict.GetCurSel();
    if (index >= 0 && index < m_nSubtitleTrackCount && m_pPlayer)
        libvlc_video_set_spu(m_pPlayer, m_nSubtitleTracks[index], false);
}

void CMediaCodecTestDlg::OnSpuChange2() 
{
    int index = m_SubPict2.GetCurSel();
    if (index >= 0 && index < m_nSubtitleTrackCount && m_pPlayer)
        libvlc_video_set_spu(m_pPlayer, m_nSubtitleTracks[index], true);
}

void CMediaCodecTestDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
        /* seek */
        if ((void*)pScrollBar == (void*)&m_seekBar)
        {
            int pos = ((CSliderCtrl*)pScrollBar)->GetPos();
            int length = ((CSliderCtrl*)pScrollBar)->GetRangeMax();

            //libvlc_media_player_set_position(m_pPlayer, (float)pos / (float)length);
            libvlc_media_player_set_time(m_pPlayer, pos * 1000LL);
        }

    //if (m_pMediaPlayer && (nSBCode == TB_THUMBTRACK || nSBCode == TB_ENDTRACK))
    //{
    //    //L_MediaCodec::vfilter_params_t v_params;
    //    //L_MediaCodec::afilter_params_t a_params;

    //    /* seek */
    //    if ((void*)pScrollBar == (void*)&m_seekBar)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pMediaPlayer->Pause();
    //        m_pMediaPlayer->Seek(nPos);
    //        m_pMediaPlayer->Play();
    //    }
    //    /*else if ((void*)pScrollBar == (void*)&m_hue)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(&v_params, NULL);
    //        v_params.hue = nPos;
    //        v_params.hue = nPos - 180;
    //        m_pCodec->SetCodecParams(&v_params, NULL);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_Contrast)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(&v_params, NULL);
    //        v_params.contrast = nPos;
    //        m_pCodec->SetCodecParams(&v_params, NULL);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_brightness)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(&v_params, NULL);
    //        v_params.brightness = nPos;
    //        m_pCodec->SetCodecParams(&v_params, NULL);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_saturation)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(&v_params, NULL);
    //        v_params.saturation = nPos;
    //        m_pCodec->SetCodecParams(&v_params, NULL);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_gamma)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(&v_params, NULL);
    //        v_params.gamma = nPos;
    //        m_pCodec->SetCodecParams(&v_params, NULL);
    //    }*/
    //    /*else if ((void*)pScrollBar == (void*)&m_volume)
    //    {
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(NULL, &a_params);
    //        a_params.volume = nPos;
    //        m_pCodec->SetCodecParams(NULL, &a_params);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_balance)
    //    {
    //        m_equApply.SetCheck(BST_UNCHECKED);
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(NULL, &a_params);
    //        a_params.basstreble_flag = TRUE;
    //        a_params.balance = nPos;
    //        m_pCodec->SetCodecParams(NULL, &a_params);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_treble)
    //    {
    //        m_equApply.SetCheck(BST_UNCHECKED);
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(NULL, &a_params);
    //        a_params.basstreble_flag = TRUE;
    //        a_params.treble = nPos;
    //        m_pCodec->SetCodecParams(NULL, &a_params);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_Bass)
    //    {
    //        m_equApply.SetCheck(BST_UNCHECKED);
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(NULL, &a_params);
    //        a_params.basstreble_flag = TRUE;
    //        a_params.bass = nPos;
    //        m_pCodec->SetCodecParams(NULL, &a_params);
    //    }
    //    else if ((void*)pScrollBar == (void*)&m_preamp)
    //    {
    //        m_equApply.SetCheck(BST_UNCHECKED);
    //        nPos = ((CSliderCtrl*)pScrollBar)->GetPos();
    //        m_pCodec->GetCodecParams(NULL, &a_params);
    //        a_params.basstreble_flag = TRUE;
    //        a_params.preamp = nPos;
    //        m_pCodec->SetCodecParams(NULL, &a_params);
    //    }*/
    //}
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMediaCodecTestDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CMediaCodecTestDlg::OnSelchangeFilterModeList() 
{
	// TODO: Add your control notification handler code here
	
}

void CMediaCodecTestDlg::OnCheckMute() 
{
	// TODO: Add your control notification handler code here
	
}

void CMediaCodecTestDlg::OnCheckEqu() 
{
	// TODO: Add your control notification handler code here
	
}


void CMediaCodecTestDlg::OnCancel()
{
    // TODO: Add your specialized code here and/or call the base class
    if (g_dispWnd)
        ::DestroyWindow(g_dispWnd);
    g_dispWnd = NULL;

    if (m_pVLC)
    {
        libvlc_release(m_pVLC);
        m_pVLC = NULL;
    }

    CDialogEx::OnCancel();
}

void CMediaCodecTestDlg::OnSeekBarClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

}