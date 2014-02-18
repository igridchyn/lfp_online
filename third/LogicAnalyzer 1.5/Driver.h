
#pragma once

#include "Functions.h"
#include "WinIoCtl.h"
#include "Conio.h"

// Porttalk
#define IOCTL_READ_PORT_UCHAR            CTL_CODE(40000, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_UCHAR           CTL_CODE(40000, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IOPM_ALLOW_EXCUSIVE_ACCESS CTL_CODE(40000, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_IOPM_ON_PROCESSID   CTL_CODE(40000, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)

// WinRing0
#define IOCTL_READ_IO_PORT_BYTE          CTL_CODE(40000, 0x833, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRITE_IO_PORT_BYTE         CTL_CODE(40000, 0x836, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define DRIVER_INSTALL 1
#define DRIVER_REMOVE  2

#define INIT_OK          0
#define INIT_COPY_ERR    1
#define INIT_LOAD_ERR    2
#define INIT_ALLOWIO_ERR 3

#define SRVICE_PERMISS (SC_MANAGER_CREATE_SERVICE | SERVICE_START | SERVICE_STOP | DELETE | SERVICE_CHANGE_CONFIG)

class CDriver
{
public:
	CDriver();
	virtual ~CDriver();
	DWORD Initialize(DWORD* pu32_ApiError);
	CString GetDriverName();
	DWORD ManageDriver(LPCTSTR DriverId, LPCTSTR DriverPath, USHORT Function);
	DWORD AllowIO();
	DWORD WriteIoPortByte(WORD u16_Port, BYTE u8_Value);
	DWORD ReadIoPortByte(WORD u16_Port, BYTE* pu8_Value);

private:
	DWORD OpenDriver(LPCTSTR DriverId);
	DWORD InstallDriver(SC_HANDLE hSCManager, LPCTSTR DriverId, LPCTSTR DriverPath);
	DWORD RemoveDriver (SC_HANDLE hSCManager, LPCTSTR DriverId);
	DWORD StartDriver  (SC_HANDLE hSCManager, LPCTSTR DriverId);
	DWORD StopDriver   (SC_HANDLE hSCManager, LPCTSTR DriverId);

	// for WinRing0:
	struct WRITE_IO_PORT_INPUT
	{
		ULONG PortNumber; 
		UCHAR CharData;
	};

	HANDLE  mh_Driver;
	BOOL    mb_PortTalk;
	BOOL    mb_WinRing0;
	CString ms_DriverName;
	CString ms_DriverFile;
	CString ms_DriverID;
};


