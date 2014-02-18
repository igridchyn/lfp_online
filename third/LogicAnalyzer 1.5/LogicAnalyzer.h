
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"
#include "Driver.h"
#include "Port.h"
#include "Serial.h"

class CLogicAnalyzerApp : public CWinApp
{
public:
	CLogicAnalyzerApp();

	CFunctions mi_Func;
	CDriver    mi_Driver;
	CPort      mi_Port;
	CSerial    mi_Serial;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogicAnalyzerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CLogicAnalyzerApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

