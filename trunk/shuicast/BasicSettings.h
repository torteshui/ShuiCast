#pragma once

// BasicSettings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBasicSettings dialog

#define SLAB_NONE 0
#define SLAB_JOINT 1
#define SLAB_PARAMETRIC 2

class CBasicSettings : public CDialog
{

public:
    CBasicSettings ( CWnd *pParent = NULL );   // standard constructor

    // Dialog Data
    //{{AFX_DATA(CBasicSettings)
    enum { IDD = IDD_PROPPAGE_LARGE };
    CButton   m_JointStereoCtrl;
    CButton   m_UseBitrateCtrl;
    CEdit     m_QualityCtrl;
    CEdit     m_BitrateCtrl;
    CEdit     m_ChannelsCtrl;
    CComboBox m_ServerTypeCtrl;
    CComboBox m_EncoderTypeCtrl;
    CString   m_Bitrate;
    CString   m_Channels;
    CString   m_EncoderType;
    CString   m_Mountpoint;
    CString   m_Password;
    CString   m_Quality;
    CString   m_ReconnectSecs;
    CString   m_Samplerate;
    CString   m_ServerIP;
    CString   m_ServerPort;
    CString   m_ServerType;
    CString   m_LamePreset;
    BOOL      m_UseBitrate;
    BOOL      m_JointStereo;
    CStatic   m_JointStereoLabelCtrl;
    CStatic   m_ParaStereoLabelCtrl;
    BOOL      m_AsioSelectChannel;
    CString   m_AsioChannel;
    CComboBox m_AsioChannelCtrl;
    CString   m_AsioChannel2;
    CComboBox m_AsioChannel2Ctrl;
    CEdit     m_AttenuationCtrl;
    CString   m_Attenuation;
    CStatic   m_AsioGroupBox;
    //}}AFX_DATA
    void SetStereoLabels ( int val );
    void UpdateFields    ();

    // ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBasicSettings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	CBrush m_brush;
	// Generated message map functions
	//{{AFX_MSG(CBasicSettings)
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeEncoderType();
	afx_msg void OnSelchangeAttenuation();
	afx_msg void OnSelendokEncoderType();
	afx_msg void OnUseBitrate();
	afx_msg void OnJointStereo();
	afx_msg void OnBitrate();
	afx_msg void OnChannels();
	afx_msg void OnSelchangeAsio();
	afx_msg void OnSelendokAsio();
	afx_msg void OnSelchangeAsio2();
	afx_msg void OnSelendokAsio2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
