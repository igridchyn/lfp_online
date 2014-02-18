
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
#include "Functions.h"
#include <locale.h>

extern CLogicAnalyzerApp gi_App;

BOOL CFunctions::Initialize()
{
	HMODULE h_Kernel = GetModuleHandle("kernel32");
	mf_IsWow64      = (LPFN_IsWow64)     GetProcAddress(h_Kernel, "IsWow64Process");
	mf_Wow64Disable = (LPFN_Wow64Disable)GetProcAddress(h_Kernel, "Wow64DisableWow64FsRedirection");
	mf_Wow64Revert  = (LPFN_Wow64Revert) GetProcAddress(h_Kernel, "Wow64RevertWow64FsRedirection");

	mb_Win64Bit = FALSE;
	#ifdef _WIN64
		mb_Win64Bit = TRUE; // Compiled as 64 Bit
	#else
		if(mf_IsWow64) mf_IsWow64(GetCurrentProcess(), &mb_Win64Bit);
	#endif

	OSVERSIONINFO k_OS;
	k_OS.dwOSVersionInfoSize = sizeof(k_OS);
	GetVersionEx(&k_OS);
	mb_Win9x = (k_OS.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

	// required for CString.Format(" %f ")
	setlocale(LC_ALL, "English");

	__int64 s64_Frequ;
	QueryPerformanceFrequency((LARGE_INTEGER*)&s64_Frequ);
	md_PerfFrequ = (double)s64_Frequ;
	if (!md_PerfFrequ)
	{
		MessageBox(0, "The hardware is very old and does not support a performance counter.", "ElmueSoft Logic Analyzer", MB_ICONSTOP | MB_TOPMOST);
		return FALSE;
	}

	DWORD u32_Err;
	CString s_MyDocs;
	if (u32_Err = GetSpecialFolder(CSIDL_PERSONAL, &s_MyDocs))
	{
		ShowErrorMsg(u32_Err, "while getting My Documents folder");
		return FALSE;
	}

	ms_OutFolder = s_MyDocs + "ElmueSoft-LogicAnalyzer\\"; 
	if (u32_Err = CreateDirectory(ms_OutFolder))
	{
		ShowErrorMsg(u32_Err, "while creating LogicAnalyzer directory");
		return FALSE;
	}

	// If it is not possible to delete the help file this means that it is already open.
	// This is no reason to show an error message -> ignore it.
	CString s_Help = ms_OutFolder + "Help.chm";
	DeleteFile(s_Help);

	if (!CFileEx::IsFile(s_Help))
	{
		if (u32_Err = CFileEx::WriteResource(MAKEINTRESOURCE(IDR_HELP), RT_RCDATA, s_Help))
		{
			ShowErrorMsg(u32_Err, "while copying the file Help.chm");
			return FALSE;
		}
	}

	return TRUE;
}

// return TRUE -> running on a 64 Bit Windows
BOOL CFunctions::IsWin64Bit()
{
	return mb_Win64Bit;
}

// returns TRUE on Windows 95,98,ME
BOOL CFunctions::IsWin9x()
{
	return mb_Win9x;
}

double CFunctions::GetPerfFrequency()
{
	return md_PerfFrequ;
}

CString CFunctions::GetOutFolder()
{
	return ms_OutFolder;
}

// Turns 64 Bit Windows Filesystem Redirection on / off
// This avoids redirection from "C:\Windows\System32" to "C:\Windows\SysWow64"
// ATTENTION: FOR EVERY DISABLE YOU MUST CALL A REVERT AS SOON AS POSSIBLE!!!!
void CFunctions::ToggleWow64(BOOL b_Disable)
{
	if (!mf_Wow64Disable || !mf_Wow64Revert)
		return; // 32 Bit Windows

	if (b_Disable) mf_Wow64Disable(&mp_OldValue);
	else           mf_Wow64Revert(mp_OldValue);
}

// returns "C:\Windows\System32"
DWORD CFunctions::GetSystemDir(CString* ps_SysDir)
{
	char* s8_Buf = ps_SysDir->GetBuffer(MAX_PATH);
	
	DWORD u32_Err = 0;
	DWORD u32_Len = GetSystemDirectory(s8_Buf, MAX_PATH);
	if (!u32_Len)
		 u32_Err = GetLastError();

	ps_SysDir->ReleaseBuffer(u32_Len);

	if (ps_SysDir->Right(1) != "\\")
		*ps_SysDir += "\\";

	return u32_Err;
}

// retuns the address of the resource in memory
// This must NOT be released after usage
// s_Type = RT_RCDATA
// s_ID   = MAKEINTRESOURCE(120)
DWORD CFunctions::GetResource(LPCTSTR s_ID, LPCTSTR s_Type, CString* ps_Data)
{
	char* ps8_Data;
	DWORD u32_Size;
	DWORD u32_Error;
	if (u32_Error = GetResource(s_ID, s_Type, &ps8_Data, &u32_Size))
	{
		ps_Data->Empty();
		return u32_Error;
	}

	char*  s8_Buf = ps_Data->GetBuffer(u32_Size);
	memcpy(s8_Buf, ps8_Data, u32_Size);
	ps_Data->ReleaseBuffer(u32_Size);
	return 0;
}
DWORD CFunctions::GetResource(LPCTSTR s_ID, LPCTSTR s_Type, char** ps8_Data, DWORD *pu32_Size)
{
	HRSRC h_Res = FindResource(NULL, s_ID, s_Type);
	if (!h_Res)
		return GetLastError();

	HGLOBAL h_Glb = LoadResource(NULL, h_Res);
	if (!h_Glb)
		return GetLastError();

	*pu32_Size = SizeofResource(NULL, h_Res);
	if (!*pu32_Size)
		return GetLastError();

	*ps8_Data = (char*)LockResource(h_Glb);
	return 0;
}

DWORD CFunctions::CreateDirectory(CString s_Folder)
{
	if (!::CreateDirectory(s_Folder, 0))
	{
		DWORD u32_Err = GetLastError();
		if (u32_Err != ERROR_ALREADY_EXISTS)
			return u32_Err;
	}
	return 0;
}

// s_Location     = "while creating directory"
// s_ExtraMessage = additional explanation to be appended at the end
void CFunctions::ShowErrorMsg(DWORD u32_Error, CString s_Location, CString s_ExtraMessage)
{
	CString s_Code;
	s_Code.Format("API Error %u %s", u32_Error, s_Location); 

	CString s_Msg;
	const DWORD BUFLEN = 1000;
	char s8_Buf[BUFLEN];

	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, u32_Error, 0, s8_Buf, BUFLEN, 0))
		s_Msg.Format("%s:\n%s", s_Code, s8_Buf);
	else 
		s_Msg.Format("%s:\nWindows has no explanation for this error", s_Code);

	s_Msg.TrimRight(); // some messages end with useless Linefeeds

	if (s_ExtraMessage.GetLength())
		s_Msg += "\n" + s_ExtraMessage;

	if (gi_App.m_pMainWnd && gi_App.m_pMainWnd->m_hWnd)
		MessageBox(gi_App.m_pMainWnd->m_hWnd, s_Msg, "ElmueSoft Logic Analyzer", MB_ICONSTOP);
	else
		MessageBox(0, s_Msg, "ElmueSoft Logic Analyzer", MB_ICONSTOP | MB_TOPMOST);
}

// returns "15 Byte" or "33.199 MB"
// s_Unit = "Byte" or "Samples / Second"
void CFunctions::FormatUnits(double d_Size, CString* ps_Size, CString s_Unit)
{
	const static char* s8_Units[] = { "", "Kilo", "Mega", "Giga" };

	if (d_Size < 1024)
	{
		ps_Size->Format("%d %s", (int)d_Size, s_Unit);
		return;
	}

	int Index = 0;

	while (d_Size >= 1024)
	{
		d_Size /= 1024;
		Index ++;
	}

	ps_Size->Format("%.3f %s %s", d_Size, s8_Units[Index], s_Unit);
}


// u32_FolderID = CSIDL_PERSONAL --> "C:\Documents and Settings\Username\My Documents\"
DWORD CFunctions::GetSpecialFolder(DWORD u32_FolderID, CString *ps_Folder)
{
	*ps_Folder = "";

	LPITEMIDLIST p_IDlist;
	HRESULT hr = SHGetSpecialFolderLocation(0, u32_FolderID, &p_IDlist);
	if (!SUCCEEDED(hr))
		return (DWORD)hr;

	char* s8_Buf = ps_Folder->GetBuffer(MAX_PATH);
	SHGetPathFromIDList(p_IDlist, s8_Buf);
	ps_Folder->ReleaseBuffer();

	if (ps_Folder->Right(1) != "\\")
		*ps_Folder += "\\";
	
	return 0;	
}

// Start a file
DWORD CFunctions::Execute(CString s_Path, BOOL b_Maximize)
{
	SHELLEXECUTEINFO k_Info = { 0 };

    k_Info.cbSize = sizeof(k_Info);
    k_Info.fMask  = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
    k_Info.lpVerb = "open";
    k_Info.lpFile = s_Path;
    k_Info.nShow  = (b_Maximize) ? SW_MAXIMIZE : SW_SHOWNORMAL;

	if (!ShellExecuteEx(&k_Info))
		return GetLastError();

	return 0;
}

// Allowed input = "10,8 us", "22µs", "17.5ms", "39 s", "45sec"
// returns 0.0 on error
double CFunctions::ParseInterval(CString s_Interval)
{
	s_Interval.TrimLeft();
	s_Interval.TrimRight();

	if (s_Interval.GetLength() < 2)
		return 0.0;

	s_Interval.MakeLower();
	s_Interval.Replace("," , "."); // locale has been set to english

	double d_Value = atof(s_Interval);

	if (s_Interval.Right(2) == "ms")
		return d_Value / 1000.0;

	if (s_Interval.Right(2) == "us" || s_Interval.Right(2) == "µs")
		return d_Value / 1000000.0;

	return d_Value;
}

// b_Round=TRUE -> Time is displayed in a raster of 1,2,2.5,5 - 10,20,25,50 - etc
// d_Interval = 0,05542235 --> d_Factor = 0,01, d_Step = 5 --> d_Unit = 0,05
// d_Interval must never be negative
void CFunctions::GetOszilloscopeUnit(double d_Interval, double* pd_Factor, double* pd_Step, BOOL b_Round)
{
	if (d_Interval <= 0.0)
	{
		ASSERT(0);
		*pd_Factor = 1.0;
		*pd_Step   = 1.0;
		return;
	}

	*pd_Factor = 1.0;
	if (d_Interval < 1.0)
	{
		while (d_Interval < 1.0)
		{
			d_Interval *= 10;
			*pd_Factor /= 10;
		}
	}
	else if (d_Interval >= 10.0)
	{
		while (d_Interval >= 10.0)
		{
			d_Interval /= 10;
			*pd_Factor *= 10;
		}
	}

	// Here d_Interval is always between 1.00000.. and 9.99999..

	if (b_Round)
	{
		d_Interval += 0.5; // better rounding

		*pd_Step = 1.0;
		if (d_Interval >= 5.0)
		{
			*pd_Step = 5.0;
		}
		else if (d_Interval >= 2.5)
		{
			*pd_Step = 2.5;
		}
		else if (d_Interval >= 2.0)
		{
			*pd_Step = 2.0;
		}
	}
	else
	{
		*pd_Step = d_Interval;
	}
}

// Get the Html display time (0.002 --> "2 ms")
CString CFunctions::FormatInterval(double d_Interval)
{
	CString s_Out;
	if (d_Interval >= 1.0)
	{
		s_Out.Format("%.1f sec", d_Interval);
		return s_Out;
	}

	d_Interval *= 1000.0;

	if (d_Interval >= 1.0)
	{
		s_Out.Format("%.1f ms", d_Interval);
		return s_Out;
	}

	d_Interval *= 1000.0;

	if (d_Interval >= 1.0)
	{
		s_Out.Format("%.1f µs", d_Interval);
		return s_Out;
	}

	d_Interval *= 1000.0;

	if (d_Interval >= 1.0)
	{
		s_Out.Format("%.1f ns", d_Interval);
		return s_Out;
	}

	return "Invalid value";
}

// Get the frequency (2000 --> "2 kHz")
// Get the baudrate  (4800 --> "4.8 kBaud")
// Get the samperate (4800 --> "4.8 kSamples")
CString CFunctions::FormatFrequency(double d_Frequency, const char* s8_Unit)
{
	CString s_Out;
	if (d_Frequency < 1000.0)
	{
		s_Out.Format("%.0f %s", d_Frequency, s8_Unit);
		return s_Out;
	}

	d_Frequency /= 1000.0;

	if (d_Frequency < 1000.0)
	{
		s_Out.Format("%.1f k%s", d_Frequency, s8_Unit);
		return s_Out;
	}

	d_Frequency /= 1000.0;

	if (d_Frequency < 1000.0)
	{
		s_Out.Format("%.1f M%s", d_Frequency, s8_Unit);
		return s_Out;
	}

	return "Invalid value";
}

// converts the given time into a time with milliseconds precicion
// 125.000 corresponds to 00:02:05.000 o'clock
double CFunctions::CompactTime(SYSTEMTIME* pk_Time)
{
	return  (double)pk_Time->wSecond
	      + (double)pk_Time->wMinute * 60
		  + (double)pk_Time->wHour * 3600;
}

// returns time in the form "15:26:15.435.234" with microsecond resolution
// d_Time must be passed as seconds since 00:00 o'clock from CFunctions::CompactTime()
CString CFunctions::FormatExactTime(double d_Time)
{
	int s32_Time  = (int) d_Time;
	int s32_Hour  = (s32_Time / 3600);
	int s32_Min   = (s32_Time / 60) % 60;
	int s32_Sec   = (s32_Time)      % 60;

	double d_Frac = d_Time - s32_Time;
	int s32_Milli = (int)(d_Frac * 1000.0)    % 1000;
	int s32_Micro = (int)(d_Frac * 1000000.0) % 1000;

	CString s_Out;
	s_Out.Format("%02d:%02d:%02d.%03d.%03d", s32_Hour, s32_Min, s32_Sec, s32_Milli, s32_Micro);
	return s_Out;
}

// Parse "17:35:55" or "17:35:55.231" or "17:35:55.321.887"
// Empty strings are allowed
BOOL CFunctions::ParseExactTime(CString s_Time, double* pd_Time)
{
	DWORD u32_Hour, u32_Min, u32_Sec, u32_Milli, u32_Micro;
	char* s8_Buffer = s_Time.GetBuffer(0);
	*pd_Time = 0.0;

	s_Time.TrimLeft ();
	s_Time.TrimRight();
	if (s_Time.GetLength() == 0)
		return TRUE;
	
	if (!GetNextNumber(&s8_Buffer, ':', &u32_Hour,  FALSE))
		return FALSE;
	if (!GetNextNumber(&s8_Buffer, ':', &u32_Min,   FALSE))
		return FALSE;
	if (!GetNextNumber(&s8_Buffer, '.', &u32_Sec,   FALSE))
		return FALSE;
	if (!GetNextNumber(&s8_Buffer, '.', &u32_Milli, TRUE))
		return FALSE;
	if (!GetNextNumber(&s8_Buffer,  0,  &u32_Micro, TRUE))
		return FALSE;

	if (u32_Hour > 23 || u32_Min > 59 || u32_Sec > 59 || u32_Milli > 999 || u32_Micro > 999)
		return FALSE;

	*pd_Time = 3600.0 * u32_Hour + 60.0 * u32_Min + u32_Sec + 0.001 * u32_Milli + 0.000001 * u32_Micro;
	return TRUE;
}

// Tries to read the following integer number up to the next delimiter
// Increments the pointer behind the limiter
// Returns FALSE if there is no number or an invalid character
// b_Optional = TRUE: If the number does not exist -> return pu32_Number = 0 and error only on invalid characters
BOOL CFunctions::GetNextNumber(char** pps_String, char c_Delimiter, DWORD* pu32_Number, BOOL b_Optional)
{
	*pu32_Number = 0;
	BOOL b_Found = FALSE;

	while (TRUE)
	{
		char c_Char = (*pps_String)[0];
		if (!c_Char)
			break; // String End

		(*pps_String) ++;
		
		if (c_Char == c_Delimiter)
			break;

		if (c_Char >= '0' && c_Char <= '9')
		{
			b_Found = TRUE;
			*pu32_Number *= 10;
			*pu32_Number += (c_Char - '0');
		}
		else return FALSE; // invalid character
	}
	return (b_Found || b_Optional);
}

// Deletes all files in a folder
// s_Filter = "Image_*.gif"
void CFunctions::DeleteAllFiles(CString s_Folder, CString s_Filter)
{
	if (s_Folder.Right(1) != "\\") s_Folder += "\\";

	WIN32_FIND_DATA k_Find;
	HANDLE h_File = FindFirstFile(s_Folder + s_Filter, &k_Find);

	if (h_File == INVALID_HANDLE_VALUE)
		return; // no files
	do
	{
		if (strcmp(k_Find.cFileName, ".")  == 0 || 
			strcmp(k_Find.cFileName, "..") == 0)
			continue;

		if (k_Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// This function does not pase subdirectories
		}
		else
		{
			DeleteFile(s_Folder + k_Find.cFileName);
		}
	}
	while (FindNextFile(h_File, &k_Find));

	FindClose(h_File);
}

// Calculates the Parity of the given Byte
// Parity odd  = The sum of all bits INCLUDING the parity bit is odd  = 1 = TRUE
// Parity even = The sum of all bits INCLUDING the parity bit is even = 0 = FALSE
BOOL CFunctions::CheckParity(WORD u16_Data, int s32_BitCount, BOOL b_Parity)
{
	for (int Bit=0; Bit<s32_BitCount; Bit++)
	{
		if (u16_Data & (1 << Bit))
			b_Parity = !b_Parity;
	}
	return b_Parity;
}