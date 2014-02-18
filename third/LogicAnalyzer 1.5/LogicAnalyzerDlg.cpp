
#include "stdafx.h"
#include "LogicAnalyzer.h"
#include "LogicAnalyzerDlg.h"
#include "Port.h"

extern CLogicAnalyzerApp gi_App;

CLogicAnalyzerDlg::CLogicAnalyzerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLogicAnalyzerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLogicAnalyzerDlg)
	//}}AFX_DATA_INIT
	mh_Icon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	mb_Capturing = FALSE;
	mb_Analyzing = FALSE;

	HINSTANCE h_Inst = gi_App.m_hInstance;
	// LoadIcon() is so primitive that it cannot load 16x16 Pixel Icons!
	// mh_LedIcon[b_Enabled][b_Status]
	mh_LedIcon[0][0] = (HICON) LoadImage(h_Inst, MAKEINTRESOURCE(IDI_ICON_LED_GREY),  IMAGE_ICON, 0, 0, 0);
	mh_LedIcon[0][1] = mh_LedIcon[0][0];
	mh_LedIcon[1][0] = (HICON) LoadImage(h_Inst, MAKEINTRESOURCE(IDI_ICON_LED_RED),   IMAGE_ICON, 0, 0, 0);
	mh_LedIcon[1][1] = (HICON) LoadImage(h_Inst, MAKEINTRESOURCE(IDI_ICON_LED_GREEN), IMAGE_ICON, 0, 0, 0);
}

void CLogicAnalyzerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogicAnalyzerDlg)
	DDX_Control(pDX, IDC_STATIC_STATUSBAR, mCtrl_StaticStatusbar);
	DDX_Control(pDX, IDC_EDIT_END_TIME, mCtrl_EditAnalyzeEnd);
	DDX_Control(pDX, IDC_EDIT_START_TIME, mCtrl_EditAnalyzeStart);
	DDX_Control(pDX, IDC_COMBO_DRAWMODE, mCtrl_ComboDrawMode);
	DDX_Control(pDX, IDC_COMBO_DATABITS, mCtrl_ComboDatabits);
	DDX_Control(pDX, IDC_COMBO_PARITY, mCtrl_ComboParity);
	DDX_Control(pDX, IDC_COMBO_BAUDRATE, mCtrl_ComboBaudrate);
	DDX_Control(pDX, IDC_COMBO_STOPBITS, mCtrl_ComboStopbits);
	DDX_Control(pDX, IDC_COMBO_PROTOCOL, mCtrl_ComboProtocol);
	DDX_Control(pDX, IDC_COMBO_PORT, mCtrl_ComboPort);
	DDX_Control(pDX, IDC_EDIT_HTML_HEADING, mCtrl_EditHtmlHeading);
	DDX_Control(pDX, IDC_CHECK_DEL_OLD_FILES, mCtrl_CheckDeleteOldFiles);
	DDX_Control(pDX, IDC_STATIC_EGGTIMER2, mCtrl_IconEggTimer2);
	DDX_Control(pDX, IDC_EDIT_RASTER_UNIT, mCtrl_EditRasterUnit);
	DDX_Control(pDX, IDC_EDIT_IDLE_TIME, mCtrl_EditIdleTime);
	DDX_Control(pDX, IDC_CHECK_AUTO_UNITS, mCtrl_CheckAutoUnits);
	DDX_Control(pDX, IDC_BTN_CAPTURE, mCtrl_BtnCapture);
	DDX_Control(pDX, IDC_BTN_ANALYZE, mCtrl_BtnAnalyze);
	DDX_Control(pDX, IDC_BTN_SPEED, mCtrl_BtnMeasureSpeed);
	DDX_Control(pDX, IDC_STATIC_EGGTIMER, mCtrl_IconEggTimer);
	DDX_Control(pDX, IDC_CHECK_USE_CONTROL, mCtrl_CheckCtrl);
	DDX_Control(pDX, IDC_STATIC_STATUS_4, mCtrl_BitStatus4);
	DDX_Control(pDX, IDC_STATIC_STATUS_3, mCtrl_BitStatus3);
	DDX_Control(pDX, IDC_STATIC_STATUS_2, mCtrl_BitStatus2);
	DDX_Control(pDX, IDC_STATIC_STATUS_1, mCtrl_BitStatus1);
	DDX_Control(pDX, IDC_STATIC_STATUS_0, mCtrl_BitStatus0);
	DDX_Control(pDX, IDC_STATIC_DATA_7, mCtrl_BitData7);
	DDX_Control(pDX, IDC_STATIC_DATA_6, mCtrl_BitData6);
	DDX_Control(pDX, IDC_STATIC_DATA_5, mCtrl_BitData5);
	DDX_Control(pDX, IDC_STATIC_DATA_4, mCtrl_BitData4);
	DDX_Control(pDX, IDC_STATIC_DATA_3, mCtrl_BitData3);
	DDX_Control(pDX, IDC_STATIC_DATA_2, mCtrl_BitData2);
	DDX_Control(pDX, IDC_STATIC_DATA_1, mCtrl_BitData1);
	DDX_Control(pDX, IDC_STATIC_DATA_0, mCtrl_BitData0);
	DDX_Control(pDX, IDC_STATIC_CTRL_3, mCtrl_BitCtrl3);
	DDX_Control(pDX, IDC_STATIC_CTRL_2, mCtrl_BitCtrl2);
	DDX_Control(pDX, IDC_STATIC_CTRL_1, mCtrl_BitCtrl1);
	DDX_Control(pDX, IDC_STATIC_CTRL_0, mCtrl_BitCtrl0);
	DDX_Control(pDX, IDC_LED_STATUS_4, mCtrl_LedStatus4);
	DDX_Control(pDX, IDC_LED_STATUS_3, mCtrl_LedStatus3);
	DDX_Control(pDX, IDC_LED_STATUS_2, mCtrl_LedStatus2);
	DDX_Control(pDX, IDC_LED_STATUS_1, mCtrl_LedStatus1);
	DDX_Control(pDX, IDC_LED_STATUS_0, mCtrl_LedStatus0);
	DDX_Control(pDX, IDC_LED_CTRL_3, mCtrl_LedCtrl3);
	DDX_Control(pDX, IDC_LED_CTRL_2, mCtrl_LedCtrl2);
	DDX_Control(pDX, IDC_LED_CTRL_1, mCtrl_LedCtrl1);
	DDX_Control(pDX, IDC_LED_CTRL_0, mCtrl_LedCtrl0);
	DDX_Control(pDX, IDC_LED_DATA_7, mCtrl_LedData7);
	DDX_Control(pDX, IDC_LED_DATA_6, mCtrl_LedData6);
	DDX_Control(pDX, IDC_LED_DATA_5, mCtrl_LedData5);
	DDX_Control(pDX, IDC_LED_DATA_4, mCtrl_LedData4);
	DDX_Control(pDX, IDC_LED_DATA_3, mCtrl_LedData3);
	DDX_Control(pDX, IDC_LED_DATA_2, mCtrl_LedData2);
	DDX_Control(pDX, IDC_LED_DATA_1, mCtrl_LedData1);
	DDX_Control(pDX, IDC_LED_DATA_0, mCtrl_LedData0);
	DDX_Control(pDX, IDC_CHECK_USE_DATA, mCtrl_CheckData);
	DDX_Control(pDX, IDC_CHECK_USE_STATUS, mCtrl_CheckStatus);
	DDX_Control(pDX, IDC_STATIC_CAPTURED, mCtrl_StaticCaptured);
	DDX_Control(pDX, IDC_EDIT_MEMSIZE, mCtrl_EditMemSize);
	DDX_Control(pDX, IDC_STATIC_DRIVER, mCtrl_StaticDriver);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLogicAnalyzerDlg, CDialog)
	//{{AFX_MSG_MAP(CLogicAnalyzerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SPEED, OnBtnSpeed)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_USE_DATA, OnCheckBoxPorts)
	ON_BN_CLICKED(IDC_BTN_ANALYZE, OnBtnAnalyze)
	ON_BN_CLICKED(IDC_BTN_CAPTURE, OnBtnCapture)
	ON_BN_CLICKED(IDC_CHECK_AUTO_UNITS, OnCheckAutoUnits)
	ON_CBN_SELCHANGE(IDC_COMBO_PORT, OnSelchangeComboPort)
	ON_EN_CHANGE(IDC_EDIT_MEMSIZE, OnChangeEditMemsize)
	ON_CBN_SELCHANGE(IDC_COMBO_PROTOCOL, OnSelchangeComboProtocol)
	ON_BN_CLICKED(IDC_CHECK_USE_STATUS, OnCheckBoxPorts)
	ON_BN_CLICKED(IDC_CHECK_USE_CONTROL, OnCheckBoxPorts)
	ON_BN_CLICKED(IDC_BTN_HELP, OnBtnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CLogicAnalyzerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(mh_Icon, TRUE);	 // Set big icon
	SetIcon(mh_Icon, FALSE); // Set small icon

	mCtrl_ComboPort.     SetCurSel(0); // Port 378
	mCtrl_ComboProtocol. SetCurSel(0); // no Serial protocol
	mCtrl_StaticDriver.  SetWindowText(CString("Driver: ") + gi_App.mi_Driver.GetDriverName());
	mCtrl_EditMemSize.   SetWindowText("8");
	mCtrl_EditRasterUnit.SetWindowText("20 us");
	mCtrl_EditIdleTime.  SetWindowText("1 ms");
	mCtrl_CheckAutoUnits.SetCheck(TRUE);

	// Beep via PC speaker
	#ifndef _DEBUG
		gi_App.mi_Port.BeepSpeaker(1000, 70);
		gi_App.mi_Port.BeepSpeaker(1300, 70);
		gi_App.mi_Port.BeepSpeaker(1600, 70);
		gi_App.mi_Port.BeepSpeaker(2000, 70);
	#endif

	OnCheckBoxPorts();
	OnSelchangeComboPort();
	OnChangeEditMemsize();
	EnableGUI();

	SetTimer(ID_TIMER_GUI_UPDATE, 500, 0);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CLogicAnalyzerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width()  - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, mh_Icon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CLogicAnalyzerDlg::OnQueryDragIcon()
{
	return (HCURSOR) mh_Icon;
}

// Avoid that hitting Enter closes the window
void CLogicAnalyzerDlg::OnOK()
{
}

void CLogicAnalyzerDlg::OnTimer(UINT nIDEvent) 
{
	CDialog::OnTimer(nIDEvent);

	switch (nIDEvent)
	{
		case ID_TIMER_GUI_UPDATE:
		{
			if (mb_Capturing) ShowProgress();
			else              SetLeds(TRUE);

			if (mb_Analyzing) ShowStatus();
			break;
		}
		case ID_TIMER_CAPTURE_FINISHED:
		{
			KillTimer(nIDEvent);
			ShowProgress(); // show the very last value
			mb_Capturing = FALSE;
			EnableGUI();

			double d_AnalyzeStart, d_AnalyzeEnd;
			gi_App.mi_Port.GetCaptureTime(&d_AnalyzeStart, &d_AnalyzeEnd);
			mCtrl_EditAnalyzeStart.SetWindowText(gi_App.mi_Func.FormatExactTime(d_AnalyzeStart));
			mCtrl_EditAnalyzeEnd.  SetWindowText(gi_App.mi_Func.FormatExactTime(d_AnalyzeEnd));

			DWORD u32_Err = gi_App.mi_Port.GetCaptureLastError();
			if (u32_Err)
			{
				gi_App.mi_Func.ShowErrorMsg(u32_Err, "while capturing data");
			}
			break;
		}
		case ID_TIMER_ANALYZE_FINISHED:
		{
			KillTimer(nIDEvent);
			mb_Analyzing = FALSE;
			mCtrl_StaticStatusbar.SetWindowText("");
			EnableGUI();

			if (mCtrl_CheckAutoUnits.GetCheck())
			{
				// Write the calculated values into the GUI
				double d_Unit, d_Idle;
				gi_App.mi_Serial.GetUnits(&d_Unit, &d_Idle);
				CString s_Unit = gi_App.mi_Func.FormatInterval(d_Unit);
				CString s_Idle = gi_App.mi_Func.FormatInterval(d_Idle);
				s_Unit.Replace("µs", "us");
				s_Idle.Replace("µs", "us");
				mCtrl_EditRasterUnit.SetWindowText(s_Unit);
				mCtrl_EditIdleTime.  SetWindowText(s_Idle);
			}
			break;
		}
	}
}

// ===================================================================
// ===================================================================


void CLogicAnalyzerDlg::OnSelchangeComboPort() 
{
	CString s_Port;
	mCtrl_ComboPort.GetWindowText(s_Port);

	DWORD u32_Port;
	_stscanf(s_Port, "%x", &u32_Port);

	mk_Settg.u16_BasePort = (WORD)u32_Port;

	gi_App.mi_Port.InitializePorts(mk_Settg.u16_BasePort);
}

void CLogicAnalyzerDlg::OnCheckBoxPorts() 
{
	mk_Settg.u32_PortMask = 0;
	if (mCtrl_CheckData.  GetCheck()) mk_Settg.u32_PortMask |= DATA_MASK;
	if (mCtrl_CheckStatus.GetCheck()) mk_Settg.u32_PortMask |= STATUS_MASK;
	if (mCtrl_CheckCtrl.  GetCheck()) mk_Settg.u32_PortMask |= CTRL_MASK;

	mCtrl_BitData0.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData1.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData2.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData3.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData4.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData5.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData6.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_BitData7.EnableWindow(mk_Settg.u32_PortMask & DATA_MASK);

	mCtrl_BitStatus0.EnableWindow(mk_Settg.u32_PortMask & STATUS_MASK);
	mCtrl_BitStatus1.EnableWindow(mk_Settg.u32_PortMask & STATUS_MASK);
	mCtrl_BitStatus2.EnableWindow(mk_Settg.u32_PortMask & STATUS_MASK);
	mCtrl_BitStatus3.EnableWindow(mk_Settg.u32_PortMask & STATUS_MASK);
	mCtrl_BitStatus4.EnableWindow(mk_Settg.u32_PortMask & STATUS_MASK);

	mCtrl_BitCtrl0.EnableWindow(mk_Settg.u32_PortMask & CTRL_MASK);
	mCtrl_BitCtrl1.EnableWindow(mk_Settg.u32_PortMask & CTRL_MASK);
	mCtrl_BitCtrl2.EnableWindow(mk_Settg.u32_PortMask & CTRL_MASK);
	mCtrl_BitCtrl3.EnableWindow(mk_Settg.u32_PortMask & CTRL_MASK);
}

void CLogicAnalyzerDlg::OnChangeEditMemsize() 
{
	CString s_MemSize;
	mCtrl_EditMemSize.GetWindowText(s_MemSize);

	DWORD u32_Size = _ttol(s_MemSize);
	mk_Settg.u32_MemSize = u32_Size *1024*1024;
}

// Enables / Disables the GUI corresponding to the current action
void CLogicAnalyzerDlg::EnableGUI()
{
	BOOL b_Auto = mCtrl_CheckAutoUnits.GetCheck();

	mCtrl_ComboPort.          EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_EditMemSize.        EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_BtnMeasureSpeed.    EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_CheckData.          EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_CheckStatus.        EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_CheckCtrl.          EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_CheckDeleteOldFiles.EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_EditHtmlHeading.    EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_CheckAutoUnits.     EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_EditAnalyzeStart.   EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_EditAnalyzeEnd.     EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_EditRasterUnit.     EnableWindow(!mb_Capturing && !mb_Analyzing && !b_Auto);
	mCtrl_EditIdleTime.       EnableWindow(!mb_Capturing && !mb_Analyzing && !b_Auto);

	mCtrl_IconEggTimer.       ShowWindow  (mb_Capturing);
	mCtrl_IconEggTimer2.      ShowWindow  (mb_Analyzing);
	mCtrl_BtnCapture.         EnableWindow(!mb_Analyzing);
	mCtrl_BtnAnalyze.         EnableWindow(!mb_Capturing);
	mCtrl_BtnCapture.         SetWindowText(mb_Capturing ? "Stop Capture":"Start Capture");
	mCtrl_BtnAnalyze.         SetWindowText(mb_Analyzing ? "Stop Analyze":"Analyze Data");

	BOOL b_Serial = (mCtrl_ComboProtocol.GetCurSel() >  CSerial::E_Proto_None);
	BOOL b_Async  = (mCtrl_ComboProtocol.GetCurSel() == CSerial::E_Proto_Async);
	mCtrl_ComboProtocol.EnableWindow(!mb_Capturing && !mb_Analyzing);
	mCtrl_ComboParity.  EnableWindow(!mb_Capturing && !mb_Analyzing && b_Async);
	mCtrl_ComboDatabits.EnableWindow(!mb_Capturing && !mb_Analyzing && b_Async);
	mCtrl_ComboStopbits.EnableWindow(!mb_Capturing && !mb_Analyzing && b_Async);
	mCtrl_ComboBaudrate.EnableWindow(!mb_Capturing && !mb_Analyzing && b_Async);
	mCtrl_ComboDrawMode.EnableWindow(!mb_Capturing && !mb_Analyzing && b_Serial);

	if (!b_Serial) mCtrl_ComboDrawMode.SetCurSel(0); // only Images
	
	if (mb_Capturing) SetLeds(FALSE); // all grey
}

void CLogicAnalyzerDlg::OnCheckAutoUnits() 
{
	EnableGUI();
}
void CLogicAnalyzerDlg::OnSelchangeComboProtocol() 
{
	EnableGUI();	
}


// Button Speedcheck
void CLogicAnalyzerDlg::OnBtnSpeed() 
{
	DWORD u32_Err = gi_App.mi_Port.CreateSpeedDiagram(mk_Settg.u16_BasePort, mk_Settg.u32_PortMask);
	if (u32_Err)
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "");
}

// Set the color of the 9 LEDs
// b_Show = FALSE --> All LED = grey
void CLogicAnalyzerDlg::SetLeds(BOOL b_Show)
{
	BYTE  u8_Data[PORT_COUNT];
	DWORD u32_Err = gi_App.mi_Port.GetPortData(mk_Settg.u16_BasePort, mk_Settg.u32_PortMask, u8_Data);
	if (u32_Err) b_Show = FALSE;

	BOOL b_Enabled = b_Show && (mk_Settg.u32_PortMask & DATA_MASK);
	mCtrl_LedData0.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 0)]);
	mCtrl_LedData1.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 1)]);
	mCtrl_LedData2.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 2)]);
	mCtrl_LedData3.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 3)]);
	mCtrl_LedData4.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 4)]);
	mCtrl_LedData5.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 5)]);
	mCtrl_LedData6.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 6)]);
	mCtrl_LedData7.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, DATA_OFFSET, 7)]);
	
	b_Enabled = b_Show && (mk_Settg.u32_PortMask & STATUS_MASK);
	mCtrl_LedStatus0.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, STATUS_OFFSET, 0)]);
	mCtrl_LedStatus1.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, STATUS_OFFSET, 1)]);
	mCtrl_LedStatus2.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, STATUS_OFFSET, 2)]);
	mCtrl_LedStatus3.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, STATUS_OFFSET, 3)]);
	mCtrl_LedStatus4.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, STATUS_OFFSET, 4)]);

	b_Enabled = b_Show && (mk_Settg.u32_PortMask & CTRL_MASK);
	mCtrl_LedCtrl0.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, CTRL_OFFSET, 0)]);
	mCtrl_LedCtrl1.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, CTRL_OFFSET, 1)]);
	mCtrl_LedCtrl2.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, CTRL_OFFSET, 2)]);
	mCtrl_LedCtrl3.SetIcon(mh_LedIcon[b_Enabled][gi_App.mi_Port.GetPortBit(u8_Data, CTRL_OFFSET, 3)]);
}

// Shows how many bytes have been written into memory / file
void CLogicAnalyzerDlg::ShowProgress()
{
	DWORD u32_Bytes = gi_App.mi_Port.GetCapturedBytes();

	CString s_Size;
	gi_App.mi_Func.FormatUnits((float)u32_Bytes, &s_Size, "Byte");

	mCtrl_StaticCaptured.SetWindowText(CString("Captured Data: ") + s_Size);
}

void CLogicAnalyzerDlg::ShowStatus()
{
	mCtrl_StaticStatusbar.SetWindowText(" " + gi_App.mi_Serial.GetProgress());
}

// ==================================

void CLogicAnalyzerDlg::OnBtnCapture() 
{
	if (mb_Capturing)
	{
		gi_App.mi_Port.StopCapture();
		return;
	}

	if (!mk_Settg.u32_MemSize)
	{
		mk_Settg.u32_MemSize = 1024*1024;
		mCtrl_EditMemSize.SetWindowText("1"); // Minimum 1 MB
	}

	mCtrl_EditAnalyzeStart.SetWindowText("");
	mCtrl_EditAnalyzeEnd.  SetWindowText("");

	mb_Capturing = gi_App.mi_Port.StartCapture(mk_Settg.u16_BasePort, mk_Settg.u32_MemSize, mk_Settg.u32_PortMask);
	EnableGUI();
}

// ==================================

void CLogicAnalyzerDlg::OnBtnAnalyze() 
{
	if (mb_Analyzing)
	{
		gi_App.mi_Serial.Abort();
		return;
	}
	
	double d_Unit = 0.0;
	double d_Idle = 0.0;

	if (!mCtrl_CheckAutoUnits.GetCheck())
	{
		CString s_Unit, s_Idle;
		mCtrl_EditRasterUnit.GetWindowText(s_Unit);
		mCtrl_EditIdleTime.  GetWindowText(s_Idle);

		d_Unit = gi_App.mi_Func.ParseInterval(s_Unit);
		d_Idle = gi_App.mi_Func.ParseInterval(s_Idle);

		if (s_Unit.GetLength() && d_Unit <= 0.0)
		{
			MessageBox("The raster unit is invalid.", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
			return;
		}
		if (s_Idle.GetLength() && d_Idle <= 0.0)
		{
			MessageBox("The idle interval is invalid.", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
			return;
		}
	}

	CString s_HtmlHeading;
	mCtrl_EditHtmlHeading.GetWindowText(s_HtmlHeading);
	BOOL b_DeleteOldFiles = mCtrl_CheckDeleteOldFiles.GetCheck();

	CSerial::eProtocol e_Protocol = (CSerial::eProtocol) mCtrl_ComboProtocol.GetCurSel();
	CSerial::eDrawMode e_DrawMode = (CSerial::eDrawMode)(mCtrl_ComboDrawMode.GetCurSel() + 1);

	CString s_Baudrate, s_Parity, s_Databits, s_Stopbits, s_AnalyzeStart, s_AnalyzeEnd;
	mCtrl_ComboBaudrate.GetWindowText(s_Baudrate);
	mCtrl_ComboParity.  GetWindowText(s_Parity);
	mCtrl_ComboDatabits.GetWindowText(s_Databits);
	mCtrl_ComboStopbits.GetWindowText(s_Stopbits);
	mCtrl_EditAnalyzeStart.GetWindowText(s_AnalyzeStart);
	mCtrl_EditAnalyzeEnd.  GetWindowText(s_AnalyzeEnd);

	double d_AnalyzeStart, d_AnalyzeEnd;
	if (!gi_App.mi_Func.ParseExactTime(s_AnalyzeStart, &d_AnalyzeStart) ||
		!gi_App.mi_Func.ParseExactTime(s_AnalyzeEnd,   &d_AnalyzeEnd))
	{
		MessageBox("The Start and End time must be entered in one of the formats:\nHH:MM:SS\nHH:MM:SS.mmm\nHH:MM:SS.mmm.uuu\n(Hour, Minutes, Seconds, MilliSeconds, MicroSeconds)", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
		return;
	}

	if (d_AnalyzeStart > 0 && d_AnalyzeEnd > 0 && d_AnalyzeStart >= d_AnalyzeEnd)
	{
		MessageBox("Invalid time interval.", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
		return;
	}

	int s32_Baudrate = 0;
	CSerial::eParity   e_Parity   = CSerial::E_ParNone;
	CSerial::eDataBits e_DataBits = CSerial::E_8Data;
	CSerial::eStopBits e_StopBits = CSerial::E_1Stop;

	if (e_Protocol == CSerial::E_Proto_Async)
	{
		s32_Baudrate = atol(s_Baudrate);
		if (s32_Baudrate <= 0)
		{
			MessageBox("Please enter a valid Baudrate!", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
			return;
		}

		     if (s_Parity == "None")  e_Parity = CSerial::E_ParNone;
		else if (s_Parity == "Even")  e_Parity = CSerial::E_ParEven;
		else if (s_Parity == "Odd")   e_Parity = CSerial::E_ParOdd;
		else 
		{
			MessageBox("Please select the Parity!", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
			return;
		}

		     if (s_Databits == "5") e_DataBits = CSerial::E_5Data;
		else if (s_Databits == "6") e_DataBits = CSerial::E_6Data;
		else if (s_Databits == "7") e_DataBits = CSerial::E_7Data;
		else if (s_Databits == "8") e_DataBits = CSerial::E_8Data;
		else if (s_Databits == "9") e_DataBits = CSerial::E_9Data;
		else 
		{
			MessageBox("Please select the Data Bits!", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
			return;
		}

		     if (s_Stopbits == "1") e_StopBits = CSerial::E_1Stop;
		else if (s_Stopbits == "2") e_StopBits = CSerial::E_2Stop;
		else 
		{
			MessageBox("Please select the Stop Bits!", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
			return;
		}
	}

	mb_Analyzing = TRUE;
	EnableGUI();

	// Set working mode of CSerial
	gi_App.mi_Serial.SetProtocol(e_Protocol, e_Parity, e_DataBits, e_StopBits, s32_Baudrate);

	// Start analyzing in CAnalyzer
	gi_App.mi_Serial.Analyze(e_DrawMode, d_Unit, d_Idle, d_AnalyzeStart, d_AnalyzeEnd, b_DeleteOldFiles, s_HtmlHeading);
}


void CLogicAnalyzerDlg::OnBtnHelp() 
{
	// The file Help.chm is copied in CFunctions::Initialize() when the program starts
	CString s_HelpFile = gi_App.mi_Func.GetOutFolder() + "Help.chm";
	DWORD u32_Err = gi_App.mi_Func.Execute(s_HelpFile, TRUE);
	if (u32_Err)
	{
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "while opening the file\n" + s_HelpFile);
	}
}
