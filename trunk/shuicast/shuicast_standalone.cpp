// shuicast_standalone.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "shuicast_standalone.h"
#include "MainWindow.h"
#ifdef USE_NEW_CONFIG
#include "shuicast_config.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMainWindow *mainWindow;
//CWnd *myWin;

char    logPrefix[255] = "shuicaststandalone";

/////////////////////////////////////////////////////////////////////////////
// CShuiCastStandalone

BEGIN_MESSAGE_MAP(CShuiCastStandaloneApp, CWinApp)
	//{{AFX_MSG_MAP(CShuiCastStandaloneApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShuiCastStandaloneApp construction

void inputMetadataCallback(void *gbl, void *pValue)
{
    CEncoder *encoder = (CEncoder*)gbl;
    mainWindow->inputMetadataCallback( encoder->encoderNumber, pValue, FILE_LINE );
}

void outputStatusCallback(void *gbl, void *pValue)
{
    CEncoder *encoder = (CEncoder*)gbl;
    mainWindow->outputStatusCallback( encoder->encoderNumber, pValue, FILE_LINE );
    mainWindow->outputMountCallback( encoder->encoderNumber, encoder->m_Mountpoint );
    //mainWindow->outputChannelCallback( encoder->encoderNumber, encoder->m_AsioChannel );
}

void writeBytesCallback(void *gbl, void *pValue)
{
    CEncoder *encoder = (CEncoder*)gbl;
    mainWindow->writeBytesCallback( encoder->encoderNumber, pValue );
}

void outputServerNameCallback(void *gbl, void *pValue)
{
    CEncoder *encoder = (CEncoder*)gbl;
    mainWindow->outputServerNameCallback( encoder->encoderNumber, pValue );
}

void outputBitrateCallback(void *gbl, void *pValue)
{
    CEncoder *encoder = (CEncoder*)gbl;
    mainWindow->outputBitrateCallback( encoder->encoderNumber, pValue );
}

void outputStreamURLCallback(void *gbl, void *pValue)
{
    CEncoder *encoder = (CEncoder*)gbl;
    mainWindow->outputStreamURLCallback( encoder->encoderNumber, pValue );
}

int shuicast_init ( CEncoder *encoder )
{
	encoder->SetServerStatusCallback( outputStatusCallback );
	encoder->SetGeneralStatusCallback( NULL );
	encoder->SetWriteBytesCallback( writeBytesCallback );
	encoder->SetBitrateCallback( outputBitrateCallback );
	encoder->SetServerNameCallback( outputServerNameCallback );
	encoder->SetDestURLCallback( outputStreamURLCallback );
    //strcpy( encoder->gConfigFileName, ".\\shuicast_standalone" );
    encoder->ReadConfigFile();
	return 1;
}

CShuiCastStandaloneApp::CShuiCastStandaloneApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CShuiCastStandaloneApp object

CShuiCastStandaloneApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CShuiCastStandaloneApp initialization
/*
bool done;
INT  nResult;

int RunModalWindow(HWND hwndDialog,HWND hwndParent)
{
  if(hwndParent != NULL)
    EnableWindow(hwndParent,FALSE);

  MSG msg;
  
  for(done=false;done==false;WaitMessage())
  {
    while(PeekMessage(&msg,0,0,0,PM_REMOVE))
    {
      if(msg.message == WM_QUIT)
      {
        done = true;
        PostMessage(NULL,WM_QUIT,0,0);
        break;
      }

      if(!IsDialogMessage(hwndDialog,&msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  if(hwndParent != NULL)
    EnableWindow(hwndParent,TRUE);

  DestroyWindow(hwndDialog);

  return nResult;
}
*/

void CShuiCastStandaloneApp::SetMainAfxWin(CWnd *pwnd) {
    m_pMainWnd = pwnd;
}

BOOL CShuiCastStandaloneApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef USE_NEW_CONFIG
    LoadConfigs(".", "shuicaststandalone");
	const saneConfig * sc =	saneLoadConfigs(_T("shuicaststandalone_0.cfg"));
	/*
	OutputDebugString("AppName: ");
	OutputDebugString(sc->appName);
	OutputDebugString("\nCurrentDir: ");
	OutputDebugString(sc->currentDir);
	OutputDebugString("\nExeName: ");
	OutputDebugString(sc->exename);
	OutputDebugString("\nFirstConfig: ");
	OutputDebugString(sc->firstConfig);
	OutputDebugString("\nFirstWritableFolder: ");
	OutputDebugString(sc->firstWritableFolder);
	OutputDebugString("\n");
	*/
#else
	char currentDir[MAX_PATH] = ".";
	char tmpfile[MAX_PATH] = "";
	wsprintf(tmpfile, "%s\\.tmp", currentDir);

	FILE *filep = fopen(tmpfile, "w");
	if (filep == 0) {
		char path[MAX_PATH] = "";

		SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
		strcpy(currentDir, path);
	}
	else {
		fclose(filep);
		_unlink(tmpfile);
	}

    LoadConfigs(currentDir, "shuicaststandalone");
#endif

    mainWindow = new CMainWindow(m_pMainWnd);
    theApp.SetMainAfxWin(mainWindow);
    mainWindow->InitializeWindow();
    //mainWindow->Create((UINT)IDD_SHUICAST, this);
	mainWindow->DoModal();

	return FALSE;
}
