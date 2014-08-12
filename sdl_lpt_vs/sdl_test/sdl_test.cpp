// sdl_test.cpp : Defines the entry point for the console application.
//

#include "inpout32.h"
#include "SDL.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

struct WRITE_IO_PORT_INPUT
{
	ULONG PortNumber;
	UCHAR CharData;
};

#define SRVICE_PERMISS (SC_MANAGER_CREATE_SERVICE | SERVICE_START | SERVICE_STOP | DELETE | SERVICE_CHANGE_CONFIG)
int write_ltp_logic_analyzer(){
	LPTSTR s_Driver = L"\\\\.\\WinRing0_1_2_0";

	// ? register service first ?
	SC_HANDLE h_SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!h_SCManager){
		printf("Cannot open SC manager!\n");
		return 3;
	}

	bool regservice = false;
	// run only once to register service
	if (regservice){
		LPTSTR DriverId = L"WinRing0_1_2_0";
		SC_HANDLE h_Service = CreateService(h_SCManager, DriverId, DriverId,
			SRVICE_PERMISS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			L"C:\\Windows\\System32\\drivers\\WinRing0.sys", NULL, NULL, NULL, NULL, NULL);
		if (!h_Service)
		{
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

			printf("Cannot register service: ");
			wprintf(L"%s\n", lpMsgBuf);
			return 4;
		}
		else{
			CloseServiceHandle(h_Service);
		}
	}

	HANDLE mh_Driver = CreateFile(s_Driver, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (mh_Driver == INVALID_HANDLE_VALUE){
		printf("Driver open error !\n");
		return 1;
	}

	DWORD u32_RetLen;
	WRITE_IO_PORT_INPUT k_In;
	// 0xFF: all high
	k_In.CharData = 0x00;
	// base port 1 address: 0x0378
	k_In.PortNumber = 0x0378;

	DWORD IOCTL_WRITE_IO_PORT_BYTE = CTL_CODE(40000, 0x836, METHOD_BUFFERED, FILE_WRITE_ACCESS);

	for (int i = 0; i < 3000000; ++i){
		if (!DeviceIoControl(mh_Driver, IOCTL_WRITE_IO_PORT_BYTE, &k_In, sizeof(k_In),
			NULL, 0, &u32_RetLen, NULL)){
			printf("Writing error!\n");
			return 2;
		}
	}

	return 0;
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

	for (int i = 0; i < 200000; ++i){
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

		Sleep(20);
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
	int CHANNEL = 2;
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

		//write_ltp();

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
	// InpOut32 - way
	Opendriver();
	
	//To place a TTL pulse use the command
	//>> calllib('inpout32', 'Out32', 888, 255)
	// 888 = x0378, 255 = 0xFF

	Out32(0x0378, 0xFF);

	//write_ltp_logic_analyzer();

	//write_ltp();
	get_image();

	return 0;
}

