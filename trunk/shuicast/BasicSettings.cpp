// BasicSettings.cpp : implementation file
//

#include "stdafx.h"
#include "shuicast.h"
#include "BasicSettings.h"

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
	DDX_Control(pDX, IDC_ATTENUATION, m_AttenuationCtrl);
	DDX_Text(pDX, IDC_ATTENUATION, m_Attenuation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBasicSettings, CDialog)
	//{{AFX_MSG_MAP(CBasicSettings)
	ON_CBN_SELCHANGE(IDC_ENCODER_TYPE, OnSelchangeEncoderType)
	ON_CBN_SELENDOK(IDC_ENCODER_TYPE, OnSelendokEncoderType)
	ON_BN_CLICKED(IDC_USEBITRATE, OnUsebitrate)
	ON_BN_CLICKED(IDC_JOINTSTEREO, OnJointstereo)
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
	m_ServerTypeCtrl.AddString(_T("Icecast2"));
	m_ServerTypeCtrl.AddString(_T("Shoutcast"));
#ifdef HAVE_VORBIS
    m_EncoderTypeCtrl.AddString(_T("OggVorbis"));
#endif
#ifdef HAVE_FLAC
    m_EncoderTypeCtrl.AddString(_T("Ogg FLAC"));
#endif
#ifdef HAVE_LAME
    hDLL = LoadLibrary(_T("lame_enc.dll"));
    if(hDLL != NULL) {
        m_EncoderTypeCtrl.AddString(_T("MP3 Lame"));
		FreeLibrary(hDLL);
    }
#endif
#ifdef HAVE_FAAC
	hDLL = LoadLibrary(_T("libfaac.dll"));
    if(hDLL != NULL) {
        m_EncoderTypeCtrl.AddString(_T("AAC"));
		FreeLibrary(hDLL);
    }
#endif
#ifdef HAVE_AACP
	hDLL = LoadLibrary(_T("enc_aacplus.dll"));
    if(hDLL != NULL) {
        m_EncoderTypeCtrl.AddString(_T("HE-AAC"));
        m_EncoderTypeCtrl.AddString(_T("HE-AAC High"));
        m_EncoderTypeCtrl.AddString(_T("LC-AAC"));
        m_EncoderTypeCtrl.AddString(_T("AAC Plus"));
		FreeLibrary(hDLL);
    }
	else {
		//FreeLibrary(hDLL);
		hDLL = LoadLibrary(_T("plugins\\enc_aacplus.dll"));
		if(hDLL != NULL) {
			m_EncoderTypeCtrl.AddString(_T("HE-AAC"));
			m_EncoderTypeCtrl.AddString(_T("HE-AAC High"));
			m_EncoderTypeCtrl.AddString(_T("LC-AAC"));
			m_EncoderTypeCtrl.AddString(_T("AAC Plus"));
			FreeLibrary(hDLL);
		}
	}
#endif
#ifdef HAVE_FHGAACP
	hDLL = LoadLibrary(_T("enc_fhgaac.dll"));
    if(hDLL != NULL) {
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-AUTO"));
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-LC"));
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-HE"));
        m_EncoderTypeCtrl.AddString(_T("FHGAAC-HEv2"));
		FreeLibrary(hDLL);
    }
	else {
		//FreeLibrary(hDLL);
		hDLL = LoadLibrary(_T("plugins\\enc_fhgaac.dll"));
		if(hDLL != NULL) {
			m_EncoderTypeCtrl.AddString(_T("FHGAAC-AUTO"));
			m_EncoderTypeCtrl.AddString(_T("FHGAAC-LC"));
			m_EncoderTypeCtrl.AddString(_T("FHGAAC-HE"));
			m_EncoderTypeCtrl.AddString(_T("FHGAAC-HEv2"));
			FreeLibrary(hDLL);
		}
	}
#endif
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CBasicSettings::UpdateFields() {
    m_BitrateCtrl.EnableWindow(TRUE);
    m_QualityCtrl.EnableWindow(TRUE);
	m_UseBitrateCtrl.EnableWindow(FALSE);
    if (m_EncoderType == "AAC Plus") {
	    m_BitrateCtrl.EnableWindow(TRUE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
    }
    if (m_EncoderType == "AAC") {
		if (m_UseBitrate) {
	        m_BitrateCtrl.EnableWindow(TRUE);
	        m_QualityCtrl.EnableWindow(FALSE);
		}
		else {
	        m_QualityCtrl.EnableWindow(TRUE);
	        m_BitrateCtrl.EnableWindow(FALSE);
		}
		m_JointStereoCtrl.EnableWindow(FALSE);
    }
    if (m_EncoderType == "OggVorbis") {
		m_UseBitrateCtrl.EnableWindow(TRUE);
		if (m_UseBitrate) {
	        m_BitrateCtrl.EnableWindow(TRUE);
	        m_QualityCtrl.EnableWindow(FALSE);
		}
		else {
	        m_QualityCtrl.EnableWindow(TRUE);
	        m_BitrateCtrl.EnableWindow(FALSE);
		}
		m_JointStereoCtrl.EnableWindow(FALSE);
    }
    if (m_EncoderType == "MP3 Lame") {
        m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(TRUE);
    }
    if (m_EncoderType == "Ogg FLAC") {
	    m_BitrateCtrl.EnableWindow(FALSE);
	    m_QualityCtrl.EnableWindow(FALSE);
		m_JointStereoCtrl.EnableWindow(FALSE);
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

void CBasicSettings::OnUsebitrate() 
{
	UpdateData(TRUE);
	UpdateFields();
}

void CBasicSettings::OnJointstereo() 
{
	UpdateData(TRUE);
	UpdateFields();
}
