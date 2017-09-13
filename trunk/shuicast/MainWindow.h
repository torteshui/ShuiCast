#pragma once

// MainWindow.h : header file
//
#include <afxcmn.h>
#include "Config.h"
#include "EditMetadata.h"
#include "About.h"
#include <afxdisp.h>
#include <afxext.h>
#include "FlexMeters.h"

class CSystemTray;

void setMetadata(char *metadata);
void setMetadataFromMediaPlayer(char *metadata);
#ifdef USE_NEW_CONFIG
void LoadConfigs(char *currentDir, char *subdir);
#else
void LoadConfigs(char *currentDir, char *logFile);
#endif
int initializeshuicast();
bool LiveRecordingCheck();
bool HaveEncoderAlwaysDSP();
void UpdatePeak(int rmsL, int rmsR, int peakL, int peakR);
int handleAllOutput(float *samples, int nsamples, int nchannels, int in_samplerate);
void addComment(char *comment);
void freeComment();

int getLastX();
int getLastY();
void setLastX(int x);
void setLastY(int y);
void setLiveRecFlag(int live);
void writeMainConfig();
void setAuto(int flag);

enum VUSTATE { VU_ON, VU_OFF, VU_SWITCHOFF };

#define MAX_ENCODERS 10

/////////////////////////////////////////////////////////////////////////////
// CMainWindow dialog

class CMainWindow : public CDialog
{
// Construction
public:
	CMainWindow(CWnd* pParent = NULL);   // standard constructor
    ~CMainWindow();
// Dialog Data
	//{{AFX_DATA(CMainWindow)
	enum { IDD = IDD_SHUICAST };
	CComboBox	m_RecCardsCtrl;
	CStatic	m_OnOff;
	CStatic	m_MeterPeak;
	CStatic	m_MeterRMS;
	CButton	m_AutoConnectCtrl;
	CButton	m_startMinimizedCtrl;
	CButton	m_LimiterCtrl;
	CSliderCtrl	m_RecVolumeCtrl; //** sliders!!
	CComboBox	m_RecDevicesCtrl;
	CComboBox	m_AsioRateCtrl;
	CButton	m_ConnectCtrl;
	CButton	m_LiveRecCtrl;
	CListCtrl	m_Encoders;
	CString	m_Bitrate;
	CString	m_Destination;
	CString	m_Bandwidth;
	CString	m_Metadata; //** textbox!!
	CString	m_ServerDescription;
	BOOL	m_LiveRec;
	CString	m_RecDevices;
	CString	m_RecCards;
	CString m_AsioRate;
	int		m_RecVolume; //** sliders!!
	BOOL	m_AutoConnect;
	BOOL	m_startMinimized;
	BOOL	m_Limiter;
	CString	m_StaticStatus;
	CStatic m_StaticStatusCtrl;
	CSliderCtrl m_limitdbCtrl;
	int m_limitdb;
	CSliderCtrl m_gaindbCtrl;
	int m_gaindb;
	CSliderCtrl m_limitpreCtrl;
	int m_limitpre;
	CString m_staticLimitdb;
	CString m_staticGaindb;
	CString m_staticLimitpre;
	//}}AFX_DATA
    afx_msg void OnRestore();
	afx_msg void OnExit();

	void OnSTRestore();
	void OnSTExit();

    void generalStatusCallback(void *pValue, char *source, int line);
    void inputMetadataCallback(int enc, void *pValue, char *source, int line);
    void outputStatusCallback(int enc, void *pValue, char *source, int line, bool bSendToLog = true);
    void outputMountCallback(int enc, void *pValue);
    void outputChannelCallback(int enc, void *pValue);
    void writeBytesCallback(int enc, void *pValue);
    void outputServerNameCallback(int enc, void *pValue);
    void outputBitrateCallback(int enc, void *pValue);
    void outputStreamURLCallback(int enc, void *pValue);
    void stopshuicast();
    int startshuicast(int enc);
    void EditConfig(int enc);
    void UpdatePeak(int peakL, int peakR);
	void SetupTrayIcon();
	void SetupTaskBarButton();
	void OnSysCommand(UINT nID, LPARAM lParam);

    int m_iItemOnMenu;
    int m_SpecificEncoder;
    long gWindowHeight;
    int m_CurrentInput;
	int m_CurrentInputCard;
	int m_numChan;
    CConfig *configDialog;
    CEditMetadata   *editMetadata;
    CBitmap liveRecOn;
    CBitmap liveRecOff;

    CAbout  *aboutBox;
    int reconnectTimerId;
    int autoconnectTimerId;
    int metadataTimerId;
    void ProcessConfigDone(int enc, CConfig *pConfig);
    void ProcessEditMetadataDone(CEditMetadata *pConfig);
    void CleanUp();
	void DoConnect();
    void InitializeWindow();
    char    m_currentDir[MAX_PATH];
    CFlexMeters flexmeters;
    double bias;
    int m_VUStatus;

	HICON hIcon_;
	bool bMinimized_;
	CSystemTray* pTrayIcon_;
	int nTrayNotificationMsg_;
	bool visible;
protected:
	virtual void DoSysCommand(UINT nID, LPARAM lParam);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainWindow)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:
	// Generated message map functions
	//{{AFX_MSG(CMainWindow)
	afx_msg void OnSelchangeRecdevices();
	afx_msg void OnSelchangeReccards();
	afx_msg void OnSelchangeAsioRate();
	afx_msg void OnConnect();
	afx_msg void OnAddEncoder();
	afx_msg void OnDblclkEncoders(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnRclickEncoders(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPopupConfigure();
	afx_msg void OnPopupConnect();
	afx_msg void OnLiverec();
	afx_msg void OnLimiter();
	afx_msg void OnStartMinimized();
	afx_msg void OnPopupDelete();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnManualeditMetadata();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnAboutAbout();
	afx_msg void OnAboutHelp();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnMeter();
	afx_msg void OnKeydownEncoders(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButton1();
	virtual void OnCancel();
	afx_msg void OnSetfocusEncoders(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT gotShowWindow(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT startMinimized(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnItemchangedEncoders(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonEqu();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
