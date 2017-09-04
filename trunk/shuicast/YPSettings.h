#pragma once

// YPSettings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CYPSettings dialog

class CYPSettings : public CDialog
{
// Construction
public:
	CYPSettings(CWnd* pParent = NULL);   // standard constructor

    void EnableDisable();

// Dialog Data
	//{{AFX_DATA(CYPSettings)
	enum { IDD = IDD_PROPPAGE_LARGE1 };
	CEdit	m_StreamIRCCtrl;
	CEdit	m_StreamICQCtrl;
	CEdit	m_StreamAIMCtrl;
	CEdit	m_StreamURLCtrl;
	CEdit	m_StreamNameCtrl;
	CEdit	m_StreamGenreCtrl;
	CEdit	m_StreamDescCtrl;
	BOOL	m_Public;
	CString	m_StreamDesc;
	CString	m_StreamGenre;
	CString	m_StreamName;
	CString	m_StreamURL;
	CString	m_StreamICQ;
	CString	m_StreamIRC;
	CString	m_StreamAIM;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CYPSettings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CBrush m_brush;
	// Generated message map functions
	//{{AFX_MSG(CYPSettings)
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPublic();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
