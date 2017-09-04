#pragma once

// About.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog

class CAbout : public CDialog
{
// Construction
public:
	CAbout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAbout)
	enum { IDD = IDD_ABOUT };
	CString	m_Version;
	CString m_Configpath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAbout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAbout)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
