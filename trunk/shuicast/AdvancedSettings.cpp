// AdvancedSettings.cpp : implementation file
//

#include "stdafx.h"
#include "altacast.h"
#include "AdvancedSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdvancedSettings dialog


CAdvancedSettings::CAdvancedSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CAdvancedSettings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvancedSettings)
	m_ArchiveDirectory = _T("");
	m_Logfile = _T("");
	m_Loglevel = _T("");
	m_Savestream = FALSE;
	m_Savewav = FALSE;
	m_forceDSP = FALSE;
	//}}AFX_DATA_INIT
}


void CAdvancedSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvancedSettings)
	DDX_Control(pDX, IDC_ARCHIVE_DIRECTORY, m_ArchiveDirectoryCtrl);
	DDX_Control(pDX, IDC_SAVEWAV, m_SavewavCtrl);
	//DDX_Control(pDX, IDC_FORCEDSP, m_forceDSPCtrl);
	DDX_Text(pDX, IDC_ARCHIVE_DIRECTORY, m_ArchiveDirectory);
	DDX_Text(pDX, IDC_LOGFILE, m_Logfile);
	DDX_Text(pDX, IDC_LOGLEVEL, m_Loglevel);
	DDX_Check(pDX, IDC_SAVESTREAM, m_Savestream);
	DDX_Check(pDX, IDC_SAVEWAV, m_Savewav);
	//DDX_Check(pDX, IDC_FORCEDSP, m_forceDSP);
#ifdef MULTIASIO
	DDX_Control(pDX, IDC_TIMEDSTREAM, m_SchedulerEnableCtrl);
	DDX_Check(pDX, IDC_TIMEDSTREAM, m_SchedulerEnable);
#define MAKE_DOW_DDX(DOW, dow) \
	DDX_Control(pDX, IDC_##DOW##ONTIME, m_##dow##OnTimeCtrl); \
	DDX_Control(pDX, IDC_##DOW##OFFTIME, m_##dow##OffTimeCtrl); \
	DDX_Control(pDX, IDC_##DOW##ENABLE, m_##dow##EnabledCtrl); \
	DDX_Check(pDX, IDC_##DOW##ENABLE, m_##dow##Enabled);
	MAKE_DOW_DDX(MON, Monday);
	MAKE_DOW_DDX(TUE, Tuesday);
	MAKE_DOW_DDX(WED, Wednesday);
	MAKE_DOW_DDX(THU, Thursday);
	MAKE_DOW_DDX(FRI, Friday);
	MAKE_DOW_DDX(SAT, Saturday);
	MAKE_DOW_DDX(SUN, Sunday);
	DDX_Control(pDX, IDC_ONLABEL, m_onLabel);
	DDX_Control(pDX, IDC_OFFLABEL, m_offLabel);
#endif
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvancedSettings, CDialog)
	//{{AFX_MSG_MAP(CAdvancedSettings)
	ON_BN_CLICKED(IDC_SAVESTREAM, OnSavestream)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdvancedSettings message handlers
HBRUSH CAdvancedSettings::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	return m_brush;
}

void CAdvancedSettings::EnableDisable()
{
    if (m_Savestream) {
        m_ArchiveDirectoryCtrl.EnableWindow(TRUE);
        m_SavewavCtrl.EnableWindow(TRUE);
    }
    else {
        m_ArchiveDirectoryCtrl.EnableWindow(FALSE);
        m_SavewavCtrl.EnableWindow(FALSE);
    }
}

BOOL CAdvancedSettings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_brush.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	
#ifdef MULTIASIO
#define DOW_HIDE_THEM(dow) \
	m_##dow##OnTimeCtrl.ShowWindow(SW_HIDE); \
	m_##dow##OffTimeCtrl.ShowWindow(SW_HIDE); \
	m_##dow##EnabledCtrl.ShowWindow(SW_HIDE);
	DOW_HIDE_THEM(Monday);
	DOW_HIDE_THEM(Tuesday);
	DOW_HIDE_THEM(Wednesday);
	DOW_HIDE_THEM(Thursday);
	DOW_HIDE_THEM(Friday);
	DOW_HIDE_THEM(Saturday);
	DOW_HIDE_THEM(Sunday);
	m_SchedulerEnableCtrl.ShowWindow(SW_HIDE);
	m_onLabel.ShowWindow(SW_HIDE);
	m_offLabel.ShowWindow(SW_HIDE);
#endif
#ifdef EDCASTSTANDALONE
	m_forceDSP = false;
	m_forceDSPCtrl.ShowWindow(SW_HIDE);
#endif
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAdvancedSettings::OnSavestream() 
{
    UpdateData(TRUE);
    EnableDisable();	
}
