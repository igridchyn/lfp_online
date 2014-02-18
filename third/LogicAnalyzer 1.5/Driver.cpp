//-----------------------------------------------------------------------------
//  
//   PORTTALK/WINRING0 DRIVER
//   Author: Elmü (www.netcult.ch/elmue)
//
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

extern CLogicAnalyzerApp gi_App;

CDriver::CDriver()
{
	mh_Driver     = INVALID_HANDLE_VALUE;
	mb_PortTalk   = FALSE;
	mb_WinRing0   = FALSE;
	ms_DriverName = "none";
}

CDriver::~CDriver()
{
	CloseHandle(mh_Driver);
}

// returns a INIT_.. constant
DWORD CDriver::Initialize(DWORD* pu32_ApiError)
{
	// On 9x Windows no driver required
	if (gi_App.mi_Func.IsWin9x())
		return INIT_OK;

	if (gi_App.mi_Func.IsWin64Bit())
	{
		// Porttalk does not exist as 64 Bit version -> use the slower WinRin0 driver
		mb_WinRing0   = TRUE;
		ms_DriverName = "WinRing0";
		ms_DriverFile = "WinRing0.sys"; // Filename in C:\Windows\System32\Drivers
		ms_DriverID   = "WinRing0_1_2_0";
	}
	else // 32 Bit Windows -> use Porttalk (faster)
	{
		mb_PortTalk   = TRUE;
		ms_DriverName = "PortTalk";
		ms_DriverFile = "PortTalk.sys"; // Filename in C:\Windows\System32\Drivers
		ms_DriverID   = "PortTalk";
	}

	// OpenDriver() will always return without error here except for the very first time 
	// when the driver has not yet been registered in the Registry
	// For OpenDriver() administrator privileges are !NOT! required!
	*pu32_ApiError = OpenDriver(ms_DriverID);
	if (*pu32_ApiError)
	{
		// "C:\Windows\System32\"
		CString s_SysDir;
		*pu32_ApiError = gi_App.mi_Func.GetSystemDir(&s_SysDir);
		if (*pu32_ApiError)
			return INIT_COPY_ERR;

		CString s_Driver = s_SysDir + "drivers\\" + ms_DriverFile;

		ManageDriver(ms_DriverID, s_Driver, DRIVER_REMOVE);

		if (!CFileEx::IsFile(s_Driver))
		{
			DWORD u32_Res;
			if (mb_PortTalk) u32_Res = IDR_PORTTALK;
			else
			{
				if (gi_App.mi_Func.IsWin64Bit()) u32_Res = IDR_WINRING_64;
				else                             u32_Res = IDR_WINRING_32;
			}

			// Save the driver binary into the System32\Driver directory
			// Only Administrator!
			*pu32_ApiError = CFileEx::WriteResource(MAKEINTRESOURCE(u32_Res), RT_RCDATA, s_Driver);
			if (*pu32_ApiError)
				return INIT_COPY_ERR;
		}

		// DRIVER_INSTALL registers the driver in the Registry:
		// Only Administrator!
		*pu32_ApiError = ManageDriver(ms_DriverID, s_Driver, DRIVER_INSTALL);
		if (*pu32_ApiError) 
		{
			ManageDriver(ms_DriverID, s_Driver, DRIVER_REMOVE);
			return INIT_LOAD_ERR;
		}
		
		*pu32_ApiError = OpenDriver(ms_DriverID);
		if (*pu32_ApiError)
			return INIT_LOAD_ERR;
	}

	if (mb_PortTalk)
	{
		*pu32_ApiError = AllowIO();
		if (*pu32_ApiError)
			return INIT_ALLOWIO_ERR;
	}
	return INIT_OK;
}

CString CDriver::GetDriverName()
{
	if (gi_App.mi_Func.IsWin64Bit())
		return ms_DriverName + " (64 Bit)";
	else
		return ms_DriverName + " (32 Bit)";
}

//-----------------------------------------------------------------------------

// returns API error or 0 on success
DWORD CDriver::ManageDriver(LPCTSTR DriverId, LPCTSTR DriverPath, USHORT Function)
{
	SC_HANDLE h_SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!h_SCManager)
        return GetLastError();

	DWORD u32_Err = 0;
    switch (Function)
	{
	case DRIVER_INSTALL:
		u32_Err = InstallDriver(h_SCManager, DriverId, DriverPath);
		if (!u32_Err)
			 u32_Err = StartDriver(h_SCManager, DriverId);
		break;

	case DRIVER_REMOVE:
		u32_Err = StopDriver(h_SCManager, DriverId);
		if (!u32_Err)
			 u32_Err = RemoveDriver(h_SCManager, DriverId);
		break;

	default:
		u32_Err = ERROR_INVALID_PARAMETER;
		break;
    }

	CloseServiceHandle(h_SCManager);
	return u32_Err;
}

//-----------------------------------------------------------------------------

// returns API error or 0 on success
DWORD CDriver::OpenDriver(LPCTSTR DriverId)
{
	CloseHandle(mh_Driver);

	CString s_Driver = CString("\\\\.\\") + DriverId;

	mh_Driver = CreateFile(s_Driver, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (mh_Driver == INVALID_HANDLE_VALUE)
		return GetLastError();
	
	return 0;
}

//-----------------------------------------------------------------------------

// returns API error or 0 on success
// u32_Err = ERROR_SERVICE_MARKED_FOR_DELETE means that the driver is still loaded and in use
DWORD CDriver::InstallDriver(SC_HANDLE h_SCManager, LPCTSTR DriverId, LPCTSTR DriverPath)
{
	SC_HANDLE h_Service = CreateService(h_SCManager, DriverId, DriverId,
	                      SRVICE_PERMISS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
	                      DriverPath, NULL, NULL, NULL, NULL, NULL);
    if (h_Service)
	{
		CloseServiceHandle(h_Service);
		return 0;
	}
	
	DWORD u32_Err = GetLastError();
	if (u32_Err != ERROR_SERVICE_EXISTS) // 1073
		return u32_Err;

    return 0;
}

//-----------------------------------------------------------------------------

// returns API error or 0 on success
DWORD CDriver::StartDriver(SC_HANDLE h_SCManager, LPCTSTR DriverId)
{
	SC_HANDLE h_Service = OpenService(h_SCManager, DriverId, SRVICE_PERMISS);
    if (!h_Service)
		return GetLastError();

	// StartService may return ERROR_FILE_NOT_FOUND.
	// Is this a bug in the driver ??
	// But this does not matter: OpenDriver() will return a valid handle in spite of that!
	DWORD u32_Err = 0;
	if (!StartService(h_Service, 0, NULL))
		u32_Err = GetLastError();
	
	if (u32_Err == ERROR_SERVICE_ALREADY_RUNNING) // 1056
		u32_Err = 0;

	CloseServiceHandle(h_Service);
	return u32_Err;
}

//-----------------------------------------------------------------------------

// returns API error or 0 on success
DWORD CDriver::StopDriver(SC_HANDLE h_SCManager, LPCTSTR DriverId)
{
	CloseHandle(mh_Driver);
	mh_Driver = INVALID_HANDLE_VALUE;

	SC_HANDLE h_Service = OpenService(h_SCManager, DriverId, SRVICE_PERMISS);
    if (!h_Service)
		return 0; // Driver is not installed -> nothing to stop
	
	DWORD u32_Err = 0;
	SERVICE_STATUS k_Status;
	if (!ControlService(h_Service, SERVICE_CONTROL_STOP, &k_Status))
		u32_Err = GetLastError();

	if (u32_Err == ERROR_SERVICE_NOT_ACTIVE || u32_Err == ERROR_SERVICE_MARKED_FOR_DELETE) // 1062, 1072
		u32_Err = 0;

    CloseServiceHandle(h_Service);
    return u32_Err;
}

//-----------------------------------------------------------------------------

// returns API error or 0 on success
DWORD CDriver::RemoveDriver(SC_HANDLE h_SCManager, LPCTSTR DriverId)
{
	CloseHandle(mh_Driver);
	mh_Driver = INVALID_HANDLE_VALUE;

    SC_HANDLE h_Service = OpenService(h_SCManager, DriverId, SRVICE_PERMISS);
    if (!h_Service)
		return 0;// Driver is not installed -> nothing to remove

	DWORD u32_Err = 0;
	if (!DeleteService(h_Service))
		u32_Err = GetLastError();
		
	CloseServiceHandle(h_Service);
	if (u32_Err == ERROR_SERVICE_MARKED_FOR_DELETE) // 1072
		return 0;

	return u32_Err;
}

// ##############################################################################
// ##############################################################################
// ##############################################################################

// Allows unlimited access to all IO ports for the current process
// This avoids the usage of slow drivers by DIRECTLY using _inp and _outp
DWORD CDriver::AllowIO()
{
	if (mh_Driver == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;


	DWORD u32_RetLen;
	if (!DeviceIoControl(mh_Driver, IOCTL_IOPM_ALLOW_EXCUSIVE_ACCESS, 
	                     NULL, 0, NULL, 0, &u32_RetLen, NULL))
		return GetLastError();

	DWORD u32_PID = GetCurrentProcessId();
	if (!DeviceIoControl(mh_Driver, IOCTL_ENABLE_IOPM_ON_PROCESSID, &u32_PID, sizeof(u32_PID),
	                     NULL, 0, &u32_RetLen, NULL))
		return GetLastError();

	return 0;
}

DWORD CDriver::WriteIoPortByte(WORD u16_Port, BYTE u8_Value)
{
	if (mb_WinRing0)
	{
		// Write a Byte to an I/O Port via Driver
		DWORD u32_RetLen;
		WRITE_IO_PORT_INPUT k_In;
		k_In.CharData   =  u8_Value;
		k_In.PortNumber = u16_Port;

		if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &k_In, sizeof(k_In),
							 NULL, 0, &u32_RetLen, NULL))
			return GetLastError();

		return 0;
	}
	else
	{
		// DIRECTLY write a Byte to an I/O Port
		_outp(u16_Port, u8_Value);
		return 0;
	}
}

DWORD CDriver::ReadIoPortByte(WORD u16_Port, BYTE* pu8_Value)
{
	if (mb_WinRing0)
	{
		// Read a Byte from an I/O Port via Driver
		DWORD u32_RetLen;
		WORD  u16_Value;
		if (!DeviceIoControl(mh_Driver, IOCTL_READ_IO_PORT_BYTE, &u16_Port, sizeof(u16_Port),
							 &u16_Value, sizeof(u16_Value), &u32_RetLen, NULL))
			return GetLastError();

		*pu8_Value = (BYTE)u16_Value;
		return 0;
	}
	else
	{
		// DIRECTLY read a Byte from an I/O Port
		*pu8_Value = _inp(u16_Port);
		return 0;
	}
}
