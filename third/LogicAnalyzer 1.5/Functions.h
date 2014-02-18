
#pragma once

#include <windows.h>
#include <winsvc.h>

class CFunctions  
{
	typedef BOOL (WINAPI* LPFN_IsWow64)     (HANDLE hProcess, PBOOL Wow64Process);
	typedef BOOL (WINAPI* LPFN_Wow64Disable)(PVOID *OldValue);
	typedef BOOL (WINAPI* LPFN_Wow64Revert) (PVOID  OldValue);

public:
	BOOL    Initialize();
	BOOL    IsWin64Bit();
	BOOL    IsWin9x();
	CString GetOutFolder();
	double  GetPerfFrequency();
	void    ToggleWow64(BOOL b_Disable);
	DWORD   GetResource(LPCTSTR s_ID, LPCTSTR s_Type, CString* ps_Data);
	DWORD   GetResource(LPCTSTR s_ID, LPCTSTR s_Type, char** ps8_Data, DWORD *pu32_Size);
	DWORD   CreateDirectory(CString s_Folder);
	void    DeleteAllFiles(CString s_Folder, CString s_Filter);
	DWORD   GetSystemDir(CString* ps_SysDir);
	void    ShowErrorMsg(DWORD u32_Error, CString s_Location, CString s_ExtraMessage="");
	void    FormatUnits(double d_Size, CString* ps_Size, CString s_Unit);
	DWORD   GetSpecialFolder(DWORD u32_FolderID, CString *ps_Folder);
	DWORD   Execute(CString s_Path, BOOL b_Maximize);
	double  ParseInterval(CString s_Interval);
	void    GetOszilloscopeUnit(double d_Interval, double* pd_Factor, double* pd_Step, BOOL b_Round);
	CString FormatInterval(double d_Interval);
	CString FormatFrequency(double d_Frequency, const char* s8_Unit);
	CString FormatExactTime(double d_Interval);
	double  CompactTime(SYSTEMTIME* pk_Time);
	BOOL    CheckParity(WORD u16_Data, int s32_BitCount, BOOL b_Parity);
	BOOL    ParseExactTime(CString s_Time, double* pd_Time);

private:
	BOOL    GetNextNumber(char** ps_String, char c_Delimiter, DWORD* pu32_Number, BOOL b_Optional);

	LPFN_IsWow64      mf_IsWow64;
	LPFN_Wow64Disable mf_Wow64Disable;
	LPFN_Wow64Revert  mf_Wow64Revert;
	void*             mp_OldValue;

	double   md_PerfFrequ;
	CString  ms_OutFolder;
	BOOL     mb_Win64Bit;
	BOOL     mb_Win9x;
};


