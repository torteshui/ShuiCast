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

BOOL CConfig::OnInitDialog ()
{
    CDialog::OnInitDialog();
    m_TabCtrl.InsertItem( 0, _T( "Basic Settings" ) );
    m_TabCtrl.InsertItem( 1, _T( "YP Settings" ) );
    m_TabCtrl.InsertItem( 2, _T( "Advanced Settings" ) );
    basicSettings->Create( (UINT)IDD_PROPPAGE_LARGE, this );
    ypSettings->Create( (UINT)IDD_PROPPAGE_LARGE1, this );
    advSettings->Create( (UINT)IDD_PROPPAGE_LARGE2, this );
    basicSettings->ShowWindow( SW_SHOW );
    ypSettings->ShowWindow( SW_HIDE );
    advSettings->ShowWindow( SW_HIDE );
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfig::OnSelchangeTab1 ( NMHDR *pNMHDR, LRESULT *pResult )
{
    int selected = m_TabCtrl.GetCurSel();
    basicSettings->ShowWindow( (selected == 0) ? SW_SHOW : SW_HIDE );
    ypSettings->ShowWindow   ( (selected == 1) ? SW_SHOW : SW_HIDE );
    advSettings->ShowWindow  ( (selected == 2) ? SW_SHOW : SW_HIDE );
    *pResult = 0;
}

void CConfig::GlobalsToDialog ( CEncoder *encoder )
{
    // Basic Settings 
    currentEnc = encoder->encoderNumber;
	basicSettings->setStereoLabels(SLAB_NONE);
    basicSettings->m_Bitrate.Format( _T( "%d" ), encoder->GetCurrentBitrate() );
    basicSettings->m_Channels.Format( _T( "%d" ), encoder->GetCurrentChannels() );
    basicSettings->m_Samplerate.Format( _T( "%d" ), encoder->GetCurrentSamplerate() );
    basicSettings->m_Attenuation = encoder->m_AttenuationTable;
    basicSettings->m_UseBitrate = (BOOL)encoder->m_UseBitrate;

    switch ( encoder->GetEncoderType() )
    {
    case ENCODER_AAC:
        basicSettings->m_EncoderType = "AAC";
        basicSettings->m_Quality = encoder->gAACQuality;
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_AACP_HE:
        basicSettings->m_EncoderType = "HE-AAC";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_PARAMETRIC );
        break;
    case ENCODER_AACP_HE_HIGH:
        basicSettings->m_EncoderType = "HE-AAC High";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_AACP_LC:
        basicSettings->m_EncoderType = "LC-AAC";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_FG_AACP_AUTO:
        basicSettings->m_EncoderType = "FHGAAC-AUTO";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_FG_AACP_LC:
        basicSettings->m_EncoderType = "FHGAAC-LC";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_FG_AACP_HE:
        basicSettings->m_EncoderType = "FHGAAC-HE";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_FG_AACP_HEV2:
        basicSettings->m_EncoderType = "FHGAAC-HEv2";
        basicSettings->m_Quality = "";
        basicSettings->setStereoLabels( SLAB_PARAMETRIC );
        basicSettings->m_JointStereo = true;
        break;
    case ENCODER_LAME:
        basicSettings->m_EncoderType = "MP3 Lame";
        basicSettings->m_Quality = encoder->m_OggQuality;
        basicSettings->setStereoLabels( SLAB_JOINT );
        break;
    case ENCODER_OGG:
        basicSettings->m_EncoderType = "OggVorbis";
        basicSettings->m_Quality = encoder->m_OggQuality;
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    case ENCODER_FLAC:
        basicSettings->m_EncoderType = "Ogg FLAC";
        basicSettings->m_Quality = encoder->m_OggQuality;
        basicSettings->setStereoLabels( SLAB_NONE );
        break;
    }

    basicSettings->m_EncoderTypeCtrl.SelectString(0, basicSettings->m_EncoderType);
    basicSettings->m_Mountpoint = encoder->m_Mountpoint;
    basicSettings->m_Password = encoder->m_Password;
    basicSettings->m_ReconnectSecs.Format( _T( "%d" ), encoder->m_ReconnectSec );
    basicSettings->m_ServerIP = encoder->m_Server;
    basicSettings->m_ServerPort = encoder->m_Port;

    if ( encoder->IsServerType( SERVER_SHOUTCAST ) ) basicSettings->m_ServerType = "Shoutcast";
    if ( encoder->IsServerType( SERVER_ICECAST2  ) ) basicSettings->m_ServerType = "Icecast2";  // TODO: also ICECAST?
    basicSettings->m_ServerTypeCtrl.SelectString(0, basicSettings->m_ServerType);
    basicSettings->m_JointStereo = (encoder->m_JointStereo != 0);

    // m_AsioChannel2 ??
    basicSettings->m_AsioChannel = encoder->m_AsioChannel;
    basicSettings->UpdateData(FALSE);
    basicSettings->UpdateFields();

    // YP Settings
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

    // Advanced Settings
    advSettings->m_Savestream = encoder->m_SaveDirectoryFlag;
    advSettings->m_ArchiveDirectory = encoder->m_SaveDirectory;
    advSettings->m_Loglevel.Format( _T( "%d" ), encoder->m_LogLevel );
    advSettings->m_Logfile = encoder->m_LogFile;
    advSettings->m_Savewav = encoder->m_SaveAsWAV;
    advSettings->m_forceDSP = encoder->m_ForceDSPRecording;
    advSettings->UpdateData(FALSE);
    advSettings->EnableDisable();
}

void CConfig::DialogToGlobals ( CEncoder *encoder )
{
    currentEnc = encoder->encoderNumber;
    //basicSettings->UpdateData(TRUE);

    encoder->m_CurrentBitrate    = atoi( LPCSTR( basicSettings->m_Bitrate ) );
    encoder->m_CurrentChannels   = atoi( LPCSTR( basicSettings->m_Channels ) );
    encoder->m_CurrentSamplerate = atoi( LPCSTR( basicSettings->m_Samplerate ) );
    
         if ( basicSettings->m_EncoderType == "HE-AAC"      ) encoder->SetEncoderType( ENCODER_AACP_HE      );
    else if ( basicSettings->m_EncoderType == "HE-AAC High" ) encoder->SetEncoderType( ENCODER_AACP_HE_HIGH );
    else if ( basicSettings->m_EncoderType == "LC-AAC"      ) encoder->SetEncoderType( ENCODER_AACP_LC      );
    else if ( basicSettings->m_EncoderType == "AAC"         ) encoder->SetEncoderType( ENCODER_AAC          );
    else if ( basicSettings->m_EncoderType == "MP3 Lame"    ) encoder->SetEncoderType( ENCODER_LAME         );
    else if ( basicSettings->m_EncoderType == "OggVorbis"   ) encoder->SetEncoderType( ENCODER_OGG          );
    else if ( basicSettings->m_EncoderType == "Ogg FLAC"    ) encoder->SetEncoderType( ENCODER_FLAC         );
    else if ( basicSettings->m_EncoderType == "FHGAAC-AUTO" ) encoder->SetEncoderType( ENCODER_FG_AACP_AUTO );
    else if ( basicSettings->m_EncoderType == "FHGAAC-LC"   ) encoder->SetEncoderType( ENCODER_FG_AACP_LC   );
    else if ( basicSettings->m_EncoderType == "FHGAAC-HE"   ) encoder->SetEncoderType( ENCODER_FG_AACP_HE   );
    else if ( basicSettings->m_EncoderType == "FHGAAC-HEv2" ) encoder->SetEncoderType( ENCODER_FG_AACP_HEV2 );
    else                                                      encoder->SetEncoderType( ENCODER_NONE         );

    encoder->m_UseBitrate = !!basicSettings->m_UseBitrate;  // double negation to get rid of warning
    encoder->m_JointStereo = basicSettings->m_JointStereo ? 1 : 0;

    strcpy( encoder->m_AttenuationTable, LPCSTR( basicSettings->m_Attenuation ) );
    double atten = -fabs( atof( encoder->m_AttenuationTable ) );
    encoder->m_Attenuation = pow( 10.0, atten/20.0 );
    strcpy( encoder->m_EncodeType, LPCSTR( basicSettings->m_EncoderType ) );

    if ( encoder->IsEncoderType( ENCODER_AAC  ) ) strcpy( encoder->gAACQuality,  LPCSTR( basicSettings->m_Quality ) );
    if ( encoder->IsEncoderType( ENCODER_LAME ) ) strcpy( encoder->m_OggQuality, LPCSTR( basicSettings->m_Quality ) );
    if ( encoder->IsEncoderType( ENCODER_OGG  ) ) strcpy( encoder->m_OggQuality, LPCSTR( basicSettings->m_Quality ) );
    strcpy( encoder->m_Mountpoint, LPCSTR( basicSettings->m_Mountpoint ) );
    strcpy( encoder->m_Password, LPCSTR( basicSettings->m_Password ) );

    encoder->m_ReconnectSec = atoi( LPCSTR( basicSettings->m_ReconnectSecs ) );
    strcpy( encoder->m_Server, LPCSTR( basicSettings->m_ServerIP ) );
    strcpy( encoder->m_Port, LPCSTR( basicSettings->m_ServerPort ) );

    if ( basicSettings->m_ServerType == "Shoutcast" ) encoder->SetServerType( SERVER_SHOUTCAST );
    if ( basicSettings->m_ServerType == "Icecast2"  ) encoder->SetServerType( SERVER_ICECAST2  );  // TODO: also ICECAST?
    strcpy( encoder->m_ServerTypeName, LPCSTR( basicSettings->m_ServerType ) );
	// m_AsioChannel2?!!
    strcpy( encoder->m_AsioChannel, LPCSTR( basicSettings->m_AsioChannel ) );
    if ( !strcmp( encoder->m_AsioChannel, "" ) ) encoder->m_AsioSelectChannel = 0;
    else encoder->m_AsioSelectChannel = 1;
    ypSettings->UpdateData(TRUE);
    encoder->m_PubServ = ypSettings->m_Public ? 1 : 0;

    strcpy( encoder->m_ServDesc,  LPCSTR( ypSettings->m_StreamDesc  ) );
    strcpy( encoder->m_ServName,  LPCSTR( ypSettings->m_StreamName  ) );
    strcpy( encoder->m_ServGenre, LPCSTR( ypSettings->m_StreamGenre ) );
    strcpy( encoder->m_ServURL,   LPCSTR( ypSettings->m_StreamURL   ) );
    strcpy( encoder->m_ServAIM,   LPCSTR( ypSettings->m_StreamAIM   ) );
    strcpy( encoder->m_ServICQ,   LPCSTR( ypSettings->m_StreamICQ   ) );
    strcpy( encoder->m_ServIRC,   LPCSTR( ypSettings->m_StreamIRC   ) );

    advSettings->UpdateData(TRUE);

    encoder->m_SaveDirectoryFlag = advSettings->m_Savestream;
    encoder->SetSaveDirectory( LPCSTR( advSettings->m_ArchiveDirectory ) );
    encoder->m_LogLevel = atoi( LPCSTR( advSettings->m_Loglevel ) );
    strcpy( encoder->m_LogFile, LPCSTR( advSettings->m_Logfile ) );
    encoder->m_SaveAsWAV = advSettings->m_Savewav;
    encoder->m_ForceDSPRecording = advSettings->m_forceDSP;
}

void CConfig::OnClose () 
{
    CDialog::OnClose();
}

void CConfig::OnOK () 
{
    basicSettings->UpdateData( TRUE );
    ypSettings->UpdateData( TRUE );
    advSettings->UpdateData( TRUE );
    CMainWindow *pwin = (CMainWindow*)parentDialog;
    pwin->ProcessConfigDone( currentEnc, this );
    CDialog::OnOK();
}

void CConfig::OnCancel () 
{
    CMainWindow *pwin = (CMainWindow*)parentDialog;
    pwin->ProcessConfigDone( -1, this );
    CDialog::OnCancel();
}
