// Config.cpp : implementation file
//

#include "stdafx.h"
#include "shuicast.h"
#include "Config.h"
#include "MainWindow.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfig dialog


CConfig::CConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfig)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    basicSettings = new CBasicSettings();
    ypSettings = new CYPSettings();
    advSettings = new CAdvancedSettings();
    currentEnc = 0;
}

CConfig::~CConfig()
{
    if (basicSettings) {
        delete basicSettings;
    }
    if (ypSettings) {
        delete ypSettings;
    }
    if (advSettings) {
        delete advSettings;
    }
}
void CConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfig)
	DDX_Control(pDX, IDC_TAB1, m_TabCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfig, CDialog)
	//{{AFX_MSG_MAP(CConfig)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfig message handlers

BOOL CConfig::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_TabCtrl.InsertItem(0, _T("Basic Settings"));
	m_TabCtrl.InsertItem(1, _T("YP Settings"));
	m_TabCtrl.InsertItem(2, _T("Advanced Settings"));

    basicSettings->Create((UINT)IDD_PROPPAGE_LARGE, this);    
    ypSettings->Create((UINT)IDD_PROPPAGE_LARGE1, this);    
    advSettings->Create((UINT)IDD_PROPPAGE_LARGE2, this);    

    basicSettings->ShowWindow(SW_SHOW);
    ypSettings->ShowWindow(SW_HIDE);
    advSettings->ShowWindow(SW_HIDE);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfig::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int selected = m_TabCtrl.GetCurSel();
	switch(selected) {
		case 0:
			basicSettings->ShowWindow(SW_SHOW);
			ypSettings->ShowWindow(SW_HIDE);
			advSettings->ShowWindow(SW_HIDE);
			break;
		case 1:
			ypSettings->ShowWindow(SW_SHOW);
			basicSettings->ShowWindow(SW_HIDE);
			advSettings->ShowWindow(SW_HIDE);
			break;
		case 2:
			basicSettings->ShowWindow(SW_HIDE);
			ypSettings->ShowWindow(SW_HIDE);
			advSettings->ShowWindow(SW_SHOW);
			break;
    }

	*pResult = 0;
}
void CConfig::GlobalsToDialog ( CEncoder *encoder )
{
    char    buf[255] = "";
    /*Basic Settings 
	CString	m_Bitrate;
	CString	m_Channels;
	CString	m_EncoderType;
	CString	m_Mountpoint;
	CString	m_Password;
	CString	m_Quality;
	CString	m_ReconnectSecs;
	CString	m_Samplerate;
	CString	m_ServerIP;
	CString	m_ServerPort;
	CString	m_ServerType;
    */
    currentEnc = encoder->encoderNumber;
	basicSettings->setStereoLabels(SLAB_NONE);
    wsprintf( buf, "%d", encoder->GetCurrentBitrate() );
    basicSettings->m_Bitrate = buf;
    wsprintf( buf, "%d", encoder->GetCurrentChannels() );
    basicSettings->m_Channels = buf;
    wsprintf( buf, "%d", encoder->GetCurrentSamplerate() );
    basicSettings->m_Samplerate = buf;
    basicSettings->m_Attenuation = encoder->m_AttenuationTable;
    if ( encoder->gOggBitQualFlag == 0 ) { // Quality
		basicSettings->m_UseBitrate = false;
	}
	else {
		basicSettings->m_UseBitrate = true;
	}

    if ( encoder->m_Type == ENCODER_AAC ) {  // TODO: use switch
        basicSettings->m_EncoderType = "AAC";
        basicSettings->m_Quality = encoder->gAACQuality;
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_AACP_HE ) {
        basicSettings->m_EncoderType = "HE-AAC";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_PARAMETRIC);
    }
    if ( encoder->m_Type == ENCODER_AACP_HE_HIGH ) {
        basicSettings->m_EncoderType = "HE-AAC High";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_AACP_LC ) {
        basicSettings->m_EncoderType = "LC-AAC";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_FG_AACP_AUTO ) {
        basicSettings->m_EncoderType = "FHGAAC-AUTO";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_FG_AACP_LC ) {
        basicSettings->m_EncoderType = "FHGAAC-LC";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_FG_AACP_HE ) {
        basicSettings->m_EncoderType = "FHGAAC-HE";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_FG_AACP_HEV2 ) {
        basicSettings->m_EncoderType = "FHGAAC-HEv2";
        basicSettings->m_Quality = "";
		basicSettings->setStereoLabels(SLAB_PARAMETRIC);
		basicSettings->m_JointStereo = true;
    }
    if ( encoder->m_Type == ENCODER_LAME ) {
        basicSettings->m_EncoderType = "MP3 Lame";
        basicSettings->m_Quality = encoder->m_OggQuality;
		basicSettings->setStereoLabels(SLAB_JOINT);
    }
    if ( encoder->m_Type == ENCODER_OGG ) {
        basicSettings->m_EncoderType = "OggVorbis";
        basicSettings->m_Quality = encoder->m_OggQuality;
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    if ( encoder->m_Type == ENCODER_FLAC ) {
        basicSettings->m_EncoderType = "Ogg FLAC";
        basicSettings->m_Quality = encoder->m_OggQuality;
		basicSettings->setStereoLabels(SLAB_NONE);
    }
    basicSettings->m_EncoderTypeCtrl.SelectString(0, basicSettings->m_EncoderType);
    basicSettings->m_Mountpoint = encoder->m_Mountpoint;
    basicSettings->m_Password = encoder->m_Password;
    wsprintf( buf, "%d", encoder->m_ReconnectSec );

    basicSettings->m_ReconnectSecs = buf;
    basicSettings->m_ServerIP = encoder->m_Server;
    basicSettings->m_ServerPort = encoder->m_Port;

    if ( encoder->gShoutcastFlag ) {
        basicSettings->m_ServerType = "Shoutcast";
    }
    if ( encoder->gIcecast2Flag ) {
        basicSettings->m_ServerType = "Icecast2";
    }

    basicSettings->m_ServerTypeCtrl.SelectString(0, basicSettings->m_ServerType);

    if ( encoder->LAMEJointStereoFlag ) {
		basicSettings->m_JointStereo = true;
	}
	else
	{
		basicSettings->m_JointStereo = false;
	}
	// m_AsioChannel2 ??
    basicSettings->m_AsioChannel = encoder->gAsioChannel;
    basicSettings->UpdateData(FALSE);
    basicSettings->UpdateFields();
    /* YP Settings
    	BOOL	m_Public;
	CString	m_StreamDesc;
	CString	m_StreamGenre;
	CString	m_StreamName;
	CString	m_StreamURL;
    */
    ypSettings->m_Public = encoder->m_PubServ ? true : false;
    ypSettings->m_StreamDesc = encoder->m_ServDesc;
    ypSettings->m_StreamName = encoder->m_ServName;
    ypSettings->m_StreamGenre = encoder->m_ServGenre;
    ypSettings->m_StreamURL = encoder->m_ServURL;
    ypSettings->m_StreamAIM = encoder->m_ServAIM;
    ypSettings->m_StreamICQ = encoder->m_ServICQ;
    ypSettings->m_StreamIRC = encoder->m_ServIRC;
    ypSettings->UpdateData(FALSE);
    ypSettings->EnableDisable();
    /* Advanced Settings
    	CString	m_ArchiveDirectory;
	CString	m_Logfile;
	CString	m_Loglevel;
	BOOL	m_Savestream;
	BOOL	m_Savewav;
    */
    advSettings->m_Savestream = encoder->gSaveDirectoryFlag;
    advSettings->m_ArchiveDirectory = encoder->gSaveDirectory;
    wsprintf( buf, "%d", encoder->gLogLevel );
    advSettings->m_Loglevel = buf;
    advSettings->m_Logfile = encoder->gLogFile;

    advSettings->m_Savewav = encoder->gSaveAsWAV;
    advSettings->m_forceDSP = encoder->gForceDSPrecording;
    advSettings->UpdateData(FALSE);
    advSettings->EnableDisable();
}

void CConfig::DialogToGlobals ( CEncoder *encoder )
{
    char    buf[255] = "";
    /*Basic Settings 
	CString	m_Bitrate;
	CString	m_Channels;
	CString	m_EncoderType;
	CString	m_Mountpoint;
	CString	m_Password;
	CString	m_Quality;
	CString	m_ReconnectSecs;
	CString	m_Samplerate;
	CString	m_ServerIP;
	CString	m_ServerPort;
	CString	m_ServerType;
    */
    currentEnc = encoder->encoderNumber;

//    basicSettings->UpdateData(TRUE);

    encoder->m_CurrentBitrate    = atoi( LPCSTR( basicSettings->m_Bitrate ) );
    encoder->m_CurrentChannels   = atoi( LPCSTR( basicSettings->m_Channels ) );
    encoder->m_CurrentSamplerate = atoi( LPCSTR( basicSettings->m_Samplerate ) );
    encoder->m_Type = ENCODER_NONE;

    if (basicSettings->m_EncoderType == "HE-AAC") {
        encoder->m_Type = ENCODER_AACP_HE;
    }
    if (basicSettings->m_EncoderType == "HE-AAC High") {
        encoder->m_Type = ENCODER_AACP_HE_HIGH;
    }
    if (basicSettings->m_EncoderType == "LC-AAC") {
        encoder->m_Type = ENCODER_AACP_LC;
    }
    if (basicSettings->m_EncoderType == "AAC") {
        encoder->m_Type = ENCODER_AAC;
    }
    if (basicSettings->m_EncoderType == "MP3 Lame") {
        encoder->m_Type = ENCODER_LAME;
    }
    if (basicSettings->m_EncoderType == "OggVorbis") {
        encoder->m_Type = ENCODER_OGG;
    }
    if (basicSettings->m_EncoderType == "Ogg FLAC") {
        encoder->m_Type = ENCODER_FLAC;
    }
	//type = 0(AUTO) 1(LC) 2(HE-AAC) 3(HE-AACv2) - flag = type + 1
    if (basicSettings->m_EncoderType == "FHGAAC-AUTO") {
        encoder->m_Type = ENCODER_FG_AACP_AUTO;
    }
    if (basicSettings->m_EncoderType == "FHGAAC-LC") {
        encoder->m_Type = ENCODER_FG_AACP_LC;
    }
    if (basicSettings->m_EncoderType == "FHGAAC-HE") {
        encoder->m_Type = ENCODER_FG_AACP_HE;
    }
    if (basicSettings->m_EncoderType == "FHGAAC-HEv2") {
        encoder->m_Type = ENCODER_FG_AACP_HEV2;
    }

	if (basicSettings->m_UseBitrate) {
        encoder->gOggBitQualFlag = 1;
	}
	else {
        encoder->gOggBitQualFlag = 0;
	}
	if (basicSettings->m_JointStereo) {
        encoder->LAMEJointStereoFlag = 1;
	}
	else {
        encoder->LAMEJointStereoFlag = 0;
	}

    strcpy( encoder->m_AttenuationTable, LPCSTR( basicSettings->m_Attenuation ) );
	{
        double atten = -fabs( atof( encoder->m_AttenuationTable ) );
        encoder->m_Attenuation = pow( 10.0, atten/20.0 );
	}
    strcpy( encoder->gEncodeType, LPCSTR( basicSettings->m_EncoderType ) );

    if ( encoder->m_Type == ENCODER_AAC ) {
        strcpy( encoder->gAACQuality, LPCSTR( basicSettings->m_Quality ) );
    }
    if ( encoder->m_Type == ENCODER_LAME ) {
        strcpy( encoder->m_OggQuality, LPCSTR( basicSettings->m_Quality ) );
    }
    if ( encoder->m_Type == ENCODER_OGG ) {
        strcpy( encoder->m_OggQuality, LPCSTR( basicSettings->m_Quality ) );
    }
    strcpy( encoder->m_Mountpoint, LPCSTR( basicSettings->m_Mountpoint ) );
    strcpy( encoder->m_Password, LPCSTR( basicSettings->m_Password ) );

    encoder->m_ReconnectSec = atoi( LPCSTR( basicSettings->m_ReconnectSecs ) );
    strcpy( encoder->m_Server, LPCSTR( basicSettings->m_ServerIP ) );
    strcpy( encoder->m_Port, LPCSTR( basicSettings->m_ServerPort ) );

    if (basicSettings->m_ServerType == "Shoutcast") {
        encoder->gShoutcastFlag = 1;
        encoder->gIcecast2Flag = 0;
    }
    if (basicSettings->m_ServerType == "Icecast2") {
        encoder->gShoutcastFlag = 0;
        encoder->gIcecast2Flag = 1;
    }
    strcpy( encoder->gServerType, LPCSTR( basicSettings->m_ServerType ) );
	// m_AsioChannel2?!!
    strcpy( encoder->gAsioChannel, LPCSTR( basicSettings->m_AsioChannel ) );
    if ( !strcmp( encoder->gAsioChannel, "" ) )
        encoder->gAsioSelectChannel = 0;
	else
        encoder->gAsioSelectChannel = 1;
    ypSettings->UpdateData(TRUE);
    encoder->m_PubServ = ypSettings->m_Public ? 1 : 0;

    strcpy( encoder->m_ServDesc,  LPCSTR( ypSettings->m_StreamDesc  ) );
    strcpy( encoder->m_ServName,  LPCSTR( ypSettings->m_StreamName  ) );
    strcpy( encoder->m_ServGenre, LPCSTR( ypSettings->m_StreamGenre ) );
    strcpy( encoder->m_ServURL,   LPCSTR( ypSettings->m_StreamURL   ) );
    strcpy( encoder->m_ServAIM,   LPCSTR( ypSettings->m_StreamAIM   ) );
    strcpy( encoder->m_ServICQ,   LPCSTR( ypSettings->m_StreamICQ   ) );
    strcpy( encoder->m_ServIRC,   LPCSTR( ypSettings->m_StreamIRC   ) );

    /* Advanced Settings
    	CString	m_ArchiveDirectory;
	CString	m_Logfile;
	CString	m_Loglevel;
	BOOL	m_Savestream;
	BOOL	m_Savewav;
    */
    advSettings->UpdateData(TRUE);

    encoder->gSaveDirectoryFlag = advSettings->m_Savestream;
    strcpy( encoder->gSaveDirectory, LPCSTR( advSettings->m_ArchiveDirectory ) );
    encoder->gLogLevel = atoi( LPCSTR( advSettings->m_Loglevel ) );
    strcpy( encoder->gLogFile, LPCSTR( advSettings->m_Logfile ) );
    encoder->gSaveAsWAV = advSettings->m_Savewav;
    encoder->gForceDSPrecording = advSettings->m_forceDSP;

}

void CConfig::OnClose() 
{
    CDialog::OnClose();
}

void CConfig::OnOK() 
{
	basicSettings->UpdateData(TRUE);
	ypSettings->UpdateData(TRUE);
	advSettings->UpdateData(TRUE);
    CMainWindow *pwin = (CMainWindow *)parentDialog;
    pwin->ProcessConfigDone(currentEnc, this);
    CDialog::OnOK();
}

void CConfig::OnCancel() 
{
    CMainWindow *pwin = (CMainWindow *)parentDialog;
    pwin->ProcessConfigDone(-1, this);
    CDialog::OnCancel();
}
