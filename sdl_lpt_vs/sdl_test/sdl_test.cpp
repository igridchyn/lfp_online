// sdl_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SDL.h"
#include <windows.h>
#include <WinBase.h>
#include <winioctl.h>
#include <conio.h>
#include <winsvc.h>
#include <minwindef.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

#define IDR_BIN1                        101

struct WRITE_IO_PORT_INPUT
{
	ULONG PortNumber;
	UCHAR CharData;
};

char path[MAX_PATH];
HINSTANCE hmodule;

// was LPCTSTR
int inst(LPCSTR pszDriver)
{
	char szDriverSys[MAX_PATH];
	strcpy_s(szDriverSys, MAX_PATH, pszDriver);
	strcat_s(szDriverSys, MAX_PATH, ".sys\0");

	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;
	GetSystemDirectoryA(path, sizeof(path));
	HRSRC hResource = FindResource(hmodule, MAKEINTRESOURCE(IDR_BIN1), L"bin");
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
		char szFullPath[MAX_PATH] = "System32\\Drivers\\\0";
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
	}
	CloseServiceHandle(Ser);
	CloseServiceHandle(Mgr);

	return 0;
}

HANDLE hdriver = NULL;
/// !!! was LPCTSTR
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
		Ser = OpenService(Mgr, L"hwinterfacex64", GENERIC_EXECUTE);
		if (Ser)
		{
			//if (!StartServiceA(Ser, 0, NULL))
			if (!StartService(Ser, 0, NULL))
			{
				CloseServiceHandle(Ser);
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

#define DRIVERNAMEx64 "hwinterfacex64\0"
#define DRIVERNAMEi386 "hwinterface\0"
int Opendriver(BOOL bX64)
{
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

#define IOCTL_READ_PORT_UCHAR	 -1673519100 //CTL_CODE(40000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_UCHAR	 -1673519096 //CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
void _stdcall Out32(short PortAddress, short data)
{
		unsigned int error;
		DWORD BytesReturned;
		BYTE Buffer[3];
		unsigned short * pBuffer;
		pBuffer = (unsigned short *)&Buffer[0];
		*pBuffer = LOWORD(PortAddress);
		Buffer[2] = LOBYTE(data);

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

void write_ltp(){
	DWORD error;
	LPTSTR lpMsgBuf;

	HANDLE mh_Driver = CreateFileA("\\\\.\\LPT1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (mh_Driver == INVALID_HANDLE_VALUE){
		error = GetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		wprintf(L"Create File Error: %d - %s\n", error, lpMsgBuf);
	}

	DWORD u32_RetLen;
	WRITE_IO_PORT_INPUT k_In;
	k_In.CharData = 0x01;
	k_In.PortNumber = 0x01;

	 //DWORD IOCTL_WRITE_IO_PORT_BYTE = CTL_CODE(40000, 0x836, METHOD_BUFFERED, FILE_WRITE_ACCESS);
	// DWORD IOCTL_WRITE_IO_PORT_BYTE = CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x836, METHOD_BUFFERED, FILE_WRITE_ACCESS);
	// DWORD IOCTL_WRITE_IO_PORT_BYTE = 0x002C0004;
	// DWORD IOCTL_WRITE_IO_PORT_BYTE = 0x222010;
	 // from inpout32drv

	// #define IOCTL_WRITE_PORT_UCHAR	 -1673519096 //CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
	 DWORD IOCTL_WRITE_IO_PORT_BYTE = -1673519096;

	for (int i = 0; i < 2; ++i){
		// unsigned long

		unsigned char val = 0;
		//BOOL res = WriteFile(mh_Driver, &buf, strlen(buf), &u32_RetLen, NULL);//write to LPT
		//BOOL res = WriteFile(mh_Driver, &val, 1, &u32_RetLen, NULL);//write to LPT
		//if (!res){
		//	printf("Write File error");
		//}

		unsigned char byte_out = 0;

		//from inpout32drv
		short PortAddress = 0x0378;
		short data = 0xFFFF;
		DWORD BytesReturned;
		BYTE Buffer[3];
		unsigned short * pBuffer;
		pBuffer = (unsigned short *)&Buffer[0];
		*pBuffer = LOWORD(PortAddress);
		Buffer[2] = LOBYTE(data);
		//if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &Buffer, 3, NULL, 0, &BytesReturned, NULL)){
		if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &Buffer, 3, &byte_out, 1, &BytesReturned, NULL)){


		//if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &val, sizeof(&val), &byte_out, 1, &u32_RetLen, NULL)){
	     //if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &k_In, sizeof(k_In), NULL, 0, &u32_RetLen, NULL)){
		//if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &k_In, sizeof(k_In), &byte_out, 1, &u32_RetLen, NULL)){
			error = GetLastError();

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

		Sleep(200);
	}
}

void putPixel(SDL_Renderer *renderer, int x, int y)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawPoint(renderer, x, y);
}

void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2){
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

typedef short t_bin;

void draw_bin(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture, const char *path){
	FILE *f = fopen(path, "rb");

	const int CHUNK_SIZE = 432; // bytes
	int CHANNEL = 1;
	const int HEADER_LEN = 32; // bytes
	const int BLOCK_SIZE = 64 * 2; // bytes

	unsigned char block[CHUNK_SIZE];
	const int CH_MAP[] = { 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 24, 25, 26, 27, 28, 29, 30, 31 };

	t_bin val_prev = 1;
	int x_prev = 1;
	int plot_hor_scale = 10;
	int plot_scale = 40;

	// read first some data without signal in the beginning
	//for (int i = 0; i < 400000; ++i){
	//	fread((void*)block, CHUNK_SIZE, 1, f);
	//}

	for (int i = 0; i < 40000; ++i){
		fread((void*)block, CHUNK_SIZE, 1, f);

		// iterate throug 3 batches in 1 chunk
		for (int batch = 0; batch < 3; ++batch){
			t_bin *ch_dat = (t_bin*)(block + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[CHANNEL]);
			t_bin val = *ch_dat;
			printf("%d \n", val);

			// scale for plotting
			const int SHIFT = 11000;
			val = val + SHIFT;
			val = val > 0 ? val / plot_scale : 1;
			val = val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;

			int sample_number = i * 3 + batch;
			if (sample_number % plot_hor_scale == 0){
				SDL_SetRenderTarget(renderer, texture);
				drawLine(renderer, x_prev, val_prev, x_prev + 1, val);

				SDL_SetRenderTarget(renderer, NULL);
				SDL_RenderCopy(renderer, texture, NULL, NULL);
				SDL_RenderPresent(renderer);
					
				val_prev = val;
				x_prev++;
				if (x_prev == SCREEN_WIDTH - 1){
					x_prev = 1;

					// reset screen
					SDL_SetRenderTarget(renderer, texture);
					SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
					SDL_RenderClear(renderer);
					SDL_RenderPresent(renderer);
				}
			}

			SDL_Event e;
			bool quit = false;
			while (SDL_PollEvent(&e) != 0)
			{
				//User requests quit
				if (e.type == SDL_QUIT)
				{
					quit = true;
				}
				//User presses a key
				else if (e.type == SDL_KEYDOWN)
				{
					//Select surfaces based on key press
					switch (e.key.keysym.sym)
					{
					case SDLK_UP:
						plot_scale += 5;
						break;
					case SDLK_DOWN:
						plot_scale = plot_scale > 5 ? plot_scale - 5 : 5;
						break;

					case SDLK_RIGHT:
						plot_hor_scale += 2;
						break;
					case SDLK_LEFT:
						plot_hor_scale = plot_hor_scale > 2 ? plot_hor_scale - 2 : 2;
						break;

					case SDLK_ESCAPE:
						return;
						break;
					case SDLK_1:
						CHANNEL = 1;
						break;
					case SDLK_2:
						CHANNEL = 5;
						break;
					case SDLK_3:
						CHANNEL = 9;
						break;
					case SDLK_4:
						CHANNEL = 13;
						break;
					case SDLK_5:
						CHANNEL = 17;
						break;
					}
				}
			}
		}
	}
}

void draw_test(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture){
	for (int x = 0; x<10; x++)
	{
		SDL_SetRenderTarget(renderer, texture);

		for (int y = 0; y<10; y++)
		{
			putPixel(renderer, 40 + x * 10, 50 + y);
		}

		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		SDL_Delay(100);
	}
}

int get_image(){
	SDL_Window *window;
	SDL_Texture *texture;
	SDL_Renderer *renderer;

	window = SDL_CreateWindow("SDL2 Test", 50, 50, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);// SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

	//void *d3dDLL = NULL;
	//d3dDLL = SDL_LoadObject("D3D9.DLL");
	
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	const char *error = SDL_GetError();

	// draw_test(window, renderer, texture);
	draw_bin(window, renderer, texture, "D:/data/hai_bing/jc86-2612/jc86-2612_21sleeping30mins.bin");
	SDL_Delay(2000);

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	Opendriver(true);
	Out32(0x0378, 0xFFFF);

	//write_ltp();
	//get_image();

	return 0;
}

