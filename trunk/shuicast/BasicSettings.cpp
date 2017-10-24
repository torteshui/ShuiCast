// BasicSettings.cpp : implementation file
//

#include "stdafx.h"
#include "shuicast.h"
#include "BasicSettings.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasicSettings dialog

CBasicSettings::CBasicSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CBasicSettings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBasicSettings)
	m_Bitrate = _T("");
	m_Channels = _T("");
	m_EncoderType = _T("");
	m_Attenuation = _T("");
	m_Mountpoint = _T("");
	m_Password = _T("");
	m_Quality = _T("");
	m_ReconnectSecs = _T("");
	m_Samplerate = _T("");
	m_ServerIP = _T("");
	m_ServerPort = _T("");
	m_ServerType = _T("");
	m_LamePreset = _T("");
	m_UseBitrate = FALSE;
	m_JointStereo = FALSE;
	//}}AFX_DATA_INIT
}

void CBasicSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBasicSettings)
	DDX_Control(pDX, IDC_JOINTSTEREO, m_JointStereoCtrl);
	DDX_Control(pDX, IDC_USEBITRATE, m_UseBitrateCtrl);
	DDX_Control(pDX, IDC_QUALITY, m_QualityCtrl);
	DDX_Control(pDX, IDC_BITRATE, m_BitrateCtrl);
	DDX_Control(pDX, IDC_CHANNELS, m_ChannelsCtrl);
	DDX_Control(pDX, IDC_SERVER_TYPE, m_ServerTypeCtrl);
	DDX_Control(pDX, IDC_ENCODER_TYPE, m_EncoderTypeCtrl);
	DDX_Text(pDX, IDC_BITRATE, m_Bitrate);
	DDX_Text(pDX, IDC_CHANNELS, m_Channels);
	DDX_CBString(pDX, IDC_ENCODER_TYPE, m_EncoderType);
	DDX_Text(pDX, IDC_MOUNTPOINT, m_Mountpoint);
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	DDX_Text(pDX, IDC_QUALITY, m_Quality);
	DDX_Text(pDX, IDC_RECONNECTSECS, m_ReconnectSecs);
	DDX_Text(pDX, IDC_SAMPLERATE, m_Samplerate);
	DDX_Text(pDX, IDC_SERVER_IP, m_ServerIP);
	DDX_Text(pDX, IDC_SERVER_PORT, m_ServerPort);
	DDX_CBString(pDX, IDC_SERVER_TYPE, m_ServerType);
	DDX_Check(pDX, IDC_USEBITRATE, m_UseBitrate);
	DDX_Check(pDX, IDC_JOINTSTEREO, m_JointStereo);
	DDX_Control(pDX, IDC_JOINTSTEREOLABEL, m_JointStereoLabelCtrl);
	DDX_Control(pDX, IDC_PARASTEREOLABEL, m_ParaStereoLabelCtrl);
	DDX_Control(pDX, IDC_MULTIASIOCHANNEL, m_AsioChannelCtrl);
	DDX_CBString(pDX, IDC_MULTIASIOCHANNEL, m_AsioChannel);
	DDX_Control(pDX, IDC_MULTIASIOCHANNEL2, m_AsioChannel2Ctrl);
	DDX_CBString(pDX, IDC_MULTIASIOCHANNEL2, m_AsioChannel2);
	DDX_Control(pDX, IDC_ATTENUATION, m_AttenuationCtrl);
	DDX_Text(pDX, IDC_ATTENUATION, m_Attenuation);
    DDX_Control(pDX, IDC_ASIOGROUPBOX, m_AsioGroupBox);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CBasicSettings, CDialog)
	//{{AFX_MSG_MAP(CBasicSettings)
	ON_CBN_SELCHANGE(IDC_ENCODER_TYPE, OnSelchangeEncoderType)
	ON_CBN_SELENDOK(IDC_ENCODER_TYPE, OnSelendokEncoderType)
	//ON_CBN_SELCHANGE(IDC_ATTENUATION, OnSelchangeAttenuation)
	ON_CBN_SELCHANGE(IDC_MULTIASIOCHANNEL, OnSelchangeAsio)
	ON_CBN_SELENDOK(IDC_MULTIASIOCHANNEL, OnSelendokAsio)
	ON_CBN_SELCHANGE(IDC_MULTIASIOCHANNEL2, OnSelchangeAsio2)
	ON_CBN_SELENDOK(IDC_MULTIASIOCHANNEL2, OnSelendokAsio2)
	ON_BN_CLICKED(IDC_USEBITRATE, OnUseBitrate)
	ON_BN_CLICKED(IDC_JOINTSTEREO, OnJointStereo)
	ON_EN_KILLFOCUS(IDC_BITRATE, OnBitrate)
	ON_EN_KILLFOCUS(IDC_CHANNELS, OnChannels)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

HBRUSH CBasicSettings::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	return m_brush;
}
/////////////////////////////////////////////////////////////////////////////
// CBasicSettings message handlers

BOOL CBasicSettings::OnInitDialog() 
{
    HINSTANCE       hDLL;
	m_brush.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	//m_brush.CreateSolidBrush(RGB(255, 255, 255));
	CDialog::OnInitDialog();
#ifndef MULTIASIO
	m_AsioChannelCtrl.ShowWindow(SW_HIDE);
	m_AsioChannel2Ctrl.ShowWindow(SW_HIDE);
    m_AsioGroupBox.ShowWindow(SW_HIDE);
#else
#ifdef MONOASIO
	m_AsioChannel2Ctrl.ShowWindow(SW_HIDE);
#endif
#endif
	m_ServerTypeCtrl.AddString(_T("Icecast2"));
	m_ServerTypeCtrl.AddString(_T("Shoutcast"));
#if HAVE_VORBIS
    m_EncoderTypeCtrl.AddString(_T("OggVorbis"));
#endif
#if HAVE_FLAC
    m_EncoderTypeCtrl.AddString(_T("Ogg FLAC"));
#endif
#if HAVE_LAME
    hDLL = LoadLibrary(_T("lame_enc.dll"));
    if ( hDLL == NULL ) hDLL = LoadLibrary( _T( "plugins\\lame_enc.dll" ) );
    if ( hDLL != NULL )
    {
        m_EncoderTypeCtrl.AddString(_T("MP3 Lame"));
		FreeLibrary(hDLL);
    }
#endif
#if HAVE_FAAC
	hDLL = LoadLibrary(_T("libfaac.dll"));
    if ( hDLL == NULL ) hDLL = LoadLibrary( _T( "plugins\\libfaac.dll" ) );
    if ( hDLL != NULL )
    {
        m_EncoderTypeCtrl.AddString(_T("AAC"));
		FreeLibrary(hDLL);
    }
#endif
#if HAVE_AACP
	hDLL = LoadLibrary(_T("enc_aacplus.dll"));
    if ( hDLL == NULL ) hDLL = LoadLibrary( _T( "plugins\\enc_aacplus.dll" ) );
    if ( hDLL != NULL )
    {
        m_EncoderTypeCtrl.AddString(_T("HE-AAC"));
        m_EncoderTypeCtrl.AddString(_T("HE-AAC High"));
        m_EncoderTypeCtrl.AddString(_T("LC-AAC"));
		FreeLibrary(hDLL);
    }
#endif
#if HAVE_FHGAACP
	hDLL = LoadLibrary(_T("enc_fhgaac.dll"));
    if ( hDLL == NULL ) hDLL = LoadLibrary( _T( "plugins\\enc_fhgaac.dll" ) );
    if ( hDLL != NULL )
    {
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-AUTO"));
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-LC"));
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-HE"));
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-HEv2"));
		FreeLibrary(hDLL);
    }
#endif
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBasicSettings::SetStereoLabels ( int val )
{
    m_JointStereoCtrl.ShowWindow     ( (val != 0) ? SW_SHOW : SW_HIDE );
    m_JointStereoLabelCtrl.ShowWindow( (val == 1) ? SW_SHOW : SW_HIDE );
    m_ParaStereoLabelCtrl.ShowWindow ( (val == 2) ? SW_SHOW : SW_HIDE );
}

void CBasicSettings::UpdateFields ()
{
    m_BitrateCtrl.EnableWindow(TRUE);
    m_QualityCtrl.EnableWindow(TRUE);
	m_UseBitrateCtrl.EnableWindow(FALSE);
	m_ChannelsCtrl.EnableWindow(TRUE);
	m_Attenuation.Format("%g", fabs(atof(m_Attenuation)));

    if (m_EncoderType == "FHGAAC-AUTO")
	{
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_PARAMETRIC );
		int br = atoi(LPCSTR(m_Bitrate));
		if (br < 12) br = 12;
		if (br > 448) br = 448;
		br = ((br + 3) >> 2) << 2;
		m_Bitrate.Format("%d", br);
	}
	else if (m_EncoderType == "FHGAAC-LC")
	{
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_PARAMETRIC );
		int br = atoi(LPCSTR(m_Bitrate));
		if (br < 16) br = 16;
		if (br > 448) br = 448;
		br = ((br + 3) >> 2) << 2;
		m_Bitrate.Format("%d", br);
	}
	else if (m_EncoderType == "FHGAAC-HE")
	{
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_PARAMETRIC );
		int br = atoi(LPCSTR(m_Bitrate));
		if (br < 16) br = 16;
		if (br > 128) br = 128;
		br = ((br + 3) >> 2) << 2;
		m_Bitrate.Format("%d", br);
	}
	else if (m_EncoderType == "FHGAAC-HEv2")
	{
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = true;
        SetStereoLabels( SLAB_PARAMETRIC );
		int br = atoi(LPCSTR(m_Bitrate));
		if (br < 12) br = 12;
		if (br > 56) br = 56;
		br = ((br + 3) >> 2) << 2;
		m_Bitrate.Format("%d", br);
	}
    else if (m_EncoderType == "HE-AAC") 
	{
		int br = atoi(LPCSTR(m_Bitrate));
		int ch = atoi(LPCSTR(m_Channels));
		if(!br) 
		{
			m_Bitrate = "8";
			br = 8;
		}
		if(br)
		{
			if(br < 8) m_Bitrate = "8";
			else if(br > 8 && br < 10) m_Bitrate = "10";
			else if(br > 10 && br < 12) m_Bitrate = "12";
			else if(br > 12 && br < 16) m_Bitrate = "16";
			else if(br > 16 && br < 20) m_Bitrate = "20";
			else if(br > 20 && br < 24) m_Bitrate = "24";
			else if(br > 24 && br < 28) m_Bitrate = "28";
			else if(br > 28 && br < 32) m_Bitrate = "32";
			else if(br > 32 && br < 40) m_Bitrate = "40";
			else if(br > 40 && br < 48) m_Bitrate = "48";
			else if(br > 48 && br < 56) m_Bitrate = "56";
			else if(br > 56 && br < 64) m_Bitrate = "64";
			else if(br > 64 && br < 80) m_Bitrate = "80";
			else if(br > 80 && br < 96) m_Bitrate = "96";
			else if(br > 96 && br < 112) m_Bitrate = "112";
			else if(br > 112) m_Bitrate = "128";
			br = atoi(LPCSTR(m_Bitrate));
		}
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
        SetStereoLabels( SLAB_PARAMETRIC );
		m_JointStereoCtrl.EnableWindow(TRUE);
		if(br > 64)
		{
			m_Channels = "2";
			ch = 2;
			m_JointStereoCtrl.EnableWindow(FALSE);
			m_JointStereo = false;
			m_ChannelsCtrl.EnableWindow(FALSE);
			// sync display
		}
		if(br < 12)
		{
			m_Channels = "1";
			ch = 1;
			m_JointStereoCtrl.EnableWindow(FALSE);
			m_JointStereo = false;
			m_ChannelsCtrl.EnableWindow(FALSE);
		}
		if(ch == 1) 
		{
			m_JointStereoCtrl.EnableWindow(FALSE);
			m_JointStereo = false;
			// sync display
		}
		else if (ch == 2)
		{
			if(br < 12)
			{
				m_JointStereoCtrl.EnableWindow(FALSE);
				m_Channels = "1";
				// sync display
			}
			else if (br < 16)
			{
				m_JointStereoCtrl.EnableWindow(FALSE);
				m_JointStereo = true;
				// sync display
			}
			else if (br > 56)
			{
				m_JointStereoCtrl.EnableWindow(FALSE);
				m_JointStereo = false;
				// sync display
			}
		}
    }
    else if (m_EncoderType == "HE-AAC High")
    {
		int br = atoi(LPCSTR(m_Bitrate));
		int ch = atoi(LPCSTR(m_Channels));
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_NONE );
		if(br)
		{
			if(br < 64) m_Bitrate = "64";
			else if(br > 64 && br < 80) m_Bitrate = "80";
			else if(br > 80 && br < 96) m_Bitrate = "96";
			else if(br > 96 && br < 112) m_Bitrate = "112";
			else if(br > 112 && br < 128) m_Bitrate = "128";
			else if(br > 128 && br < 160) m_Bitrate = "160";
			else if(br > 160 && br < 192) m_Bitrate = "192";
			else if(br > 192 && br < 224) m_Bitrate = "224";
			else if(br > 224) m_Bitrate = "256";
			br = atoi(LPCSTR(m_Bitrate));
		}
		if(br < 96 && ch == 2)
		{
			m_Channels = "1";
			ch = 1;
			m_ChannelsCtrl.EnableWindow(FALSE);
		}
		else if(br > 160 && ch == 1)
		{
			m_Channels = "2";
			ch = 2;
			m_ChannelsCtrl.EnableWindow(FALSE);
		}
    }
    else if (m_EncoderType == "LC-AAC")
    {
		int br = atoi(LPCSTR(m_Bitrate));
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_NONE );
		if(br)
		{
			if(br < 8) m_Bitrate = "8";
			else if(br > 8 && br < 12) m_Bitrate = "12";
			else if(br > 12 && br < 16) m_Bitrate = "16";
			else if(br > 16 && br < 20) m_Bitrate = "20";
			else if(br > 20 && br < 24) m_Bitrate = "24";
			else if(br > 24 && br < 32) m_Bitrate = "32";
			else if(br > 32 && br < 40) m_Bitrate = "40";
			else if(br > 40 && br < 48) m_Bitrate = "48";
			else if(br > 48 && br < 56) m_Bitrate = "56";
			else if(br > 56 && br < 64) m_Bitrate = "64";
			else if(br > 64 && br < 80) m_Bitrate = "80";
			else if(br > 80 && br < 96) m_Bitrate = "96";
			else if(br > 96 && br < 112) m_Bitrate = "112";
			else if(br > 112 && br < 128) m_Bitrate = "128";
			else if(br > 128 && br < 160) m_Bitrate = "160";
			else if(br > 160 && br < 192) m_Bitrate = "192";
			else if(br > 192 && br < 224) m_Bitrate = "224";
			else if(br > 224 && br < 256) m_Bitrate = "256";
			else if(br > 256) m_Bitrate = "320";
			br = atoi(LPCSTR(m_Bitrate));
		}
    }
    else if (m_EncoderType == "AAC")
    {
        m_BitrateCtrl.EnableWindow(  m_UseBitrate );
        m_QualityCtrl.EnableWindow( !m_UseBitrate );
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_NONE );
    }
    else if (m_EncoderType == "OggVorbis")
    {
		m_UseBitrateCtrl.EnableWindow(TRUE);
        m_BitrateCtrl.EnableWindow(  m_UseBitrate );
        m_QualityCtrl.EnableWindow( !m_UseBitrate );
        m_JointStereoCtrl.EnableWindow( FALSE );
		m_JointStereo = false;
        SetStereoLabels( SLAB_NONE );
    }
    else if (m_EncoderType == "MP3 Lame")
    {
        m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(TRUE);
        SetStereoLabels( SLAB_JOINT );
    }
    else if (m_EncoderType == "Ogg FLAC")
    {
	    m_BitrateCtrl.EnableWindow(FALSE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
		m_JointStereo = false;
        SetStereoLabels( SLAB_NONE );
    }
}

void CBasicSettings::OnSelchangeEncoderType() 
{
	UpdateFields();
}

void CBasicSettings::OnSelendokEncoderType() 
{
	//UpdateData(TRUE);
    int selected = m_EncoderTypeCtrl.GetCurSel();
    CString  selectedString;
    m_EncoderTypeCtrl.GetLBText(m_EncoderTypeCtrl.GetCurSel(), selectedString);
    m_BitrateCtrl.EnableWindow(TRUE);
    m_QualityCtrl.EnableWindow(TRUE);
	m_EncoderType = selectedString;
	UpdateFields();
}

void CBasicSettings::OnSelchangeAsio()
{
	UpdateFields();
}

void CBasicSettings::OnSelendokAsio() 
{
	//UpdateData(TRUE);
    int selected = m_AsioChannelCtrl.GetCurSel();
    CString  selectedString;
    m_AsioChannelCtrl.GetLBText(m_AsioChannelCtrl.GetCurSel(), selectedString);
	m_AsioChannel = selectedString;
	UpdateFields();
}

void CBasicSettings::OnSelchangeAsio2()
{
	UpdateFields();
}

void CBasicSettings::OnSelendokAsio2() 
{
	//UpdateData(TRUE);
    int selected = m_AsioChannel2Ctrl.GetCurSel();
    CString  selectedString;
    m_AsioChannel2Ctrl.GetLBText(m_AsioChannel2Ctrl.GetCurSel(), selectedString);
	m_AsioChannel2 = selectedString;
	UpdateFields();
}

void CBasicSettings::OnUseBitrate () 
{
    UpdateData( TRUE );
    UpdateFields();
}

void CBasicSettings::OnJointStereo () 
{
    UpdateData( TRUE );
    UpdateFields();
    UpdateData( FALSE );
}

void CBasicSettings::OnBitrate () 
{
    UpdateData( TRUE );
    UpdateFields();
    UpdateData( FALSE );
}

void CBasicSettings::OnChannels () 
{
    UpdateData( TRUE );
    UpdateFields();
    UpdateData( FALSE );
}
