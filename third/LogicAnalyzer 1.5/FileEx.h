
#pragma once

class CFileEx
{
	#ifndef INVALID_FILE_ATTRIBUTES
		#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
	#endif
	#ifndef INVALID_SET_FILE_POINTER
		#define INVALID_SET_FILE_POINTER ((DWORD)-1)
	#endif

public:
	enum eOpen
	{
		E_Read,         // Open an existing file for reading
		E_WriteAppend,  // Open an existing file for writing and append data to the end
		E_WriteCreate,  // Open a new or existing file, reset its current content
	};

	CFileEx();
	virtual ~CFileEx();
	DWORD   Open(LPCTSTR s_Path, eOpen e_Mode);
	DWORD   Read (      char* s8_Data, DWORD u32_Len, DWORD* pu32_Read=0);
	DWORD   Write(const char* s8_Data, DWORD u32_Len);
	void    Close();

	static DWORD  Save(LPCTSTR s_File, const char* s8_Data, DWORD u32_Len, BOOL b_Append=FALSE);
	static DWORD  Load(LPCTSTR s_File, char* s8_Data, DWORD u32_Len, DWORD* pu32_Read=0);
	static BOOL   IsFile(LPCTSTR s_File);
	static DWORD  WriteResource(LPCTSTR s_ID, LPCTSTR s_Type, CString s_File);

private:
	HANDLE  mh_File;
};


