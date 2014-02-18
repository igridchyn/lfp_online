
//-----------------------------------------------------------------------------
//     Author : ElmueSoft (www.netcult.ch/elmue)
//-----------------------------------------------------------------------------

/*
----------------------------------------------------------------------------------
Using these conventions results in better readable code and less coding errors !
----------------------------------------------------------------------------------

     cName  for generic class definitions
     CName  for MFC     class definitions
     tName  for type    definitions
     eName  for enum    definitions
     kName  for struct  definitions

    e_Name  for enum variables
    E_Name  for enum constant values

    i_Name  for instances of classes
    h_Name  for handles

    M_Name  for macros
    T_Name  for Templates
    t_Name  for TCHAR or LPTSTR

    s_Name  for strings
   sa_Name  for Ascii strings
   sw_Name  for Wide (Unicode) strings
   bs_Name  for BSTR
    f_Name  for function pointers
    k_Name  for contructs (struct)

    b_Name  bool,BOOL 1 Bit

   s8_Name    signed  8 Bit (char)
  s16_Name    signed 16 Bit (SHORT)
  s32_Name    signed 32 Bit (LONG, int)
  s64_Name    signed 64 Bit (LONGLONG)

   u8_Name  unsigned  8 Bit (BYTE)
  u16_Name  unsigned 16 bit (WORD, WCHAR)
  u32_Name  unsigned 32 Bit (DWORD, UINT)
  u64_Name  unsigned 64 Bit (ULONGLONG)

    d_Name  for double

  ----------------

    m_Name  for member variables of a class (e.g. ms32_Name for int member variable)
    g_Name  for global (static) variables   (e.g. gu16_Name for global WORD)
    p_Name  for pointer                     (e.g.   ps_Name  *pointer to string)
   pp_Name  for pointer to pointer          (e.g.  ppd_Name **pointer to double)
*/

#include "stdafx.h"
#include "LogicAnalyzer.h"
#include "Analyzer.h"
#include "FileEx.h"

extern CLogicAnalyzerApp gi_App;

CAnalyzer::CAnalyzer()
{
	// These indices increment during the lifetime of the program
	me_DrawMode         = E_DrawAll;
	mu32_ImgFileIdx     = 1;
	mu32_HtmlFileIdx    = 0;
	mb_IndividualFooter = FALSE;
	md_ExtraSpace       = 0;
	mi_Blocks.SetSize(100); // avoid multiple re-allocations
}

DWORD WINAPI AnalyzeThreadApi(void* p_Arg)
{
	((CAnalyzer*)p_Arg)->AnalyzeThread();
	return 0;
}

void CAnalyzer::Analyze(eDrawMode e_DrawMode, double d_Unit, double d_Idle, double d_AnalyzeStart, double d_AnalyzeEnd, BOOL b_DeleteOldFiles, CString s_HtmlHeading)
{
	gi_App.mi_Port.GetCaptureBuffer(&mpk_Samples, &mu32_Samples, &mu32_PortMask, &md_ClockTime);

	double d_TotalTime = mpk_Samples[mu32_Samples-1].d_Elapsed;

	md_AnalyzeStartAbs = d_AnalyzeStart;
	md_AnalyzeEndAbs   = d_AnalyzeEnd;
	md_AnalyzeStartRel = d_AnalyzeStart - md_ClockTime - 1e-6; // subtract 1 microsecond to eliminate rounding errors
	md_AnalyzeEndRel   = d_AnalyzeEnd   - md_ClockTime + 1e-6;

	if (md_AnalyzeStartRel < 0.0)         md_AnalyzeStartRel = 0.0;
	if (md_AnalyzeEndRel   > d_TotalTime) md_AnalyzeEndRel   = 0.0;

	me_DrawMode       = e_DrawMode, 
	md_Unit           = d_Unit;
	md_Idle           = d_Idle;
	ms_HtmlHeading    = s_HtmlHeading;
	mb_DeleteOldFiles = b_DeleteOldFiles;

	DWORD u32_ThreadID;
	mh_Thread = CreateThread(0, 0, &AnalyzeThreadApi, this, 0, &u32_ThreadID);
}

void CAnalyzer::Abort()
{
	mb_Abort = TRUE;
	mi_Graph.Abort();
}

void CAnalyzer::AnalyzeThread()
{
	mb_Abort = FALSE;

	AnalyzeMain();

	// ATTENTION: The GUI runs in another thread -> don't access GUI elements directly from here!
	gi_App.m_pMainWnd->SetTimer(ID_TIMER_ANALYZE_FINISHED, 100, 0);

	CloseHandle(mh_Thread);
}

void CAnalyzer::AnalyzeMain()
{
	DWORD u32_Err;

	ms_Progress = "Analyzing Lines and Intervals";

	double d_Shortest;
	PreParseData(&d_Shortest);
	if (!mk_Active.u32_Count)
	{
		MessageBox(gi_App.m_pMainWnd->m_hWnd, "No activity detected on the input lines.", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
		return;
	}

	ms_Progress = "Deleting old files";

	if (mb_DeleteOldFiles)
	{
		gi_App.mi_Func.DeleteAllFiles(gi_App.mi_Func.GetOutFolder(), "*.htm");
		gi_App.mi_Func.DeleteAllFiles(gi_App.mi_Func.GetOutFolder(), "*.gif");
		mu32_HtmlFileIdx = 0;
	}

	ms_Progress = "Initialzing";

	// Find the next possible filename
	CString s_HtmlFile;
	do
	{
		s_HtmlFile.Format("%sAnalyzer_%04d.htm", gi_App.mi_Func.GetOutFolder(), ++mu32_HtmlFileIdx);
	}
	while (CFileEx::IsFile(s_HtmlFile));

	// Open Html file for writing
	CFileEx i_HtmlFile;
	if (u32_Err = i_HtmlFile.Open(s_HtmlFile, CFileEx::E_WriteCreate))
	{
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "while opening "+s_HtmlFile);
		return;
	}

	// ===========================================================

	double d_Factor, d_Step;
	if (md_Unit == 0.0) // automatic scaling --> rounding on 1, 2, 2.5, 5, 10
	{
		// d_Shortest = 0.005 --> d_Factor = 0.001, d_Step = 5
		gi_App.mi_Func.GetOszilloscopeUnit(d_Shortest, &d_Factor, &d_Step, TRUE);
		md_Unit = d_Factor * d_Step;
	}
	else // User defined interval --> no rounding
	{
		gi_App.mi_Func.GetOszilloscopeUnit(md_Unit, &d_Factor, &d_Step, FALSE);
	}

	CGraphics::eRasterMode e_RasterX = CGraphics::GetRasterMode(d_Step);

	if (md_Idle == 0.0) md_Idle = d_Shortest * 50.0;

	// Print all inactive lines and the raster unit
	CString s_Html = PrintHtmlHead();
	if (!s_Html.GetLength())
		return;

	// mb_DeleteOldFiles = TRUE  --> Do not reset image index to avoid that browsers show old images from the cache 
	// (Analyzer_001_034.gif, Analyzer_001_035.gif, Analyzer_001_036.gif)
	// mb_DeleteOldFiles = FALSE --> reset image index because Html Index is already incremented
	// (Analyzer_015_001.gif, Analyzer_015_002.gif, Analyzer_015_003.gif)
	if (!mb_DeleteOldFiles) mu32_ImgFileIdx = 1;

	ms_Progress = "Retrieving the Sample Blocks";

	GetAllSampleBlocks(); // Fills mi_Blocks with kBlock

	if (mb_Abort)
		return;

	if (!mi_Blocks.GetSize())
	{
		MessageBox(gi_App.m_pMainWnd->m_hWnd, "No activity detected on the input lines.", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
		return;
	}

	GetAxisY(); // --> fill mk_AxisY

	CString s_Temp, s_Serial;
	for (int B=0; B<mi_Blocks.GetSize(); B++)
	{
		ms_Progress.Format("Drawing Sample Block %d from %d", B, mi_Blocks.GetSize());

		// ms_FooterHtml is filled in CSerial::AnalyzeSerial() with a comma separated list of the detected Bytes
		for (UINT A=0; A<mk_Active.u32_Count; A++)
		{
			ms_FooterHtml[A] = "";
		}

		kBlock k_CurBlock = mi_Blocks.GetAt(B);
		if (!PrintBlock(&k_CurBlock, &s_Html, e_RasterX))
			return;

		s_Serial = "";
		for (A=0; A<mk_Active.u32_Count; A++)
		{
			if (!ms_FooterHtml[A].GetLength())
				continue;

			s_Temp.Format("<div class='Line_%d'><nobr>", mk_Active.u32_Bit[A]);
			s_Serial += s_Temp;
			s_Serial += ms_FooterHtml[A];
			s_Serial += "</nobr></div>\r\n";
		}

		// Print the current time stamp only if GIF images are turned off
		if ((me_DrawMode & E_DrawImg) == 0 && s_Serial.GetLength())
		{
			CString s_Time = gi_App.mi_Func.FormatExactTime(md_ClockTime + mk_Block.d_Xstart);
			s_Html += "<div>" + s_Time + "</div>\r\n";
		}
		
		s_Html += s_Serial;

		// Print Separator
		if ((me_DrawMode & E_DrawImg) > 0 || s_Serial.GetLength())
		{
			s_Html += "<div>&nbsp;</div>\r\n\r\n";
		}

		if (u32_Err = i_HtmlFile.Write(s_Html, s_Html.GetLength()))
		{
			gi_App.mi_Func.ShowErrorMsg(u32_Err, "while writing to HTML file");
			return;
		}
		s_Html = "";

	} // end for

	s_Html = "</body>\r\n</html>\r\n";
	i_HtmlFile.Write(s_Html, s_Html.GetLength());

	i_HtmlFile.Close();

	if (mb_Abort)
		return;

	ms_Progress.Format("Opening Html file in browser");

	// Open Html file in Browser
	if (u32_Err = gi_App.mi_Func.Execute(s_HtmlFile, TRUE))
	{
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "while opening HTML file in Browser");
		return;
	}
}

// Prints pk_CurBlock. If the image would become wider than MAX_IMG_WIDTH it is splitted into partial images
// that are displayed side by side in a HTML table row.
BOOL CAnalyzer::PrintBlock(kBlock* pk_CurBlock, CString* ps_Html, CGraphics::eRasterMode e_RasterX)
{
	// Search anew for a start bit
	ResetSerial(pk_CurBlock);

	if ((me_DrawMode & E_DrawImg) == 0)
	{
		mk_Block = *pk_CurBlock;
		AnalyzeSerial();
		return TRUE;
	}

	*ps_Html += "<table cellspacing=0 cellpadding=0><tr>\r\n";

	// The maximum time interval to be displayed per image, rounded to multiples of TIME_SCALE_CELLS
	int  s32_MaxCells = (MAX_IMG_WIDTH / CELL_WIDTH / TIME_SCALE_CELLS) * TIME_SCALE_CELLS;
	double d_MaxTime  = md_Unit * s32_MaxCells;

	mk_Block = *pk_CurBlock;

	if (mk_Block.d_Xend - mk_Block.d_Xstart < d_MaxTime) // Image is shorter than MAX_IMG_WIDTH
	{
		if (!PrintDiagram(ps_Html, e_RasterX))
			return FALSE;
	}
	else // Split into multiple images with maximum width = MAX_IMG_WIDTH
	{
		do
		{
			// Set end values to the end of the entire block
			mk_Block.u32_LastSample = pk_CurBlock->u32_LastSample;
			mk_Block.d_Xend         = pk_CurBlock->d_Xend;
			mk_Block.d_XendExtra    = pk_CurBlock->d_XendExtra;
			mk_Block.d_IdleAfter    = pk_CurBlock->d_IdleAfter;

			// Search the horizontal end of the current image
			for (DWORD S=mk_Block.u32_FirstSample; S<=mk_Block.u32_LastSample; S++)
			{
				double d_SamplEnd = min(mpk_Samples[S+1].d_Elapsed, mk_Block.d_Xend);

				if (d_SamplEnd - mk_Block.d_Xstart > d_MaxTime)
				{
					// The sample is longer than the maximum allowed image width (MAX_IMG_WIDTH)
					mk_Block.u32_LastSample = S;
					mk_Block.d_Xend         = min(mk_Block.d_Xend - md_Unit, mk_Block.d_Xstart + d_MaxTime);
					mk_Block.d_XendExtra    = mk_Block.d_Xend;
					mk_Block.d_IdleAfter    = 0;
					break;
				}
			}			

			if (!PrintDiagram(ps_Html, e_RasterX))
				return FALSE;

			// Start the next image immediately behind the current image
			mk_Block.u32_FirstSample = mk_Block.u32_LastSample;
			mk_Block.d_Xstart        = mk_Block.d_Xend;
			mk_Block.d_IdleBefore    = 0;
			mk_Block.b_HasLabel      = FALSE;
		}
		while (mk_Block.d_Xend < pk_CurBlock->d_Xend);
	}

	*ps_Html += "</tr></table>\r\n";
	return TRUE;
}

// Prints mk_Block. This may be an entire block or a part of a splitted block
BOOL CAnalyzer::PrintDiagram(CString* ps_Html, CGraphics::eRasterMode e_RasterX)
{
	CString s_ImgFile, s_Temp;

	GetAxisX(); // --> fill mk_AxisX

	// =================== CREATE BITMAP ====================

	DWORD u32_Err;
	if (u32_Err = mi_Graph.CreateNewDiagram(mk_AxisX.s32_TotalWidth, mk_AxisY.s32_TotalHeight))
	{
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "while creating bitmap");
		return FALSE;
	}

	mi_Graph.SetFont ("Verdana", 9);
	mi_Graph.SetColor(CGraphics::E_PaletteRaster);

	// =================== DRAW GLITCHES + SAMPLES ====================

	for (DWORD S=mk_Block.u32_FirstSample; S<=mk_Block.u32_LastSample; S++)
	{
		double d_Xstart = max(mk_Block.d_Xstart, mpk_Samples[S].  d_Elapsed);
		double d_Xend   = min(mk_Block.d_Xend,   mpk_Samples[S+1].d_Elapsed);

		// Convert time --> pixel coordinates
		int s32_Xstart = mk_AxisX.s32_RasterStart + (int)((d_Xstart - mk_Block.d_Xstart) * CELL_WIDTH / md_Unit);
		int s32_Xend   = mk_AxisX.s32_RasterStart + (int)((d_Xend   - mk_Block.d_Xstart) * CELL_WIDTH / md_Unit);

		if ((mpk_Samples[S+1].u8_Data[CTRL_OFFSET] & CTRL_GLITCH))
		{
			for (DWORD L=0; L<mk_Active.u32_Count; L++)
			{
				// Draw dark red background
				mi_Graph.FillRectangle(s32_Xstart, mk_AxisY.s32_LineStart[L], 
									   s32_Xend - s32_Xstart, CELL_HEIGHT, CGraphics::E_PaletteGlitch);
			}
		}

		// Only in Debug mode: optionally draw all sample indices into the footer
		#if SHOW_SAMPLES
			if (mpk_Samples[S].d_Elapsed >= mk_Block.d_Xstart)
			{
				s_Temp.Format("%d", S);
				mi_Graph.DrawText(s_Temp, s32_Xstart, mk_AxisY.s32_RasterEnd, 50, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
			}
		#endif
	}

	mi_Graph.SetFont ("Verdana", 12);

	// ==================== DRAW RASTER ===================

	if (mb_IndividualFooter)
	{
		// Each line has its own footer (RxD and TxD in Async Protocol)
		for (DWORD L=0; L<mk_Active.u32_Count; L++)
		{
			mi_Graph.DrawRaster(mk_AxisX.s32_RasterStart, mk_AxisY.s32_LineStart[L], 
								mk_AxisX.s32_RasterEnd,   mk_AxisY.s32_LineStart[L] + CELL_HEIGHT, 
								CELL_WIDTH,               CELL_HEIGHT,
								e_RasterX,                CGraphics::E_OnlySolid);
		}
	}
	else
	{
		// All lines share the same footer (PS/2, I2C, SPI, etc..)
		mi_Graph.DrawRaster(mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterStart, 
							mk_AxisX.s32_RasterEnd,   mk_AxisY.s32_RasterEnd, 
							CELL_WIDTH,               CELL_HEIGHT,
							e_RasterX,                CGraphics::E_OnlySolid);
	}

	// =================== DRAW TIME AXIS ===================

	mi_Graph.SetColor(CGraphics::E_PaletteText);
	mi_Graph.DrawTimeAxis(md_ClockTime + mk_Block.d_Xstart, mk_AxisY.s32_TimeStart, TIME_HEIGHT, 
		                  CELL_WIDTH * TIME_SCALE_CELLS, md_Unit * TIME_SCALE_CELLS);

	// ===================== DRAW LINES =====================

	for (DWORD L=0; L<mk_Active.u32_Count; L++) // Loop through all active lines
	{
		if (!PrintLine(L))
			return FALSE;
	}

	// ===================== DRAW SERIAL ====================

	// Print the detected bits and bytes of the serial protocol into the bitmap in the derived class
	AnalyzeSerial();

	// ==================== SAVE GIF FILE ===================

	s_ImgFile.Format("Analyzer_%04d_%04d.gif", mu32_HtmlFileIdx, mu32_ImgFileIdx++);
	if (u32_Err = mi_Graph.SaveAsGif(gi_App.mi_Func.GetOutFolder() + s_ImgFile))
	{
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "while writing to GIF image file");
		return FALSE;
	}

	CString s_Style;
	#if SHOW_IMAGES
		s_Style = "border:1px solid blue;";
	#endif

	s_Temp.Format("\t<td><img src='%s' width=%d height=%d style='%s'></td>\r\n", 
	              s_ImgFile, mk_AxisX.s32_TotalWidth, mk_AxisY.s32_TotalHeight, s_Style);

	*ps_Html += s_Temp;
	return TRUE;
}

// Print one oscilloscope line into the current image
BOOL CAnalyzer::PrintLine(DWORD u32_ActiveLine)
{
	const static char* s8_PortNames[] = { PORT_NAME_LIST };

	DWORD u32_Port = mk_Active.u32_Port[u32_ActiveLine];
	DWORD u32_Bit  = mk_Active.u32_Bit [u32_ActiveLine];

	int   s32_Yoffset = mk_AxisY.s32_CurveStart[u32_ActiveLine];
	double  d_Xoffset = mk_Block.d_Xstart;

	if (mk_Block.b_HasLabel)
	{
		// The name of the input line e.g. "Data 5"
		CString s_Label;
		s_Label.Format("%s %d", s8_PortNames[u32_Port], u32_Bit);

		mi_Graph.SetColor(CGraphics::E_PaletteText);
		mi_Graph.DrawText(s_Label, mk_AxisX.s32_Label, s32_Yoffset, LABEL_WIDTH, CURVE_HEIGHT, CGraphics::E_MiddleLeft);
	}

	int s32_LastLevel = GetActiveBit(u32_ActiveLine, mk_Block.u32_FirstSample);
	int s32_Ypos      = s32_Yoffset + (s32_LastLevel > 0 ? 0 : CURVE_HEIGHT);

	// Draw the dashed idle interval (if any)
	DrawIdleInterval(mk_Block.d_IdleBefore, mk_AxisX.s32_RasterStart, mk_AxisX.s32_CurveStart, s32_Ypos, u32_Bit);

	mi_Graph.SetColor(CGraphics::E_PaletteLine0 + u32_Bit);
	mi_Graph.SetLineMode(CGraphics::E_LineSolid);

	for (DWORD S=mk_Block.u32_FirstSample; S<=mk_Block.u32_LastSample; S++)
	{
		if (mb_Abort)
			return FALSE;

		double d_Xstart = max(mk_Block.d_Xstart, mpk_Samples[S].  d_Elapsed);
		double d_Xend   = min(mk_Block.d_Xend,   mpk_Samples[S+1].d_Elapsed);

		// Convert time --> pixel coordinates
		int s32_Xstart = mk_AxisX.s32_RasterStart + (int)((d_Xstart - d_Xoffset) * CELL_WIDTH / md_Unit);
		int s32_Xend   = mk_AxisX.s32_RasterStart + (int)((d_Xend   - d_Xoffset) * CELL_WIDTH / md_Unit);

		// Level = 1 or 0
		int s32_Level = GetActiveBit(u32_ActiveLine, S);
		
		// Draw the vertical line
		if (s32_LastLevel != s32_Level)
		{
			s32_LastLevel = s32_Level;
			mi_Graph.DrawLine(s32_Xstart, s32_Yoffset, s32_Xstart, s32_Yoffset + CURVE_HEIGHT + 1);
			s32_Ypos = s32_Yoffset + (s32_Level > 0 ? 0 : CURVE_HEIGHT);
		}

		// Draw the horizontal line
		mi_Graph.DrawLine(s32_Xstart, s32_Ypos, s32_Xend, s32_Ypos);
	}

	// Draw the solid line for the additional space which is inserted for the async protocol
	mi_Graph.DrawLine(mk_AxisX.s32_ExtraSpace, s32_Ypos, mk_AxisX.s32_RasterEnd, s32_Ypos);

	// Draw the dashed idle interval (if any)
	DrawIdleInterval(mk_Block.d_IdleAfter, mk_AxisX.s32_RasterEnd, mk_AxisX.s32_TotalWidth, s32_Ypos, u32_Bit);

	return TRUE;
}

// Draws the dashed idle line and the idle interval at the begin and end of a block
void CAnalyzer::DrawIdleInterval(double d_Idle, int s32_Xstart, int s32_Xend, int s32_Ypos, DWORD u32_Bit)
{
	if (s32_Xstart == s32_Xend)
		return; // There is no idle interval to be drawn (e.g. part of a splitted diagram)

	// Draw the dashed line
	mi_Graph.SetColor(CGraphics::E_PaletteLine0 + u32_Bit);
	mi_Graph.SetLineMode(CGraphics::E_LineDashed);
	mi_Graph.DrawLine(s32_Xstart, s32_Ypos, s32_Xend, s32_Ypos);

	if (d_Idle <= 0)
		return;
	
	CString s_Idle = "Idle: " + gi_App.mi_Func.FormatInterval(d_Idle);
	mi_Graph.SetColor(CGraphics::E_PaletteText);
	mi_Graph.DrawText(s_Idle, min(s32_Xstart, s32_Xend), mk_AxisY.s32_TimeStart, IDLE_WIDTH, TIME_HEIGHT, CGraphics::E_MiddleCenter);
}

// ==============================================================================

// Print the Html header that shows the active lines and the current settings
// returns an empty string on error
CString CAnalyzer::PrintHtmlHead()
{
	const static char* s8_PortNames[] = { PORT_NAME_LIST };

	CString s_Html, s_Temp, s_Color;
	s_Html.GetBuffer(64000); // Allocate buffer to avoid multiple re-allocations
	
	DWORD u32_Err = gi_App.mi_Func.GetResource(MAKEINTRESOURCE(IDR_HTML_TEMPLATE), RT_RCDATA, &s_Html);
	if (u32_Err)
	{
		gi_App.mi_Func.ShowErrorMsg(u32_Err, "getting Html resource");
		return "";
	}

	// ---- Insert colors into CSS -----

	for (int Line=0; Line<8; Line++)
	{
		s_Temp. Format("{ColorLine%d}", Line);
		s_Color.Format("%06X", mi_Graph.GetPaletteColor(CGraphics::E_PaletteLine0 + Line));
		s_Html.Replace(s_Temp, s_Color);
	}

	s_Color.Format("%06X", mi_Graph.GetPaletteColor(CGraphics::E_PaletteGlitch));
	s_Html.Replace("{ColorGlitch}", s_Color);

	// ---- Print Active Lines ----

	if (ms_HtmlHeading.GetLength())
		s_Html += "<h2>"+ms_HtmlHeading+"</h2>\r\n";

	s_Html += "<table cellspacing=0 cellpadding=0>\r\n";

	for (DWORD L=0; L<mk_Active.u32_Count; L++)
	{
		DWORD u32_Port = mk_Active.u32_Port[L];
		DWORD u32_Bit  = mk_Active.u32_Bit [L];

		CString s_Descr = GetLineDescription(L);
		if (!s_Descr.GetLength())
			 s_Descr = "Activity detected";
		
		s_Temp.Format("\t<tr><td class='Row'>%s %d:</td><td class='Row Line_%d'>%s</td></tr>\r\n", 
					  s8_PortNames[u32_Port], u32_Bit, u32_Bit, s_Descr);

		s_Html += s_Temp;
	}

	// ---- Print Inactive Lines ----

	for (L=0; L<mk_Inactive.u32_Count; L++)
	{
		DWORD u32_Port  = mk_Inactive.u32_Port[L];
		DWORD u32_Bit   = mk_Inactive.u32_Bit [L];

		int  s32_Level = gi_App.mi_Port.GetPortBit(mpk_Samples[0].u8_Data, u32_Port, u32_Bit);
		char* s8_Level = (s32_Level > 0) ? "High" : "Low";

		s_Temp.Format("\t<tr><td class='Row'>%s %d:</td><td class='Row Line_%d'>No input signal (always %s)</td></tr>\r\n", 
		              s8_PortNames[u32_Port], u32_Bit, u32_Bit, s8_Level);

		s_Html += s_Temp;
	}

	s_Html += "</table>\r\n";
	s_Html += "<div>&nbsp;</div>\r\n";
	
	CString s_Total = gi_App.mi_Func.FormatInterval(mpk_Samples[mu32_Samples-1].d_Elapsed);
	s_Html += "<div class='Row'>Total Capture Time = "+s_Total+"</div>\r\n";

	if (md_AnalyzeStartRel > 0)
	{
		CString s_Start = gi_App.mi_Func.FormatExactTime(md_AnalyzeStartAbs);
		s_Html += "<div class='Row'>Start Analysis at "+s_Start+"</div>\r\n";
	}

	if (md_AnalyzeEndRel > 0)
	{
		CString s_End = gi_App.mi_Func.FormatExactTime(md_AnalyzeEndAbs);	
		s_Html += "<div class='Row'>End Analysis at "+s_End+"</div>\r\n";
	}
	
	CString s_Unit = gi_App.mi_Func.FormatInterval(md_Unit);
	CString s_Freq = gi_App.mi_Func.FormatFrequency(1.0 / md_Unit, "Hz");
	s_Html += "<div class='Row'>Hor. Raster Unit = "+s_Unit+" ("+s_Freq+")</div>\r\n";
	
	CString s_Idle = gi_App.mi_Func.FormatInterval(md_Idle);
	s_Html += "<div class='Row'>Crop Idle Intervals &gt; "+s_Idle+"</div>\r\n";

	CString s_Glitch = gi_App.mi_Func.FormatInterval(md_Unit * MIN_GLITCH_LEN);
	s_Html += "<div class='Row'>Always show <span class='Glitch'>Context Switches</span> &gt; "+s_Glitch+"</div>\r\n";
	
	s_Html += "<div>&nbsp;</div>\r\n\r\n";
	return s_Html;
}

// Fills mk_AxisY with the vertical positions of curves, lines and footers
void CAnalyzer::GetAxisY()
{
	mk_AxisY.s32_TimeStart   = 0;
	mk_AxisY.s32_RasterStart = mk_AxisY.s32_TimeStart + TIME_HEIGHT;
	mk_AxisY.s32_RasterEnd   = mk_AxisY.s32_RasterStart;

	for (DWORD L=0; L<mk_Active.u32_Count; L++)
	{
		// for Async Protocol RxD and TxD show their decoded Bytes in an individual footer
		if (mb_IndividualFooter && L>0) mk_AxisY.s32_RasterEnd += FOOTER_HEIGHT;

		mk_AxisY.s32_LineStart [L] = mk_AxisY.s32_RasterEnd;
		mk_AxisY.s32_CurveStart[L] = mk_AxisY.s32_RasterEnd + (CELL_HEIGHT - CURVE_HEIGHT) / 2;
		mk_AxisY.s32_RasterEnd += CELL_HEIGHT;
	}

	mk_AxisY.s32_TotalHeight = mk_AxisY.s32_RasterEnd + FOOTER_HEIGHT;
}

// Fills mk_AxisX with the horizontal positions of label, raster and idle intervals
void CAnalyzer::GetAxisX()
{
	int s32_CurveWidth = (int)((mk_Block.d_Xend      - mk_Block.d_Xstart) * CELL_WIDTH / md_Unit);
	int s32_ExtraWidth = (int)((mk_Block.d_XendExtra - mk_Block.d_Xend)   * CELL_WIDTH / md_Unit);
	
	mk_AxisX.s32_Label       = 0;
	mk_AxisX.s32_CurveStart  = mk_AxisX.s32_Label       + (mk_Block.b_HasLabel     ? LABEL_WIDTH : 0);
	mk_AxisX.s32_RasterStart = mk_AxisX.s32_CurveStart  + (mk_Block.d_IdleBefore>0 ? IDLE_WIDTH  : 0);
	mk_AxisX.s32_ExtraSpace  = mk_AxisX.s32_RasterStart + s32_CurveWidth;
	mk_AxisX.s32_RasterEnd   = mk_AxisX.s32_ExtraSpace  + s32_ExtraWidth;
	mk_AxisX.s32_TotalWidth  = mk_AxisX.s32_RasterEnd   + (mk_Block.d_IdleAfter>0  ? IDLE_WIDTH  : 0);
}

// ==============================================================================

// Check all blocks that come from GetNextSampleBlock() and eliminate empty blocks
void CAnalyzer::GetAllSampleBlocks()
{
	mi_Blocks.RemoveAll();

	memset(&mk_Block, 0, sizeof(kBlock));
	
	while (GetNextSampleBlock()) 
	{
		if (mk_Block.b_IsEmpty)
			continue;

		mk_Block.b_HasLabel  = TRUE;
		mk_Block.d_Xstart    = mpk_Samples[mk_Block.u32_FirstSample+1].d_Elapsed;
		mk_Block.d_Xend      = mpk_Samples[mk_Block.u32_LastSample]   .d_Elapsed;
		mk_Block.d_XendExtra = mk_Block.d_Xend;

		// Skip blocks that are not within the interval of Start Time and End Time
		if (md_AnalyzeStartRel > 0 && mk_Block.d_Xend < md_AnalyzeStartRel)
			continue;

		if (md_AnalyzeEndRel > 0 && mk_Block.d_Xstart > md_AnalyzeEndRel)
			continue;

		mi_Blocks.Add(mk_Block);
	}

	if (mb_Abort)
		return;

	// Write the idle intervals before and after the block into all blocks
	int Count = mi_Blocks.GetSize();
	int Last  = Count - 1;

	for (int B=0; B<Count; B++)
	{
		kBlock* pk_Curr = &mi_Blocks.ElementAt(B);

		if (B == 0) // the first block
		{
			if (pk_Curr->d_Xstart < md_Idle)
			{
				pk_Curr->d_Xstart        = 0;
				pk_Curr->u32_FirstSample = 0;
			}

			pk_Curr->d_IdleBefore = pk_Curr->d_Xstart;
		}

		if (B > 0)
		{
			kBlock* pk_Last = &mi_Blocks.ElementAt(B-1);

			double d_Idle = pk_Curr->d_Xstart - pk_Last->d_Xend;

			pk_Curr->d_IdleBefore = d_Idle;
			pk_Last->d_IdleAfter  = d_Idle;
		}

		if (B == Last) // the last block
		{
			double d_Idle = mpk_Samples[mu32_Samples-1].d_Elapsed - pk_Curr->d_Xend;
			if (d_Idle < md_Idle)
			{
				d_Idle = 0;
				pk_Curr->d_Xend = mpk_Samples[mu32_Samples-1].d_Elapsed;
			}

			pk_Curr->d_IdleAfter = d_Idle;
		}
	}

	// The serial protocol needs always a raster with the length of at least 1 byte.
	// But if there is only a Start Bit with the other bits = HIGH, the time after the start bit will be
	// recognized as idle time. So the raster must be extended to the right to assure that an entire byte will always fit.
	// For the Async Protocol the length of a byte is added to the end of the diagram in front of the idle interval.
	if (md_ExtraSpace > 0)
	{
		for (int B=0; B<Count; B++)
		{
			kBlock* pk_Curr = &mi_Blocks.ElementAt(B);
			if (pk_Curr->d_IdleAfter < md_ExtraSpace)
				continue;

			pk_Curr->d_XendExtra += md_ExtraSpace;
			pk_Curr->d_IdleAfter -= md_ExtraSpace;

			if (B < Last)
			{
				kBlock* pk_Next = &mi_Blocks.ElementAt(B+1);
				pk_Next->d_IdleBefore -= md_ExtraSpace;
			}
		}
	}
}

// Find the start and end point for the next block
// Sets mk_Block.u32_FirstSample, mk_Block.u32_LastSample, mk_Block.b_IsEmpty
// The start position is set one sample before and the end position is set one sample after the activities that were found
// returns FALSE when there is no more data
BOOL CAnalyzer::GetNextSampleBlock()
{
	// Start where the last blocks has terminated
	mk_Block.u32_FirstSample = mk_Block.u32_LastSample;
	mk_Block.b_IsEmpty  = TRUE;

	double  d_LastActivity = 0;
	DWORD u32_LastActivity = 0;

	for (DWORD S=mk_Block.u32_FirstSample; TRUE; S++)
	{
		if (mb_Abort || S >= mu32_Samples-1)
			return FALSE;

		CPort::kSample* pk_Last = &mpk_Samples[S];
		CPort::kSample* pk_Curr = &mpk_Samples[S+1];

		double d_SamplLen = pk_Curr->d_Elapsed - pk_Last->d_Elapsed;

		BOOL b_LevelChange = (*((DWORD*)pk_Last->u8_Data) & ALL_USED_AS_INPUT) != 
		                     (*((DWORD*)pk_Curr->u8_Data) & ALL_USED_AS_INPUT);

		BOOL b_LongGlitch  = (pk_Curr->u8_Data[CTRL_OFFSET] & CTRL_GLITCH) && 
		                     (d_SamplLen > md_Unit * MIN_GLITCH_LEN);

		mk_Block.u32_LastSample = S;

		if (mk_Block.b_IsEmpty) // Search the start of the block (where activity is detected)
		{
			if (b_LevelChange)
			{
				mk_Block.u32_FirstSample = S;   // S = sample before the first level change

				d_LastActivity = pk_Curr->d_Elapsed;
				u32_LastActivity = S+1;
				
				mk_Block.b_IsEmpty = FALSE;
			}
			if (b_LongGlitch)
			{
				mk_Block.u32_FirstSample = S-1; // S = sample before glitch

				d_LastActivity = pk_Curr->d_Elapsed;
				u32_LastActivity = S+1;
				
				// If a Glitch is longer than md_Unit * MIN_GLITCH_LEN assign him a new block 
				// EVEN IF there is no level change in this block !!
				#if SHOW_SINGLE_GLITCHES
					mk_Block.b_IsEmpty = FALSE;
				#endif
			}
		}
		else // Search the end of the block (end of activity)
		{
			if (pk_Curr->d_Elapsed - d_LastActivity > md_Idle)
			{
				mk_Block.u32_LastSample = u32_LastActivity;
				break;
			}

			if (b_LevelChange || b_LongGlitch)
			{
				  d_LastActivity = pk_Curr->d_Elapsed;
				u32_LastActivity = S+1;
			}
		}
	}
	return TRUE;
}

// returns the line status (0 or 1) at the given sample position for the given active line
int CAnalyzer::GetActiveBit(DWORD u32_ActiveLine, DWORD u32_Sample)
{
	DWORD u32_DataPort = mk_Active.u32_Port[u32_ActiveLine];
	DWORD u32_DataBit  = mk_Active.u32_Bit [u32_ActiveLine];

	return gi_App.mi_Port.GetPortBit(mpk_Samples[u32_Sample].u8_Data, u32_DataPort, u32_DataBit);
}

// ==============================================================================

// Checks all captured lines for level changes and fills mk_Active and mk_Inactive
// In d_Shortest the shortest interval is returned (in seconds)
void CAnalyzer::PreParseData(double* pd_Shortest)
{
	*pd_Shortest = 1.0e100;

	mk_Active.  u32_Count = 0;
	mk_Inactive.u32_Count = 0;

	if (mu32_Samples < 2) // There are always at least two samples
		return;

	BYTE u8_UsedBits[3];
	u8_UsedBits[DATA_OFFSET]   = DATA_USED_AS_INPUT;
	u8_UsedBits[STATUS_OFFSET] = STATUS_USED_AS_INPUT;
	u8_UsedBits[CTRL_OFFSET]   = CTRL_USED_AS_INPUT;

	// In u8_Active[0-2] those bits will be set that correspond to the active input lines.
	BYTE u8_Active[PORT_COUNT];

	for (DWORD P=0; P<PORT_COUNT; P++)
	{
		u8_Active[P] = 0;

		if ((mu32_PortMask & (1 << P)) == 0)
			continue; // This port has not been scanned

		for (DWORD S=1; S<mu32_Samples; S++)
		{
			u8_Active[P] |= (mpk_Samples[S].u8_Data[P] ^ mpk_Samples[S-1].u8_Data[P]);
		}

		// Reset the CTRL_GLITCH bit!
		u8_Active[P] &= u8_UsedBits[P];
	}

	for (P=0; P<PORT_COUNT; P++)
	{
		if ((mu32_PortMask & (1 << P)) == 0)
			continue; // This port has not been scanned

		for (DWORD B=0; B<8; B++)
		{
			switch (gi_App.mi_Port.GetPortBit(u8_Active, P, B))
			{
			case 1: // line active
				mk_Active.u32_Port[mk_Active.u32_Count] = P;
				mk_Active.u32_Bit [mk_Active.u32_Count] = B;
				mk_Active.u32_Count ++;
				break;
			case 0: // line inactive
				mk_Inactive.u32_Port[mk_Inactive.u32_Count] = P;
				mk_Inactive.u32_Bit [mk_Inactive.u32_Count] = B;
				mk_Inactive.u32_Count ++;
				break;
			case -1: 
				// the bit is unused in this port
				break;
			}
		}
	}

	// Search the shortest interval among the active lines
	for (P=0; P<PORT_COUNT; P++)
	{
		for (DWORD Mask=1; Mask<=128; Mask*=2)
		{
			if ((u8_Active[P] & Mask) == 0)
				continue; // This line is inactive or unused

			BOOL   b_OldLevel = (mpk_Samples[0].u8_Data[P] & Mask) > 0;
			double d_OldTime  =  mpk_Samples[0].d_Elapsed;

			for (DWORD S=1; S<mu32_Samples; S++)
			{
				BOOL b_Level = (mpk_Samples[S].u8_Data[P] & Mask) > 0;
				if  (b_Level == b_OldLevel)
					continue; // The line did not change

				double d_Time = mpk_Samples[S].d_Elapsed;
				double d_Span = d_Time - d_OldTime;
				
				// d_Span may be == 0.0 !!
				if (d_Span > 0.0) *pd_Shortest = min(*pd_Shortest, d_Span);

				b_OldLevel = b_Level;
				d_OldTime  = d_Time;	
			}
		}
	}
}

// returns the values that have been determined in automatic scaling mode
void CAnalyzer::GetUnits(double* pd_Unit, double* pd_Idle)
{
	*pd_Unit = md_Unit;
	*pd_Idle = md_Idle;
}

// returns the current progress info for the Statusbar
CString CAnalyzer::GetProgress()
{
	return ms_Progress;
}

// Prints the footer row with the detected bits and bytes
void CAnalyzer::AnalyzeSerial()
{
	// This virtual function must be overridden in the derived class CSerial
	ASSERT(0);
}
// Resets the CSerial class
void CAnalyzer::ResetSerial(kBlock* pk_CurBlock)
{
	// This virtual function must be overridden in the derived class CSerial
	ASSERT(0);
}
// Gets the line's name to be displayed (e.g. "Data0 = SCL", Data1 = SDA")
CString CAnalyzer::GetLineDescription(DWORD u32_Line)
{
	// This virtual function must be overridden in the derived class CSerial
	ASSERT(0);
	return "";
}

