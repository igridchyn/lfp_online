
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
#include "FileEx.h"
#include "Functions.h"

extern CLogicAnalyzerApp gi_App;

// ******************************************************************************
//  The MFC class CFile is useless because it throws exceptions.
//  This class returns error codes as it is the standard in this entire project
// ******************************************************************************

CFileEx::CFileEx()
{
	mh_File = 0;
}

CFileEx::~CFileEx()
{
	Close();
}

// ====================== STATIC ======================

// retruns TRUE if the file exists
BOOL CFileEx::IsFile(LPCTSTR s_File)
{
	gi_App.mi_Func.ToggleWow64(TRUE);

	DWORD u32_Attr = GetFileAttributes(s_File);

	gi_App.mi_Func.ToggleWow64(FALSE);

	return (u32_Attr != INVALID_FILE_ATTRIBUTES);
}

// Save an embedded resoure into a file
DWORD CFileEx::WriteResource(LPCTSTR s_ID, LPCTSTR s_Type, CString s_File)
{
	char* ps8_Data;
	DWORD u32_Size;
	DWORD u32_Err = gi_App.mi_Func.GetResource(s_ID, s_Type, &ps8_Data, &u32_Size);
	if (u32_Err)
		return u32_Err;

	return Save(s_File, ps8_Data, u32_Size);
}

// Writes into a file, returns 0 or API error
DWORD CFileEx::Save(LPCTSTR s_File, const char* s8_Data, DWORD u32_Len, BOOL b_Append)
{
	gi_App.mi_Func.ToggleWow64(TRUE);

	CFileEx i_File;
	DWORD u32_Err = i_File.Open(s_File, b_Append ? E_WriteAppend : E_WriteCreate);

	gi_App.mi_Func.ToggleWow64(FALSE);

	if (u32_Err)
		return u32_Err;

	if (u32_Err = i_File.Write(s8_Data, u32_Len))
		return u32_Err;

	i_File.Close();
	return 0;
}

// Loads the content of the file into the buffer which must be large enough otherwise the file is loaded partially
// returns in pu32_Len the bytes read
DWORD CFileEx::Load(LPCTSTR s_File, char* s8_Data, DWORD u32_Len, DWORD* pu32_Read)
{
	CFileEx i_File;
	DWORD u32_Err = i_File.Open(s_File, E_Read);
	if (u32_Err)
		return u32_Err;

	if (u32_Err = i_File.Read(s8_Data, u32_Len, pu32_Read))
		return u32_Err;

	i_File.Close();
	return 0;
}

// ====================== MEMBER ======================

// Opens an existing file or creates a new file
DWORD CFileEx::Open(LPCTSTR s_Path, eOpen e_Mode)
{
	DWORD u32_CreateFlag;
	DWORD u32_AccessFlag;
	switch (e_Mode)
	{
		case E_Read:        
			u32_CreateFlag = OPEN_EXISTING; 
			u32_AccessFlag = GENERIC_READ;
			break;
		case E_WriteAppend: 
			u32_CreateFlag = OPEN_EXISTING; 
			u32_AccessFlag = GENERIC_WRITE;
			break;
		case E_WriteCreate: 
			u32_CreateFlag = CREATE_ALWAYS; 
			u32_AccessFlag = GENERIC_WRITE;
			break;
	}

	mh_File = ::CreateFile(s_Path, u32_AccessFlag, FILE_SHARE_READ, 0, u32_CreateFlag, FILE_ATTRIBUTE_NORMAL, 0);
	if (mh_File == INVALID_HANDLE_VALUE)
	{
		mh_File = 0;
		return GetLastError();
	}

	if (e_Mode == E_WriteAppend)
	{
		if (SetFilePointer(mh_File, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER)
		{
			DWORD u32_Err = GetLastError();
			Close();
			return u32_Err;
		}
	}
	return 0;
}

DWORD CFileEx::Write(const char* s8_Data, DWORD u32_Len)
{
	if (!s8_Data || !u32_Len)
		return 0; // Noting to do

	DWORD u32_Written;
	if (!::WriteFile(mh_File, s8_Data, u32_Len, &u32_Written, 0))
	{
		DWORD u32_Err = GetLastError();
		Close();
		return u32_Err;
	}
	return 0;
}

// u32_Len must be set to the buffer size. u32_Read returns the bytes read.
DWORD CFileEx::Read(char* s8_Data, DWORD u32_Len, DWORD* pu32_Read)
{
	DWORD u32_Read;
	if (!::ReadFile(mh_File, s8_Data, u32_Len, &u32_Read, 0))
	{
		DWORD u32_Err = GetLastError();
		Close();
		if (pu32_Read) *pu32_Read = 0;
		return u32_Err;
	}
	if (pu32_Read) *pu32_Read = u32_Read;
	return 0;
}

void CFileEx::Close()
{
	if (mh_File)
	{
		CloseHandle(mh_File);
		mh_File = 0;
	}
}

