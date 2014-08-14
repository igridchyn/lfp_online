#include "inpout32.h"

#define IDR_BIN1                        101
#define IOCTL_READ_PORT_UCHAR	 -1673519100 //CTL_CODE(40000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_WRITE_PORT_UCHAR	 -1673519096 //CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_UCHAR	 CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DRIVERNAMEx64 "hwinterfacex64\0"
#define DRIVERNAMEi386 "hwinterface\0"

HINSTANCE hmodule;
char path[MAX_PATH];

void __declspec(dllexport) ReportError(){
	unsigned int error = GetLastError();
	LPTSTR lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	printf("Write error %ld :", error);
	wprintf(L"%s", lpMsgBuf);
}

// was LPCTSTR
// isntall a driver for the LPT port
int inst(LPCSTR pszDriver)
{
	char szDriverSys[MAX_PATH];
	strcpy_s(szDriverSys, MAX_PATH, pszDriver);
	strcat_s(szDriverSys, MAX_PATH, ".sys\0");

	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;
	GetSystemDirectoryA(path, sizeof(path));
	HRSRC hResource = FindResource(hmodule, MAKEINTRESOURCE(IDR_BIN1), "bin");
	if (hResource)
	{
		HGLOBAL binGlob = LoadResource(hmodule, hResource);

		if (binGlob)
		{
			void *binData = LockResource(binGlob);

			if (binData)
			{
				HANDLE file;
				strcat_s(path, sizeof(path), "\\Drivers\\");
				strcat_s(path, sizeof(path), szDriverSys);

				file = CreateFileA(path,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					0,
					NULL);

				if (file)
				{
					DWORD size, written;

					size = SizeofResource(hmodule, hResource);
					WriteFile(file, binData, size, &written, NULL);
					CloseHandle(file);

				}
			}
		}
	}

	Mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			return 5;  // error access denied
		}
	}
	else
	{
		char szFullPath[MAX_PATH] = "C:\\Windows\\System32\\Drivers\\";		
		strcat_s(szFullPath, MAX_PATH, szDriverSys);
		Ser = CreateServiceA(Mgr,
			pszDriver,
			pszDriver,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_SYSTEM_START,
			SERVICE_ERROR_NORMAL,
			szFullPath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
			);

		if (!Ser)
			ReportError();
	}
	CloseServiceHandle(Ser);
	CloseServiceHandle(Mgr);

	return 0;
}

HANDLE hdriver = NULL;
/// !!! was LPCTSTR
// start driver service
int start(LPCSTR pszDriver)
{
	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;

	Mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			Mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
			Ser = OpenServiceA(Mgr, pszDriver, GENERIC_EXECUTE);
			if (Ser)
			{    // we have permission to start the service
				if (!StartService(Ser, 0, NULL))
				{
					CloseServiceHandle(Ser);
					return 4; // we could open the service but unable to start
				}

			}

		}
	}
	else
	{// Successfuly opened Service Manager with full access
		//Ser = OpenServiceA(Mgr, pszDriver, GENERIC_EXECUTE);
		Ser = OpenService(Mgr, "hwinterfacex64", GENERIC_EXECUTE);
		if (Ser)
		{
			//if (!StartServiceA(Ser, 0, NULL))
			if (!StartService(Ser, 0, NULL))
			{
				CloseServiceHandle(Ser);
				ReportError();
				return 3; // opened the Service handle with full access permission, but unable to start
			}
			else
			{
				CloseServiceHandle(Ser);
				return 0;
			}
		}
	}
	return 1;
}

// open driver - after starting service?
int __declspec(dllexport) Opendriver()
{
	BOOL bX64 = true;

	OutputDebugStringW(L"Attempting to open InpOut driver...\n");

	char szFileName[MAX_PATH] = { NULL };
	if (bX64)
		strcpy_s(szFileName, MAX_PATH, "\\\\.\\hwinterfacex64");	//We are 64bit...
	else
		strcpy_s(szFileName, MAX_PATH, "\\\\.\\hwinterface");		//We are 32bit...

	hdriver = CreateFileA(szFileName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hdriver == INVALID_HANDLE_VALUE)
	{
		ReportError();

		if (start(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386))
		{
			inst(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
			start(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);

			hdriver = CreateFileA(szFileName,
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (hdriver != INVALID_HANDLE_VALUE)
			{
				OutputDebugStringA("Successfully opened ");
				OutputDebugStringA(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
				OutputDebugStringA(" driver");
				return 0;
			}
		}
		return 1;
	}
	OutputDebugStringA("Successfully opened ");
	OutputDebugStringA(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
	OutputDebugStringA(" driver");
	return 0;
}

void __declspec(dllexport) Out32(short PortAddress, short data)
{
	unsigned int error;
	DWORD BytesReturned;
	BYTE Buffer[3];
	unsigned short * pBuffer;
	pBuffer = (unsigned short *)&Buffer[0];
	*pBuffer = LOWORD(PortAddress);

	//for (int i = 0; i < 2; ++i){
	Buffer[2] = data;// (i % 2) ? 0xFF : 0x00;

		DeviceIoControl(hdriver,
			IOCTL_WRITE_PORT_UCHAR,
			&Buffer,
			3,
			NULL,
			0,
			&BytesReturned,
			NULL);

		//Sleep(5);
	//}

	if (!DeviceIoControl(hdriver,
		IOCTL_WRITE_PORT_UCHAR,
		&Buffer,
		3,
		NULL,
		0,
		&BytesReturned,
		NULL)){

		error = GetLastError();

		LPTSTR lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		printf("Write error %ld :", error);
		wprintf(L"%s", lpMsgBuf);
	}
}