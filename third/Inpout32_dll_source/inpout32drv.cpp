// inpout32drv.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "hwinterfacedrv.h"
#include "resource.h"
#include "conio.h"
#include "stdlib.h" 

int inst(LPCTSTR pszDriver);
int start(LPCTSTR pszDriver);

//First, lets set the DRIVERNAME depending on our configuraiton.
#define DRIVERNAMEx64 "hwinterfacex64\0"
#define DRIVERNAMEi386 "hwinterface\0"

char str[10];
int vv;

HANDLE hdriver=NULL;
char path[MAX_PATH];
HINSTANCE hmodule;
SECURITY_ATTRIBUTES sa;
int sysver;

int Opendriver(BOOL bX64);
void Closedriver(void);

BOOL APIENTRY DllMain( HINSTANCE  hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{

	hmodule = hModule;
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		sysver = SystemVersion();
		if(sysver==2)
		{
			Opendriver(IsXP64Bit());
		}
		break;
	case DLL_PROCESS_DETACH:
		if(sysver==2)
		{
			Closedriver();
		}
		break;
	}
	return TRUE;
}

/***********************************************************************/

void Closedriver(void)
{
	if (hdriver)
	{
		OutputDebugString("Closing driver...\n");
		CloseHandle(hdriver);
		hdriver=NULL;
	}
}

void _stdcall Out32(short PortAddress, short data)
{

	switch(sysver)
	{
	case 1:
#ifdef _M_IX86
		_outp( PortAddress,data);	//Will ONLY compile on i386 architecture
#endif
		break;
	case 2:
		unsigned int error;
		DWORD BytesReturned;        
		BYTE Buffer[3];
		unsigned short * pBuffer;
		pBuffer = (unsigned short *)&Buffer[0];
		*pBuffer = LOWORD(PortAddress);
		Buffer[2] = LOBYTE(data);

		error = DeviceIoControl(hdriver,
			IOCTL_WRITE_PORT_UCHAR,
			&Buffer,
			3,
			NULL,
			0,
			&BytesReturned,
			NULL);
		break;
	}


}

/*********************************************************************/

short _stdcall Inp32(short PortAddress)
{
	BYTE retval(0);
	switch(sysver)
	{
	case 1:
#ifdef _M_IX86
		retval = _inp(PortAddress);
#endif
		return retval;
		break;
	case 2:
		unsigned int error;
		DWORD BytesReturned;
		unsigned char Buffer[3];
		unsigned short * pBuffer;
		pBuffer = (unsigned short *)&Buffer;
		*pBuffer = LOWORD(PortAddress);
		Buffer[2] = 0;
		error = DeviceIoControl(hdriver,
			IOCTL_READ_PORT_UCHAR,
			&Buffer,
			2,
			&Buffer,
			1,
			&BytesReturned,
			NULL);

		return((int)Buffer[0]);

		break;
	}
	return 0;
}

/*********************************************************************/

int Opendriver(BOOL bX64)
{
	OutputDebugString("Attempting to open InpOut driver...\n");

	char szFileName[MAX_PATH] = {NULL};
	if (bX64)
		strcpy_s(szFileName, MAX_PATH, "\\\\.\\hwinterfacex64");	//We are 64bit...
	else
		strcpy_s(szFileName, MAX_PATH, "\\\\.\\hwinterface");		//We are 32bit...

	hdriver = CreateFile(szFileName, 
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if(hdriver == INVALID_HANDLE_VALUE) 
	{
		if(start(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386))
		{
			inst(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
			start(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);

			hdriver = CreateFile(szFileName, 
				GENERIC_READ | GENERIC_WRITE, 
				0, 
				NULL,
				OPEN_EXISTING, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);

			if(hdriver != INVALID_HANDLE_VALUE) 
			{
				OutputDebugString("Successfully opened ");
				OutputDebugString(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
				OutputDebugString(" driver");
				return 0;
			}
		}
		return 1;
	}
	OutputDebugString("Successfully opened ");
	OutputDebugString(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
	OutputDebugString(" driver");
	return 0;
}

/***********************************************************************/
int inst(LPCTSTR pszDriver)
{
	char szDriverSys[MAX_PATH];
	strcpy_s(szDriverSys, MAX_PATH, pszDriver);
	strcat_s(szDriverSys, MAX_PATH, ".sys\0");
	
	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;
	GetSystemDirectory(path , sizeof(path));
	HRSRC hResource = FindResource(hmodule, MAKEINTRESOURCE(IDR_BIN1), "bin");
	if(hResource)
	{
		HGLOBAL binGlob = LoadResource(hmodule, hResource);

		if(binGlob)
		{
			void *binData = LockResource(binGlob);

			if(binData)
			{
				HANDLE file;
				strcat_s(path, sizeof(path), "\\Drivers\\");
				strcat_s(path, sizeof(path), szDriverSys);
			
				file = CreateFile(path,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					0,
					NULL);

				if(file)
				{
					DWORD size, written;

					size = SizeofResource(hmodule, hResource);
					WriteFile(file, binData, size, &written, NULL);
					CloseHandle(file);

				}
			}
		}
	}

	Mgr = OpenSCManager (NULL, NULL,SC_MANAGER_ALL_ACCESS);
	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED) 
		{
			return 5;  // error access denied
		}
	}	
	else
	{
		char szFullPath[MAX_PATH] = "System32\\Drivers\\\0";
		strcat_s(szFullPath, MAX_PATH, szDriverSys);
		Ser = CreateService (Mgr,                      
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
	}
	CloseServiceHandle(Ser);
	CloseServiceHandle(Mgr);

	return 0;
}
/**************************************************************************/
int start(LPCTSTR pszDriver)
{
	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;

	Mgr = OpenSCManager (NULL, NULL,SC_MANAGER_ALL_ACCESS);

	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED) 
		{
			Mgr = OpenSCManager (NULL, NULL,GENERIC_READ);
			Ser = OpenService(Mgr,pszDriver,GENERIC_EXECUTE);
			if (Ser)
			{    // we have permission to start the service
				if(!StartService(Ser,0,NULL))
				{
					CloseServiceHandle (Ser);
					return 4; // we could open the service but unable to start
				}

			}

		}
	}
	else
	{// Successfuly opened Service Manager with full access
		Ser = OpenService(Mgr,pszDriver,GENERIC_EXECUTE);
		if (Ser)
		{
			if(!StartService(Ser,0,NULL))
			{
				CloseServiceHandle (Ser);
				return 3; // opened the Service handle with full access permission, but unable to start
			}
			else
			{
				CloseServiceHandle (Ser);
				return 0;
			}
		}
	}
	return 1;
}