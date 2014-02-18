
#include "stdafx.h"
#include "LogicAnalyzer.h"
#include "LogicAnalyzerDlg.h"


BEGIN_MESSAGE_MAP(CLogicAnalyzerApp, CWinApp)
	//{{AFX_MSG_MAP(CLogicAnalyzerApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

CLogicAnalyzerApp::CLogicAnalyzerApp()
{
}

CLogicAnalyzerApp gi_App;


BOOL CLogicAnalyzerApp::InitInstance()
{
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	if (!gi_App.mi_Func.Initialize()) // Shows error messages
		return FALSE;

	CString s_Admin = "\nAttention: The very first time this program must be started 'As Administrator',\notherwise the driver cannot be installed!";
	CString s_ErrMsg;
	DWORD u32_ApiErr;
	switch (mi_Driver.Initialize(&u32_ApiErr))
	{
	case INIT_COPY_ERR:
		gi_App.mi_Func.ShowErrorMsg(u32_ApiErr, " while copying the driver file to Windows driver directory", s_Admin);
		return FALSE;

	case INIT_LOAD_ERR:
		gi_App.mi_Func.ShowErrorMsg(u32_ApiErr, " while loading the driver", s_Admin);
		return FALSE;

	case INIT_ALLOWIO_ERR:
		gi_App.mi_Func.ShowErrorMsg(u32_ApiErr, " while allowing IO access to all ports", s_Admin);
		return FALSE;
	}

	CLogicAnalyzerDlg i_MainDialog;
	m_pMainWnd = &i_MainDialog;
	i_MainDialog.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
