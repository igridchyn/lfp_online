
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
#include "Serial.h"

extern CLogicAnalyzerApp gi_App;

// CSerial IS DERIVED FROM CAnalyzer


void CSerial::SetProtocol(eProtocol e_Protocol, eParity e_Parity, eDataBits e_DataBits, eStopBits e_StopBits, DWORD u32_Baudrate)
{
	me_Protocol = e_Protocol;

	switch (me_Protocol)
	{
		case E_Proto_Async:
			// Parity, Stopbits and Baudrate are set in SetProtocol()
			me_DataOrder  = E_LSB;
			me_Parity     = e_Parity;
			me_DataBits   = e_DataBits;
			me_StopBits   = e_StopBits;
			me_Acknowlg   = E_AckNone;   
			mu32_Baudrate = u32_Baudrate;

			BuildBitDefinition();

			// Each line RxD and TxD needs it's own footer to display the analyzed bytes
			mb_IndividualFooter = TRUE;

			// Subtract the 2 pseudo Bits: BIT_SEARCH_HIGH, BIT_SEARCH_LOW
			md_ExtraSpace = (double)(mi_Decode.s_BitDef.GetLength() -2) / mu32_Baudrate;
			break;
		
		case E_Proto_PS2:
			me_DataBits   = E_8Data;
			me_DataOrder  = E_LSB;
			me_Parity     = E_ParOdd;
			me_StopBits   = E_1Stop;
			me_Acknowlg   = E_AckNone; // is modified in Host-to-Device mode
			me_Clock      = E_ClkNeg;  // is modified in Host-to-Device mode
			mu32_Baudrate = 0;

			BuildBitDefinition();

			// Data and Clock line use only one common footer
			mb_IndividualFooter = FALSE;

			// PS/2 doesn't need extra space
			md_ExtraSpace = 0;
			break;
		
		case E_Proto_I2C:
			// To be implemented
			break;

		case E_Proto_SPI:
			// To be implemented
			break;
		
		case E_Proto_InfraRed:
			// To be implemented
			break;

		case E_Proto_SmartCard:
			// To be implemented
			break;
	}

	mi_Decode.ResetAll();
}

// This function is called from CAnalyzer when analyzing starts and after each idle interval where a new block starts
// This function is NOT called for each diagram when a very long block is split into multiple GIF images 
void CSerial::ResetSerial(kBlock* pk_CurBlock)
{
	switch (me_Protocol)
	{
		case E_Proto_Async:
		{
			mi_Decode.ResetAll();
			break;
		}
		case E_Proto_PS2:
		{
			int s32_ClkLevel  = GetActiveBit(0, pk_CurBlock->u32_FirstSample);
			int s32_DataLevel = GetActiveBit(1, pk_CurBlock->u32_FirstSample);
			
			// ATTENTION: 
			// After a Request To Send from the host (Data stays Low) may follow an idle interval up to 15ms!
			// In THIS idle interval the decoder must NOT be reset!!!
			if (s32_ClkLevel == 1 && s32_DataLevel == 1)
				mi_Decode.Reset();

			break;
		}
		case E_Proto_I2C:
			// To be implemented
			break;

		case E_Proto_SPI:
			// To be implemented
			break;
		
		case E_Proto_InfraRed:
			// To be implemented
			break;

		case E_Proto_SmartCard:
			// To be implemented
			break;
	}
}

// #####################################################################################################
// #####################################################################################################
//
//                                   FUNCTIONS CALLED BY CAnalyzer
//
// #####################################################################################################
// #####################################################################################################

// This function is called from CAnalyzer::PrintHtmlHead()
// Gets the line's name that is displayed in HTML head table
CString CSerial::GetLineDescription(DWORD u32_Line)
{
	switch (me_Protocol)
	{
		case E_Proto_Async:
		{
			CString s_Details = gi_App.mi_Func.FormatFrequency(mu32_Baudrate, "Baud");

			switch (me_DataBits)
			{
				case E_5Data: s_Details += ", 5 Data Bits";    break;
				case E_6Data: s_Details += ", 6 Data Bits";    break;
				case E_7Data: s_Details += ", 7 Data Bits";    break;
				case E_8Data: s_Details += ", 8 Data Bits";    break;
				case E_9Data: s_Details += ", 9 Data Bits";    break;
				default:      s_Details += ", Internal error"; break;
			}

			switch (me_Parity)
			{
				case E_ParNone: s_Details += ", No Parity";      break;
				case E_ParEven: s_Details += ", Even Parity";    break;
				case E_ParOdd:  s_Details += ", Odd Parity";     break;
				default:        s_Details += ", Internal error"; break;
			}

			switch (me_StopBits)
			{
				case E_1Stop: s_Details += ", 1 Stop Bit";     break;
				case E_2Stop: s_Details += ", 2 Stop Bits";    break;
				default:      s_Details += ", Internal error"; break;
			}

			switch (u32_Line)
			{
				case 0: return "Asynchronous RxD (" + s_Details + ")";
				case 1: return "Asynchronous TxD (" + s_Details + ")";
			}
			break;
		}
		

		case E_Proto_PS2:
			switch (u32_Line)
			{
				case 0: return "PS/2 - Clock";
				case 1: return "PS/2 - Data";
			}
			break;
		
		case E_Proto_I2C:
			switch (u32_Line)
			{
				case 0: return "I<sup>2</sup>C - SCL";
				case 1: return "I<sup>2</sup>C - SDA";
			}
			break;
		
		case E_Proto_SPI:
			switch (u32_Line)
			{
				case 0: return "SPI - SCLK";
				case 1: return "SPI - MOSI";
				case 2: return "SPI - MISO";
			}
			break;

		case E_Proto_InfraRed:
			switch (u32_Line)
			{
				case 0: return "InfraRed-LED";
			}
			break;

		case E_Proto_SmartCard:
			switch (u32_Line)
			{
				case 0: return "Smartcard - Clock";
				case 1: return "Smartcard - Data";
			}
			break;
	}
	return "";
}

// This function is called for each block AND for each diagram 
// when a very long block is split into multiple GIF images 
void CSerial::AnalyzeSerial()
{
	switch (me_Protocol)
	{
		case E_Proto_Async:     AnalyzeAsync();     break;
		case E_Proto_PS2:       AnalyzePS2();       break;
		case E_Proto_I2C:       AnalyzeI2C();       break;
		case E_Proto_SPI:       AnalyzeSPI();       break;
		case E_Proto_InfraRed:  AnalyzeInfraRed();  break;
		case E_Proto_SmartCard: AnalyzeSmartCard(); break;
	}
}


// #####################################################################################################
// #####################################################################################################
//
//                                          PROTOCOLS
//
// #####################################################################################################
// #####################################################################################################

// RS232, RS422, RS485 protocols
// There is only a data line, no clock line --> the receiver must know the baudrate
// Transfer may be unidirectional or bidirectional (RxD + TxD)
void CSerial::AnalyzeAsync()
{
	double d_Interval = 1.0 / mu32_Baudrate;
	
	for (DWORD Line=0; Line<mk_Active.u32_Count; Line++)
	{
		// Every line needs its own ByteData store and BitIndex counter
		mi_Decode.SetLine(Line);
		BOOL b_Glitch = FALSE;

		for (DWORD S=mk_Block.u32_FirstSample; S<=mk_Block.u32_LastSample; S++)
		{
			if ((mpk_Samples[S+1].u8_Data[CTRL_OFFSET] & CTRL_GLITCH))
				b_Glitch = TRUE;

			int  s32_DataLevel = GetActiveBit(Line, S);
			double d_DataStart = mpk_Samples[S].  d_Elapsed;
			double d_DataEnd   = mpk_Samples[S+1].d_Elapsed;

			if (mi_Decode.GetCurBit() == BIT_SEARCH_HIGH)
			{
				if (s32_DataLevel == 1) 
					mi_Decode.NextBit(); // High level in front of start bit found -> BIT_SEARCH_LOW

				continue;
			}

			if (mi_Decode.GetCurBit() == BIT_SEARCH_LOW)
			{
				if (s32_DataLevel != 0)
					continue;
				
				mi_Decode.NextBit(); // Startbit found (Low) -> BIT_START
					
				// Capture all values in the middle of the Bit.
				// Here the analyzer is synchronized anew to the data stream with every start bit.
				*mi_Decode.BitPos() = d_DataStart + d_Interval / 2; 
			}

			while (*mi_Decode.BitPos() <= d_DataEnd && *mi_Decode.BitPos() <= mk_Block.d_XendExtra)
			{
				CString s_Print;
				DWORD u32_Color;
				BOOL  b_BitOk = ProcessCurBit(s32_DataLevel, &u32_Color, &s_Print);

				// Print current Bit "<", "0..7", "P", ">" or "E" when frame error
				// The samples are taken in the middle of the bit -> Print text centered to the bit
				PrintText(s_Print, *mi_Decode.BitPos(), u32_Color, Line, E_CurveCenter);

				if (!b_BitOk) 
				{
					mi_Decode.Reset(); // Search High Level
					break;
				}

				// The last stop bit has been read
				if (mi_Decode.IsLast())
				{
					char* s8_Formatter = (me_DataBits > 8) ? "0x%03X" : "0x%02X";
					s_Print.Format(s8_Formatter, *mi_Decode.ByteData());

					PrintText(s_Print, *mi_Decode.BitPos(), CGraphics::E_PaletteText, Line, E_FooterCenter);

					// Print decoded Byte into Html and reset b_Glitch
					PrintFooterHtml(s_Print, Line, &b_Glitch);

					mi_Decode.Reset();
					mi_Decode.NextBit(); // We are already within the Stop Bit -> Search Start
					break;
				}

				 mi_Decode.NextBit();
				*mi_Decode.BitPos() += d_Interval;
			}
		}
	}
}

// You find a detailed documentation about PS2 in the folder "Documentation" in "Manual PS2 Bus.chm"
void CSerial::AnalyzePS2()
{
	if (mk_Active.u32_Count < 2)
	{
		mi_Graph.SetColor(CGraphics::E_PaletteRed);
		mi_Graph.DrawText("There must be at least 2 active lines!", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
		return;
	}

	CString s_Print;
	DWORD u32_Color    = 0;
	int   s32_LastClk  = 42; // invalid
	int   s32_LastData = 42; // invalid
	BOOL    b_Glitch   = FALSE;

	for (DWORD S=mk_Block.u32_FirstSample; S<=mk_Block.u32_LastSample; S++)
	{
		if ((mpk_Samples[S+1].u8_Data[CTRL_OFFSET] & CTRL_GLITCH))
			b_Glitch = TRUE;

		int s32_ClkLevel  = GetActiveBit(0, S);
		int s32_DataLevel = GetActiveBit(1, S);

		// Detect a Request To Send (see "Manual PS2 Bus.chm"!)
		if (mi_Decode.IsFirst() && s32_LastClk  == 0 && s32_ClkLevel  == 0 && 
		                           s32_LastData == 1 && s32_DataLevel == 0)
		{
			PrintText("RTS", mpk_Samples[S].d_Elapsed, CGraphics::E_PaletteText, 1, E_FooterCenter);
			// Switch into Host-to-Device mode
			me_Acknowlg = E_AckLast;
			me_Clock    = E_ClkPos;
			BuildBitDefinition();
		}

		if (s32_ClkLevel - s32_LastClk == me_Clock) // negative or positive going edge of clock found
		{
			BOOL b_BitOK = ProcessCurBit(s32_DataLevel, &u32_Color, &s_Print);

			// The samples are taken at the edge of the clock -> Print text left aligned to clock edge
			PrintText(s_Print, mpk_Samples[S].d_Elapsed, u32_Color, 1, E_CurveLeft);

			if (mi_Decode.GetCurBit() == BIT_STOP)
			{
				s_Print.Format("0x%02X", *mi_Decode.ByteData()); // Always 8 Data Bits
				if (me_Clock == E_ClkPos) s_Print += " (Host)";
				PrintText(s_Print, mpk_Samples[S].d_Elapsed, CGraphics::E_PaletteText, 1, E_FooterCenter);

				// Print decoded Byte into Html and reset b_Glitch
				PrintFooterHtml(s_Print, 1, &b_Glitch);
			}

			// Maybe here some more code is required to detect if the Host sends more than one byte
			if (mi_Decode.IsLast())
			{
				// Switch back to Device-to-Host mode
				me_Acknowlg = E_AckNone;
				me_Clock    = E_ClkNeg;
				BuildBitDefinition();
			}

			if (!b_BitOK || mi_Decode.IsLast())
				mi_Decode.Reset(); // reset for next byte
			else 
				mi_Decode.NextBit();
		}
		s32_LastClk  = s32_ClkLevel;
		s32_LastData = s32_DataLevel;
	}
}

//
// You find a detailed documentation about I2C in the folder "Documentation" in the "Manual I2C Bus.pdf"
//
void CSerial::AnalyzeI2C()
{
	if (mk_Active.u32_Count < 2)
	{
		mi_Graph.SetColor(CGraphics::E_PaletteRed);
		mi_Graph.DrawText("There must be at least 2 active lines!", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
		return;
	}

	mi_Graph.SetColor(CGraphics::E_PaletteRed);
	mi_Graph.DrawText("I2C Protocol not yet implemented", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
}

// The SPI bus (Motorola) is quite complicated
// 1 Clock and 2 bidirectional Data lines and various Slave Select lines
// No standard defined --> multiple configurations possible
void CSerial::AnalyzeSPI()
{
	if (mk_Active.u32_Count < 3)
	{
		mi_Graph.SetColor(CGraphics::E_PaletteRed);
		mi_Graph.DrawText("There must be at least 3 active lines!", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
		return;
	}

	mi_Graph.SetColor(CGraphics::E_PaletteRed);
	mi_Graph.DrawText("SPI Protocol not yet implemented", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
}

// Every vendor uses its own Infrared protocol
// You must implement this function for your needs
void CSerial::AnalyzeInfraRed()
{
	// if there is no active line this function will not be called

	mi_Graph.SetColor(CGraphics::E_PaletteRed);
	mi_Graph.DrawText("InfraRed Protocol(s) not yet implemented", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
}

// Every Smartcard uses its own protocol
// You must implement this function for your needs
void CSerial::AnalyzeSmartCard()
{
	if (mk_Active.u32_Count < 2)
	{
		mi_Graph.SetColor(CGraphics::E_PaletteRed);
		mi_Graph.DrawText("There must be at least 2 active lines!", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
		return;
	}

	mi_Graph.SetColor(CGraphics::E_PaletteRed);
	mi_Graph.DrawText("SmartCard Protocol(s) not yet implemented", mk_AxisX.s32_RasterStart, mk_AxisY.s32_RasterEnd, 500, FOOTER_HEIGHT, CGraphics::E_MiddleLeft);
}

// #####################################################################################################
// #####################################################################################################
//
//                                       HELPER FUNCTIONS
//
// #####################################################################################################
// #####################################################################################################


// Processes the next bit with the line level b_DataLevel
// Calculates mi_Decode.ByteData and returns the color and text to be printed for each bit
// returns FALSE on invalid Start or Stop bit or internal error
BOOL CSerial::ProcessCurBit(BOOL b_DataLevel, DWORD* pu32_Color, CString* ps_Print)
{
	char s8_Bit = mi_Decode.GetCurBit();
	
	*pu32_Color = CGraphics::E_PaletteText;
	*ps_Print   = s8_Bit;

	switch (s8_Bit)
	{
	case BIT_START:
		if (b_DataLevel != FALSE) // The start bit must always be 0
		{
			*ps_Print = ""; // This is not a start bit (no error!)
			*mi_Decode.ByteData() = 0;  // Reset
			return FALSE;
		}
		break;

	case '0': // DATA Bits
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
		if (b_DataLevel == TRUE)
			*mi_Decode.ByteData() |= (1 << (s8_Bit - '0'));
		break;

	case BIT_PARITY:
		if (gi_App.mi_Func.CheckParity(*mi_Decode.ByteData(), me_DataBits, b_DataLevel) == me_Parity)
			*pu32_Color = CGraphics::E_PaletteGreen;
		else
			*pu32_Color = CGraphics::E_PaletteRed;
		break;

	case BIT_ACK:
		if (b_DataLevel == FALSE) // Low = Slave has acknowledged
			*pu32_Color = CGraphics::E_PaletteGreen;
		else
			*pu32_Color = CGraphics::E_PaletteRed;
		break;

	case BIT_STOP:
		if (b_DataLevel != TRUE) // The stop bit must always be 1
		{
			*ps_Print   = "E"; // Error
			*pu32_Color = CGraphics::E_PaletteRed;
			*mi_Decode.ByteData() = 0; // The byte is invalid
			return FALSE;
		}
		break;
	}
	return TRUE;
}

// Builds a string where each character represents one bit 
// "<01234567D>" means: Start, Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Parity Odd, Stop
// Use this function only for simple (static) protocols! For I2C it must be programmed dynamically.
void CSerial::BuildBitDefinition()
{
	CString s_Bits;

	switch (me_Protocol)
	{
	case E_Proto_Async:
		s_Bits += BIT_SEARCH_HIGH; // Search a High level on the data line
		s_Bits += BIT_SEARCH_LOW;  // Search the Start Bit on the data line
		break;
	}

	s_Bits += BIT_START;

	if (me_DataOrder == E_LSB) // Least significant bit first
	{
		for (int d=0; d<me_DataBits; d++)
		{
			s_Bits += (char)('0'+d);
		}
	}
	else // Most significant bit first
	{
		for (int d=me_DataBits-1; d>=0; d--)
		{
			s_Bits += (char)('0'+d);
		}
	}

	switch (me_Parity)
	{
		case E_ParEven:
		case E_ParOdd:
			s_Bits += BIT_PARITY;
			break;
	}

	for (int s=0; s<me_StopBits; s++)
	{
		s_Bits += BIT_STOP;
	}

	switch (me_Acknowlg)
	{
		case E_AckLast:
			s_Bits += BIT_ACK;
			break;
	}

	mi_Decode.s_BitDef = s_Bits;
}

// Prints a short text with the given color at the sample position into the curve or footer
void CSerial::PrintText(LPCTSTR s_Text, double d_Elapsed, DWORD u32_Color, DWORD u32_Line, ePrintPos e_Pos)
{
	if ((me_DrawMode & E_DrawImg) == 0)
		return;

	if (!s_Text[0]) return;

	SIZE   k_Size   = mi_Graph.DrawText(s_Text, 0, 0, 0xFFFF, 0xFFFF, CGraphics::E_OnlyMeasure);
	double d_Xpos   = max(mk_Block.d_Xstart, d_Elapsed);
	int  s32_Xpos   = mk_AxisX.s32_RasterStart + (int)((d_Xpos - mk_Block.d_Xstart) * CELL_WIDTH / md_Unit);
	int  s32_Ypos   = mk_AxisY.s32_CurveStart[u32_Line];
	int  s32_Height = CURVE_HEIGHT;

	switch (e_Pos)
	{
		case E_FooterLeft:
		case E_FooterCenter:
			s32_Ypos   = mk_AxisY.s32_LineStart[u32_Line] + CELL_HEIGHT;
			s32_Height = FOOTER_HEIGHT;
			break;
	}

	switch (e_Pos)
	{
		case E_CurveCenter:
		case E_FooterCenter:
		{
			s32_Xpos -= k_Size.cx / 2;
			break;
		}
	}

	// Make text fit into splitted images
	s32_Xpos = max(s32_Xpos, 0);
	s32_Xpos = min(s32_Xpos, mi_Graph.GetDimensions().cx - k_Size.cx);

	mi_Graph.SetColor(u32_Color);
	mi_Graph.DrawText(s_Text, s32_Xpos, s32_Ypos, 500, s32_Height, CGraphics::E_MiddleLeft);
}

// Print the decoded Bytes below the image into a <DIV> tag
// Glitches are marked in front of the Byte that may be corrupt
// b_Glitch is reset to FALSE
void CSerial::PrintFooterHtml(LPCTSTR s_Text, DWORD Line, BOOL* pb_Glitch)
{
	if ((me_DrawMode & E_DrawFooter) == 0)
		return;

	if (*pb_Glitch)
	{
		if (ms_FooterHtml[Line].GetLength() > 0) 
			ms_FooterHtml[Line] += ", ";

		ms_FooterHtml[Line] += " <span class='Glitch'>&nbsp;Glitch&nbsp;</span>";
		*pb_Glitch = FALSE;
	}

	if (ms_FooterHtml[Line].GetLength() > 0) 
		ms_FooterHtml[Line] += ", ";
	
	ms_FooterHtml[Line] += s_Text;
}