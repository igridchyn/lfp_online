
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
#include "Graphics.h"
#include "Port.h"
#include "conio.h"

extern CLogicAnalyzerApp gi_App;

// ###############################################################################################
//                                         SPEAKER
// ###############################################################################################

/*
Chip 8253:
Counter Clock = 1.193.180 MHz
Port 40: Counter 0     (18,2 Hz)
Port 41: Counter 1     (DRAM Refresh)
Port 42: Counter 2     (Speaker (16 Bit))
Port 43: Logic Control (= 0xB6)
Port 61: AND Gates     (Bit 0: Enable Counter 2 output,
                        Bit 1: Enable Speaker)

Port 43:

Bit 0 = 0 : Counters are binary numbers
-------------
Bit 1 = 1 
Bit 2 = 1
Bit 3 = 0 : Square wave 
-------------
Bit 4 = 1 
Bit 5 = 1 : First load least significante Byte then most signifiacant Byte
-------------
Bit 6 = 0
Bit 7 = 1 : Counter 2 is concerned 

*/
// Let the PC loudspeaker beep for u32_Duration milliseconds with a frequency of d_Frequ Hz
// This function should be called from a separate thread
DWORD CPort::BeepSpeaker(double d_Frequ, DWORD u32_Duration)
{
	if (d_Frequ < 20 || d_Frequ > 10000)
		return ERROR_INVALID_PARAMETER;

	DWORD u32_Timer = (DWORD)(((double)1193180) / d_Frequ);

	DWORD u32_Err;
	BYTE u8_OldSpkr;
	u32_Err = gi_App.mi_Driver.ReadIoPortByte(0x61, &u8_OldSpkr);
	if (u32_Err)
		return u32_Err;

	u32_Err = gi_App.mi_Driver.WriteIoPortByte(0x43, 0xB6);  // 1011 0110
	if (u32_Err)
		return u32_Err;

	u32_Err = gi_App.mi_Driver.WriteIoPortByte(0x42, (BYTE)(u32_Timer % 256));
	if (u32_Err)
		return u32_Err;

	u32_Err = gi_App.mi_Driver.WriteIoPortByte(0x42, (BYTE)(u32_Timer / 256));
	if (u32_Err)
		return u32_Err;

	// Speaker ON
	u32_Err = gi_App.mi_Driver.WriteIoPortByte(0x61, u8_OldSpkr | 3); 
	if (u32_Err)
		return u32_Err;

	Sleep(u32_Duration);

	// Speaker OFF
	return gi_App.mi_Driver.WriteIoPortByte(0x61, u8_OldSpkr & ~ 3);
}

// ###############################################################################################
//                                    PARALLEL PORT
// ###############################################################################################


// Switches the bidrectional Data Port into Read mode 
// and writes TRUE to all in/out lines to release the Open Collectors
void CPort::InitializePorts(WORD u16_BasePort)
{
	// ECP Control Register Port:
	// Enable Byte Mode, Turn off DMA and ECP Interrupts
	BYTE u8_ECR = E_Byte_Mode;
	gi_App.mi_Driver.WriteIoPortByte(u16_BasePort + ECR_OFFSET, u8_ECR);

	// ---------------------------------------------------------------

	// STATUS Port:
	// This Port is Read Only -> no initialization required

	// ---------------------------------------------------------------

	// CTRL Port:
	// Turn off all 4 open collector transistors (set HIGH) and turn off Data Port Output (set Bit 5)
	BYTE u8_Ctrl = CTRL_USED_AS_INPUT ^ CTRL_INVERTED;
	gi_App.mi_Driver.WriteIoPortByte(u16_BasePort + CTRL_OFFSET, u8_Ctrl);

	// ---------------------------------------------------------------
	
	// DATA Port:
	// Turn off all 8 open collector transistors (set HIGH)
	BYTE u8_Data = DATA_USED_AS_INPUT ^ DATA_INVERTED;
	gi_App.mi_Driver.WriteIoPortByte(u16_BasePort + DATA_OFFSET, u8_Data);
}

// returns in pu8_Mem[0-2] 3 Bytes for Data, Status, Control Port
DWORD CPort::GetPortData(WORD u16_BasePort, DWORD u32_PortMask, BYTE* pu8_Mem)
{
	DWORD u32_Err;
	BYTE  u8_Byte = 0;
	if (u32_PortMask & DATA_MASK)
	{
		u32_Err = gi_App.mi_Driver.ReadIoPortByte(u16_BasePort + DATA_OFFSET, &u8_Byte);
		if (u32_Err)
			return u32_Err;

		u8_Byte &= DATA_USED_AS_INPUT;
		u8_Byte ^= DATA_INVERTED;
	}
	pu8_Mem[DATA_OFFSET] = u8_Byte;

	u8_Byte = 0;
	if (u32_PortMask & STATUS_MASK)
	{
		u32_Err = gi_App.mi_Driver.ReadIoPortByte(u16_BasePort + STATUS_OFFSET, &u8_Byte);
		if (u32_Err)
			return u32_Err;

		u8_Byte &= STATUS_USED_AS_INPUT;
		u8_Byte ^= STATUS_INVERTED;
	}
	pu8_Mem[STATUS_OFFSET] = u8_Byte;

	u8_Byte = 0;
	if (u32_PortMask & CTRL_MASK)
	{
		u32_Err = gi_App.mi_Driver.ReadIoPortByte(u16_BasePort + CTRL_OFFSET, &u8_Byte);
		if (u32_Err)
			return u32_Err;

		u8_Byte &= CTRL_USED_AS_INPUT;
		u8_Byte ^= CTRL_INVERTED;
	}
	pu8_Mem[CTRL_OFFSET] = u8_Byte;
	return 0;
}

// pu8_Mem[0-2] contains the data read from the 3 ports via GetPortData()
// This function returns one of the Bits in these 3 bytes
// returns 0 or 1 corresponding to the bit's state or -1 if the bit is unused
int CPort::GetPortBit(BYTE* pu8_Mem, DWORD u32_PortOffset, DWORD u32_Bit)
{
	switch (u32_PortOffset)
	{
		case DATA_OFFSET:
		{
			switch (u32_Bit)
			{
				case 0:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_0) > 0;
				case 1:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_1) > 0;
				case 2:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_2) > 0;
				case 3:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_3) > 0;
				case 4:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_4) > 0;
				case 5:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_5) > 0;
				case 6:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_6) > 0;
				case 7:  return (pu8_Mem[DATA_OFFSET] & DATA_LINE_7) > 0;
				default: return -1;
			}
		}
		case STATUS_OFFSET:
		{
			switch (u32_Bit)
			{
				case 0:  return (pu8_Mem[STATUS_OFFSET] & STATUS_LINE_0) > 0;
				case 1:  return (pu8_Mem[STATUS_OFFSET] & STATUS_LINE_1) > 0;
				case 2:  return (pu8_Mem[STATUS_OFFSET] & STATUS_LINE_2) > 0;
				case 3:  return (pu8_Mem[STATUS_OFFSET] & STATUS_LINE_3) > 0;
				case 4:  return (pu8_Mem[STATUS_OFFSET] & STATUS_LINE_4) > 0;
				default: return -1;
			}
		}
		case CTRL_OFFSET:
		{
			switch (u32_Bit)
			{
				case 0:  return (pu8_Mem[CTRL_OFFSET] & CTRL_LINE_0) > 0;
				case 1:  return (pu8_Mem[CTRL_OFFSET] & CTRL_LINE_1) > 0;
				case 2:  return (pu8_Mem[CTRL_OFFSET] & CTRL_LINE_2) > 0;
				case 3:  return (pu8_Mem[CTRL_OFFSET] & CTRL_LINE_3) > 0;
				default: return -1;
			}
		}
		default: // Invalid Mask
			return -1;
	}
}

// ###############################################################################################
//                                         MEASURE SPEED
// ###############################################################################################

// Measure the maximum speed when reading in s32_Loops loops from the parallel port(s).
// pd_Times = double[s32_Loops] receives the loop duration for each cycle
// pd_Shortest and pd_Longest receive the fastest and the slowest loop
DWORD CPort::MeasureMaxParallelSpeed(WORD   u16_BasePort, DWORD  u32_PortMask, 
									 double* pd_Times,    int    s32_Loops,
									 double* pd_Shortest, double* pd_Longest)
{
//	SetThreadPriority(..) is completely useless here!! 

	__int64 s64_Counter = 0;
	__int64 s64_LastCounter = 0;

	double d_PerfFrequ = gi_App.mi_Func.GetPerfFrequency();
	*pd_Longest  = 0;
	*pd_Shortest = 9999999; // seconds!!
	BYTE u8_Data[PORT_COUNT];

	for (int i=-1; i<s32_Loops; i++)
	{
		DWORD u32_Err = GetPortData(u16_BasePort, u32_PortMask, u8_Data);
		if (u32_Err)
			return u32_Err;

		QueryPerformanceCounter((LARGE_INTEGER*)&s64_Counter);

		// ATTENTION:
		// Normally a performance Counter value is NEVER smaller than a previous performance counter value was.
		// But due to BIOS bugs or bugs in the HAL this may happen when a thread is moved to another processor.
		// In this case you must use SetThreadAffinityMask() which normally is neither required nor recommended.
		// The following command is NOT compiled into Release builds
		ASSERT(s64_Counter > s64_LastCounter);

		if (i >= 0)
		{
			double d_Elapsed = (double)(s64_Counter - s64_LastCounter) / d_PerfFrequ;

			if (pd_Times)     pd_Times[i] = d_Elapsed;
			if (pd_Longest)  *pd_Longest  = max(*pd_Longest,  d_Elapsed);
			if (pd_Shortest) *pd_Shortest = min(*pd_Shortest, d_Elapsed);
		}
		s64_LastCounter = s64_Counter;
	}
	return 0;
}

// Create a GIF graphic with the speed of 10000 speed measure loops
DWORD CPort::CreateSpeedDiagram(WORD u16_BasePort, DWORD u32_PortMask)
{
	// Defines how often the Speed measure loop runs
	const DWORD SPEED_LOOP_COUNT = 10000;

	// Defines how many Speed Measure loops are displayed per pixel on the X axis of the GIF diagram
	const DWORD SPEED_LOOPS_PER_PIXEL = 1;

	// ATTENTION: The GIF format does not allow images wider than 65520 pixels.
	if (!SPEED_LOOPS_PER_PIXEL || SPEED_LOOP_COUNT / SPEED_LOOPS_PER_PIXEL >= 0xFFF0)
	{
		ASSERT(0);
		return ERROR_INVALID_PARAMETER;
	}

	DWORD u32_Err = 0;
	CString s_Html, s_Temp;
	CString s_GifFile = gi_App.mi_Func.GetOutFolder() + "SpeedDiagram.gif";
	CString s_HtmFile = gi_App.mi_Func.GetOutFolder() + "SpeedDiagram.htm";

	DWORD u32_PortCount = 0;
	if (u32_PortMask & DATA_MASK)   u32_PortCount++;
	if (u32_PortMask & CTRL_MASK)   u32_PortCount++;
	if (u32_PortMask & STATUS_MASK) u32_PortCount++;

	double* pd_Times = new double[SPEED_LOOP_COUNT];
	double  d_Longest, d_Shortest;
	if (u32_Err = MeasureMaxParallelSpeed(u16_BasePort, u32_PortMask, pd_Times, SPEED_LOOP_COUNT, &d_Shortest, &d_Longest))
		goto _Exit;

	if (u32_Err = DrawSpeedGraphic(s_GifFile, pd_Times, SPEED_LOOP_COUNT, SPEED_LOOPS_PER_PIXEL, d_Longest))
		goto _Exit;
		
	if (u32_Err = gi_App.mi_Func.GetResource(MAKEINTRESOURCE(IDR_HTML_TEMPLATE), RT_RCDATA, &s_Html))
		goto _Exit;

	s_Temp.Format((CString)"This diagram shows the result of %d cycles of the speed measure loop while scanning %d Port(s).<br>\r\n"+
	              "Each pixel on the X axis represents %d loop cycle(s).<br>\r\n", 
				  SPEED_LOOP_COUNT, u32_PortCount, SPEED_LOOPS_PER_PIXEL);
	s_Html += s_Temp;
	s_Html += "The Y axis shows the time period that each loop was running.<br>\r\n";
	s_Html += "The fastest loop took " + gi_App.mi_Func.FormatInterval(d_Shortest);
	s_Html += " ( = " + gi_App.mi_Func.FormatFrequency(1.0 / d_Shortest, "Hz") + ").<br>\r\n";
	s_Html += "The slowest loop took " + gi_App.mi_Func.FormatInterval(d_Longest);
	s_Html += " ( = " + gi_App.mi_Func.FormatFrequency(1.0 / d_Longest, "Hz") + ").<br>\r\n";
	s_Html += "Glitches in the diagram are the result of context switches / driver activity.<br>\r\n";
	s_Html += "<br><br><img src='SpeedDiagram.gif'>\r\n";

	if (u32_Err = CFileEx::Save(s_HtmFile, s_Html, s_Html.GetLength()))
		goto _Exit;

	if (u32_Err = gi_App.mi_Func.Execute(s_HtmFile, TRUE))
		goto _Exit;

	_Exit:
	delete pd_Times;
	return u32_Err;
}

// Draw a diagram with height=300 pixel and width=u32_Count/u32_LoopsPerPixel pixel
// Each pixel on the X axis corresponds to one loop cycle of the MeasureMaxParallelSpeed() loop
// The Y axis shows the duration of that loop in microseconds
DWORD CPort::DrawSpeedGraphic(CString s_GifFile, double* pd_Values, DWORD u32_Count, DWORD u32_LoopsPerPixel, double d_Longest)
{
	CGraphics i_Graphics;

	const DWORD u32_Height    = 300; // Height of graph in pixels
	const DWORD u32_LblHeight =  20; // The height of all labels
	const DWORD u32_LblWidth  =  80; // The width  of the labels for the Y Axis
	const DWORD u32_LinesY    =  10; // Total count of Y raster lines
	const DWORD u32_LabelX    =   5; // Draw every 5 raster lines a label on the X axis
	const DWORD u32_DistX     =  50; // Distance of X raster lines

	double d_Factor, d_Step;
	gi_App.mi_Func.GetOszilloscopeUnit(d_Longest, &d_Factor, &d_Step, true);

	double d_Max = 0;
	do
	{
		d_Max += d_Factor * d_Step;
	}
	while (d_Max < d_Longest);

	DWORD u32_Width = u32_Count/u32_LoopsPerPixel;

	DWORD u32_Err = i_Graphics.CreateNewDiagram(u32_LblWidth + u32_Width, u32_LblHeight + u32_Height);
	if (u32_Err)
		return u32_Err;

	CGraphics::eRasterMode e_RasterX = CGraphics::GetRasterMode(10.0 / u32_LabelX);
	CGraphics::eRasterMode e_RasterY = CGraphics::GetRasterMode(d_Step);

	// Draw the raster
	i_Graphics.DrawRaster(u32_LblWidth, 0, u32_Width+u32_LblWidth, u32_Height, 
	                      u32_DistX, u32_Height/u32_LinesY, e_RasterX, e_RasterY);

	i_Graphics.SetColor(CGraphics::E_PaletteText);
	i_Graphics.SetFont("Verdana", 12);

	// Draw Y axis lables
	for (DWORD Y=1; Y<u32_LinesY; Y++)
	{
		CString s_Label = gi_App.mi_Func.FormatInterval(d_Max / u32_LinesY * Y);
		i_Graphics.DrawText(s_Label, 0, (u32_LinesY-Y)*(u32_Height/u32_LinesY)-(u32_LblHeight/2), 
		                    u32_LblWidth-5, u32_LblHeight, CGraphics::E_MiddleRight);
	}

	// Draw X axis lables
	double d_TimeX = 0;
	for (DWORD X=0; X<u32_Count; X++)
	{
		d_TimeX += pd_Values[X];

		if (X>0 && X % (u32_LabelX * u32_DistX * u32_LoopsPerPixel) == 0)
		{
			CString s_Label = gi_App.mi_Func.FormatInterval(d_TimeX);
			i_Graphics.DrawText(s_Label, u32_LblWidth + X/u32_LoopsPerPixel, u32_Height, 1000, u32_LblHeight, CGraphics::E_MiddleLeft);
		}
	}

	// Draw the graph
	i_Graphics.SetColor(CGraphics::E_PaletteGreen);
	i_Graphics.SetLineMode(CGraphics::E_LineSolid);

	int X1,X2,Y1,Y2;
	for (DWORD i=0; i<u32_Count; i++)
	{
		X2 = i;
		Y2 = u32_Height - (int)(pd_Values[i] / d_Max * u32_Height);
		if (i>0)
		{
			// X=0,Y=0 is the top left corner
			i_Graphics.DrawLine(u32_LblWidth + X1/u32_LoopsPerPixel, Y1,
			                    u32_LblWidth + X2/u32_LoopsPerPixel, Y2);
		}
		X1 = X2;
		Y1 = Y2;
	}

	return i_Graphics.SaveAsGif(s_GifFile);
}

// ###############################################################################################
//                                            CAPTURE
// ###############################################################################################


DWORD WINAPI CaptureThreadApi(void* p_Arg)
{
	((CPort*)p_Arg)->CaptureThread();
	return 0;
}

// Allocate memory and start a thread that captures into memory
BOOL CPort::StartCapture(WORD u16_BasePort, DWORD u32_MemSize, DWORD u32_PortMask)
{
	// Just for debugging: Load capture data from file
	#if LOAD_CAPTURE_FROM_FILE
		CString s_File = gi_App.mi_Func.GetOutFolder() + "Capture.bin";
		CFileEx i_File;
		DWORD u32_Err;
		if (u32_Err = i_File.Open(s_File, CFileEx::E_Read))
		{
			gi_App.mi_Func.ShowErrorMsg(u32_Err, "while opening Capture.bin file");
			return FALSE;
		}

		if (u32_Err = i_File.Read((char*)&mk_Capture, sizeof(kCapture)))
		{
			gi_App.mi_Func.ShowErrorMsg(u32_Err, "while reading Capture.bin file");
			return FALSE;
		}
		
		if (!AllocateMem(mk_Capture.u32_Samples * sizeof(kSample)))
			return FALSE;

		if (u32_Err = i_File.Read((char*)mk_Memory.pk_Samples, mk_Memory.u32_MemSize))
		{
			gi_App.mi_Func.ShowErrorMsg(u32_Err, "while reading Capture.bin file");
			return FALSE;
		}
		i_File.Close();

		gi_App.m_pMainWnd->SetTimer(ID_TIMER_CAPTURE_FINISHED, 100, 0);
		return TRUE;
	#endif

	if (!u32_PortMask)
	{
		MessageBox(gi_App.m_pMainWnd->m_hWnd, "Select the port(s) that you want to capture!", "ElmueSoft Logic Analyzer", MB_ICONSTOP);
		return FALSE; 
	}

	mk_Capture.u16_BasePort  = u16_BasePort;
	mk_Capture.u32_PortMask  = u32_PortMask;
	mk_Capture.b_Abort       = FALSE;
	mk_Capture.u32_Samples   = 0;
	mk_Capture.u32_LastError = 0;
	GetLocalTime(&mk_Capture.k_StartTime);

	if (!AllocateMem(u32_MemSize))
		return FALSE;

	DWORD u32_ThreadID;
	mh_Thread = CreateThread(0, 0, &CaptureThreadApi, this, 0, &u32_ThreadID);
	return TRUE;
}

BOOL CPort::AllocateMem(DWORD u32_MemSize)
{
	if (mk_Memory.pk_Samples && mk_Memory.u32_MemSize == u32_MemSize)
		return TRUE;
	
	HeapFree(GetProcessHeap(), 0, mk_Memory.pk_Samples);

	mk_Memory.pk_Samples = (kSample*)HeapAlloc(GetProcessHeap(), 0, u32_MemSize);
	if (!mk_Memory.pk_Samples)
	{
		gi_App.mi_Func.ShowErrorMsg(GetLastError(), "while allocating memory");
		return FALSE;
	}
	mk_Memory.u32_MemSize = u32_MemSize;
	return TRUE;
}

void CPort::StopCapture()
{
	mk_Capture.b_Abort = TRUE;
}

void CPort::CaptureThread()
{
//	SetThreadPriority(..) is completely useless here!!

	double    d_PerfFrequ = gi_App.mi_Func.GetPerfFrequency();
	__int64 s64_PerfCounter;
	__int64 s64_InitCounter;
	__int64 s64_LastCounter;
	__int64 s64_Limit;

	kSample* pk_CurSample  = mk_Memory.pk_Samples;
	kSample* pk_LastSample = mk_Memory.pk_Samples;
	DWORD    u32_Last      = mk_Memory.u32_MemSize / sizeof(kSample);

	double d_Shortest, d_Longest;
	mk_Capture.u32_LastError = MeasureMaxParallelSpeed(mk_Capture.u16_BasePort, mk_Capture.u32_PortMask, NULL, 100, &d_Shortest, &d_Longest);
	if (mk_Capture.u32_LastError)
		goto _Exit;

	// If a loop runs longer than s64_Limit it will be marked as "glitch" (Context Switch)
	s64_Limit = (__int64)(d_Shortest * MIN_GLITCH_LENGTH * d_PerfFrequ);

	QueryPerformanceCounter((LARGE_INTEGER*)&s64_InitCounter);
	s64_LastCounter = s64_InitCounter;

	// Store the first sample
	mk_Capture.u32_LastError = GetPortData(mk_Capture.u16_BasePort, mk_Capture.u32_PortMask, pk_CurSample->u8_Data);
	if (mk_Capture.u32_LastError)
		goto _Exit;

	pk_CurSample->d_Elapsed = 0.0;
	pk_CurSample ++;
	mk_Capture.u32_Samples = 1;

	// This loop is optimized for maximum speed
	while (mk_Capture.u32_Samples < u32_Last)
	{
		mk_Capture.u32_LastError = GetPortData(mk_Capture.u16_BasePort, mk_Capture.u32_PortMask, pk_CurSample->u8_Data);
		if (mk_Capture.u32_LastError)
			goto _Exit;

		QueryPerformanceCounter((LARGE_INTEGER*)&s64_PerfCounter);

		// ATTENTION:
		// Normally a performance Counter value is NEVER smaller than a previous performance counter value was.
		// But due to BIOS bugs or bugs in the HAL this may happen when a thread is moved to another processor.
		// In this case you must use SetThreadAffinityMask() which normally is neither required nor recommended.
		// The following command is NOT compiled into Release builds
		ASSERT(s64_PerfCounter > s64_LastCounter);

		if (s64_PerfCounter - s64_LastCounter > s64_Limit)
		{
			// Store the sample BEFORE the glitch
			pk_CurSample->d_Elapsed = (double)(s64_LastCounter - s64_InitCounter) / d_PerfFrequ;

			pk_CurSample  ++;
			pk_LastSample ++;
			mk_Capture.u32_Samples ++;

			s64_LastCounter = s64_PerfCounter;

			// Store the glitch ITSELF, set the Glitch Bit
			pk_CurSample->u8_Data[DATA_OFFSET]   = pk_LastSample->u8_Data[DATA_OFFSET];
			pk_CurSample->u8_Data[STATUS_OFFSET] = pk_LastSample->u8_Data[STATUS_OFFSET];
			pk_CurSample->u8_Data[CTRL_OFFSET]   = pk_LastSample->u8_Data[CTRL_OFFSET] | (BYTE) CTRL_GLITCH;
		}
		else
		{
			s64_LastCounter = s64_PerfCounter;

			// Wait for a change on any of the 17 input lines
			if ((*((DWORD*)pk_CurSample ->u8_Data) & ALL_USED_AS_INPUT) == 
				(*((DWORD*)pk_LastSample->u8_Data) & ALL_USED_AS_INPUT) &&
				!mk_Capture.b_Abort) // Check HERE!! (not in while(..b_Abort))
					continue;
		}

		pk_CurSample->d_Elapsed = (double)(s64_PerfCounter - s64_InitCounter) / d_PerfFrequ;

		pk_CurSample  ++;
		pk_LastSample ++;
		mk_Capture.u32_Samples ++;

		// Exit the loop only here --> assure that always the last sample will be written
		if (mk_Capture.b_Abort)
			break;
	}

	_Exit:	
	// Just for debugging: Save capture data to file
	#if SAVE_CAPTURE_TO_FILE
		CString s_File = gi_App.mi_Func.GetOutFolder() + "Capture.bin";
		DWORD u32_Err;
		if (u32_Err = CFileEx::Save(s_File, (const char*)&mk_Capture, sizeof(kCapture), FALSE))
			gi_App.mi_Func.ShowErrorMsg(u32_Err, "while writing Capture.bin file");
		// Append
		else if (u32_Err = CFileEx::Save(s_File, (const char*)mk_Memory.pk_Samples, mk_Capture.u32_Samples * sizeof(kSample), TRUE))
			gi_App.mi_Func.ShowErrorMsg(u32_Err, "while writing Capture.bin file");
	#endif

	// ATTENTION: The GUI runs in another thread -> don't access GUI controls directly from here!
	gi_App.m_pMainWnd->SetTimer(ID_TIMER_CAPTURE_FINISHED, 100, 0);

	CloseHandle(mh_Thread);
}

// returns how many bytes have been captured into memory / file
DWORD CPort::GetCapturedBytes()
{
	return mk_Capture.u32_Samples * sizeof(kSample);
}

// access the captured data
void CPort::GetCaptureBuffer(kSample** ppk_Samples, DWORD* pu32_Samples, DWORD* pu32_PortMask, double* pd_ClockTime)
{
	*ppk_Samples   = mk_Memory.pk_Samples;
	*pu32_Samples  = mk_Capture.u32_Samples;
	*pu32_PortMask = mk_Capture.u32_PortMask;
	
	#if CLOCK_TIME
		*pd_ClockTime = gi_App.mi_Func.CompactTime(&mk_Capture.k_StartTime);
	#else
		*pd_ClockTime = 0.0;
	#endif
}

void CPort::GetCaptureTime(double* pd_StartTime, double* pd_EndTime)
{
	*pd_StartTime = mk_Memory.pk_Samples[0].d_Elapsed;
	*pd_EndTime   = mk_Memory.pk_Samples[mk_Capture.u32_Samples-1].d_Elapsed;

	#if CLOCK_TIME
		double d_ClockTime = gi_App.mi_Func.CompactTime(&mk_Capture.k_StartTime);
		*pd_StartTime += d_ClockTime;
		*pd_EndTime   += d_ClockTime;
	#endif
}

DWORD CPort::GetCaptureLastError()
{
	return mk_Capture.u32_LastError;
}
