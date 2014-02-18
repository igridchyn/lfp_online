
#pragma once

class CLogicAnalyzerDlg : public CDialog
{
public:
	CLogicAnalyzerDlg(CWnd* pParent = NULL);	// standard constructor

	//{{AFX_DATA(CLogicAnalyzerDlg)
	enum { IDD = IDD_LOGICANALYZER_DIALOG };
	CStatic	mCtrl_StaticStatusbar;
	CEdit	mCtrl_EditAnalyzeEnd;
	CEdit	mCtrl_EditAnalyzeStart;
	CComboBox	mCtrl_ComboDrawMode;
	CComboBox	mCtrl_ComboDatabits;
	CComboBox	mCtrl_ComboParity;
	CComboBox	mCtrl_ComboBaudrate;
	CComboBox	mCtrl_ComboStopbits;
	CComboBox	mCtrl_ComboProtocol;
	CComboBox	mCtrl_ComboPort;
	CEdit	mCtrl_EditHtmlHeading;
	CButton	mCtrl_CheckDeleteOldFiles;
	CStatic	mCtrl_IconEggTimer2;
	CEdit	mCtrl_EditRasterUnit;
	CEdit	mCtrl_EditIdleTime;
	CButton	mCtrl_CheckAutoUnits;
	CButton	mCtrl_BtnCapture;
	CButton	mCtrl_BtnAnalyze;
	CButton	mCtrl_BtnMeasureSpeed;
	CStatic	mCtrl_IconEggTimer;
	CButton	mCtrl_CheckCtrl;
	CStatic	mCtrl_BitStatus4;
	CStatic	mCtrl_BitStatus3;
	CStatic	mCtrl_BitStatus2;
	CStatic	mCtrl_BitStatus1;
	CStatic	mCtrl_BitStatus0;
	CStatic	mCtrl_BitData7;
	CStatic	mCtrl_BitData6;
	CStatic	mCtrl_BitData5;
	CStatic	mCtrl_BitData4;
	CStatic	mCtrl_BitData3;
	CStatic	mCtrl_BitData2;
	CStatic	mCtrl_BitData1;
	CStatic	mCtrl_BitData0;
	CStatic	mCtrl_BitCtrl3;
	CStatic	mCtrl_BitCtrl2;
	CStatic	mCtrl_BitCtrl1;
	CStatic	mCtrl_BitCtrl0;
	CStatic	mCtrl_LedStatus4;
	CStatic	mCtrl_LedStatus3;
	CStatic	mCtrl_LedStatus2;
	CStatic	mCtrl_LedStatus1;
	CStatic	mCtrl_LedStatus0;
	CStatic	mCtrl_LedCtrl3;
	CStatic	mCtrl_LedCtrl2;
	CStatic	mCtrl_LedCtrl1;
	CStatic	mCtrl_LedCtrl0;
	CStatic	mCtrl_LedData7;
	CStatic	mCtrl_LedData6;
	CStatic	mCtrl_LedData5;
	CStatic	mCtrl_LedData4;
	CStatic	mCtrl_LedData3;
	CStatic	mCtrl_LedData2;
	CStatic	mCtrl_LedData1;
	CStatic	mCtrl_LedData0;
	CButton	mCtrl_CheckData;
	CButton	mCtrl_CheckStatus;
	CStatic	mCtrl_StaticCaptured;
	CEdit	mCtrl_EditMemSize;
	CStatic	mCtrl_StaticDriver;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogicAnalyzerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	// Generated message map functions
	//{{AFX_MSG(CLogicAnalyzerDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnSpeed();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnCheckBoxPorts();
	afx_msg void OnBtnAnalyze();
	afx_msg void OnBtnCapture();
	afx_msg void OnCheckAutoUnits();
	afx_msg void OnSelchangeComboPort();
	afx_msg void OnChangeEditMemsize();
	afx_msg void OnSelchangeComboProtocol();
	afx_msg void OnBtnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	struct kSetting
	{
		WORD  u16_BasePort;
		BOOL  u32_PortMask;
		DWORD u32_MemSize;
	};

	kSetting mk_Settg;

	void SetLeds(BOOL b_Show);
	void ShowProgress();
	void ShowStatus();
	void EnableGUI();

	HICON mh_Icon;
	HICON mh_LedIcon[2][2];

	BOOL  mb_Capturing;
	BOOL  mb_Analyzing;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


