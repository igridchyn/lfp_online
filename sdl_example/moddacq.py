import os
import string


# the following functions generates the bypass code in ANSI C

def proxy_Includes():
  C = """
      #include <windows.h>
	  #include "SDL.h"
	 
#include "conio.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "inpout32.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
	  
      #pragma comment(lib, "kernel32")
      #pragma comment(lib, "user32")
  """
  return C

def proxy_Globals():
  C = """
      HANDLE h_BIN_FILE = (HANDLE) NULL;
             /* YOU CAN INJECT ARBITRARY CODE INTO dacqUSB inside DllMain */
	  int check_bin = 0;
	  const int SCREEN_WIDTH = 800;
	  const int SCREEN_HEIGHT = 600;
	  const int SHIFT = 11000;
	  const int plot_scale = 40;
	  const int SIG_BUF_LEN = 8192;
	  // in packets number 
	  const int PEAK_COOLDOWN = 100;
	  
	  unsigned char ttl_status = 0xFF;
	  int last_peak = 0;
	  
	  FILE *out = NULL;
	  
	  int pkg_id = 0;
	  
	  SDL_Window *window = NULL;	
	  SDL_Texture *texture = NULL;
	  SDL_Renderer *renderer = NULL;
	  
	  const int CHUNK_SIZE = 432; // bytes
	  int CHANNEL = 1;
	  const int HEADER_LEN = 32; // bytes
	  const int BLOCK_SIZE = 64 * 2; // bytes
	  
	  int val_prev = 1;
	  int x_prev = 1;
	 
	  int *signalh = NULL;
	  int signal_pos = 0;

      BOOL APIENTRY DllMain( HANDLE hModule, DWORD reason, LPVOID
lpReserved)
      {
          switch (reason)
          {
       case DLL_PROCESS_ATTACH:
				  
				  //MessageBoxA(0, "DacqUSB has been hacked!!!", "Oops!", 0);
                  break;
       case DLL_THREAD_ATTACH:
       case DLL_THREAD_DETACH:
       case DLL_PROCESS_DETACH:
           break;
          }
          return TRUE;
      }
	  
int scaleToScreen(int val){
val = val + SHIFT;
val = val > 0 ? val / plot_scale : 1;
val = val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;

return val;
}

void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int peak){
	SDL_SetRenderDrawColor(renderer, 255,  peak ? 0 : 255,  peak ? 0 : 255, 255);
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}
		
#define IDR_BIN1                        101
#define IOCTL_READ_PORT_UCHAR	 -1673519100 //CTL_CODE(40000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_WRITE_PORT_UCHAR	 -1673519096 //CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_UCHAR	 CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DRIVERNAMEx64 "hwinterfacex64\\0"
#define DRIVERNAMEi386 "hwinterface\\0"
		
HANDLE hdriver = NULL;
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
				strcat_s(path, sizeof(path), "\\\\Drivers\\\\");
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
		char szFullPath[MAX_PATH] = "C:\\\\Windows\\\\System32\\\\Drivers\\\\";
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
		Ser = OpenServiceA(Mgr, "hwinterfacex64", GENERIC_EXECUTE);
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
	
		
		int __declspec(dllexport) Opendriver()
{
	BOOL bX64 = true;

	OutputDebugStringW(L"Attempting to open InpOut driver...\\n");

	char szFileName[MAX_PATH] = { NULL };
	if (bX64)
		strcpy_s(szFileName, MAX_PATH, "\\\\\\\\.\\\\hwinterfacex64");	//We are 64bit...
	else
		strcpy_s(szFileName, MAX_PATH, "\\\\\\\\.\\\\hwinterface");		//We are 32bit...

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
	Buffer[2] = data;

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
  """
  return C

def proxy_CreateFileA():
  C = """
      HANDLE WINAPI proxy_CreateFileA( LPCSTR filename, DWORD access,
DWORD sharing,            LPSECURITY_ATTRIBUTES sa, DWORD creation,
DWORD attributes, HANDLE templ )
      {
          const char *tmpc;
          HANDLE tmph;
          BOOL openbin = FALSE;
          /* is dacq opening a .BIN file for writing? */
		  MessageBoxA(0, "Creating File!!!", "Oops!", 0);
          if( (access & GENERIC_WRITE) && (strlen(filename) > 4)) {
              tmpc = filename + strlen(filename) - 4;
              if ( !strcmp(tmpc,".BIN") || !strcmp(tmpc,".bin") ) openbin = TRUE;
          }
          /* open the file */
          tmph = CreateFileA(filename, access, sharing, sa, creation,
attributes, templ);
          /* did just we open a .BIN file? */
          if (openbin && (tmph != (HANDLE) NULL)) {
              h_BIN_FILE = tmph;
                             /* ADD YOUR CODE HERE:                 *
ADDITIONAL INTIALISATION IF WE ARE OPNING A
BIN FILE E.G. SET UP A  TCP STREAM          
     */
          }
          return tmph;
      }
  """       
  return C

def proxy_CreateFileW():
  C = """
      HANDLE WINAPI proxy_CreateFileW( LPCWSTR filename, DWORD access,
DWORD sharing,            LPSECURITY_ATTRIBUTES sa, DWORD creation,
DWORD attributes, HANDLE templ )
      {
          HANDLE tmph;
          BOOL openbin = FALSE;
          /* is dacq opening a .BIN file for writing? */
		  //MessageBoxW(0, filename, L"Creating file W!", 0);
		  
          if(wcslen(filename) > 4) {	
		      LPCWSTR fileext;
			  LPCWSTR ebin = L".BIN";
              fileext = filename + wcslen(filename) - 4;
			  
              if ( !wcscmp(fileext, ebin) || !wcscmp(fileext, L".bin")){
				 check_bin++;
				 //MessageBoxW(0, fileext, L"Creating BIN W!", 0);
				 if (check_bin > 2 && !window){
				 	window = SDL_CreateWindow("SDL2 Test", 50, 50, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
					renderer = SDL_CreateRenderer(window, -1, 0); // SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
					texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
	  
					SDL_SetRenderTarget(renderer, texture);
					SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
					SDL_RenderClear(renderer);
					
					// ???
					SDL_RenderPresent(renderer);
					
					//out = fopen(L"C:\\Users\\data\\AppData\\Local\\Programs\\Axona\\DacqUSB\\ax_out.txt", "w");
					out = fopen("ax_out.txt", "w");
					Opendriver();
					
					
					// read config aand set parameters. otf == On-the-fly
					std::ifstream file( "C:/Users/data/igor/code/sdl_example/sdl_example/otf.cfg" );
					std::stringstream buffer;
					buffer << file.rdbuf();
					file.close();
					
					std::string line;
					while( std::getline(buffer, line) )
					{
					  std::istringstream is_line(line);
					  std::string key;
					  if( std::getline(is_line, key, '=') )
					  {
						std::string value;
						if( std::getline(is_line, value) ) 
						  // store_line(key, value);
						  //MessageBoxA(0, key.c_str(), "Param!", 0);
						  // TODO: process config values here
					  }
					}
					
				}
				openbin = TRUE;
			  }
          }
          /* open the file */		
          tmph = CreateFileW(filename, access, sharing, sa, creation,
attributes, templ);
          /* did just we open a .BIN file? */
          if (openbin && (tmph != (HANDLE) NULL)) {
              h_BIN_FILE = tmph;
                             /* ADD YOUR CODE HERE:                 *
ADDITIONAL INTIALISATION IF WE ARE OPNING A
BIN FILE E.G. SET UP A  TCP STREAM          
     */
          }
          return tmph;
      }
  """       
  return C
  
def proxy_WriteFile():
  C = """
      BOOL WINAPI proxy_WriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD
nNumberOfBytesToWrite,
          LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped )
      {
          BOOL retval;
		  
          /* are dacq writng to a .BIN file? */
          if ((h_BIN_FILE != (HANDLE) NULL) && (hFile == h_BIN_FILE)) {
						if (!signalh)
							signalh = (int*)malloc(SIG_BUF_LEN * sizeof(int));
						
						int nsamples = nNumberOfBytesToWrite / 432 * 3;
												// display the current value
						int channel = 0;
						int batch = 0;
						const int CH_MAP[] = { 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 24, 25, 26, 27, 28, 29, 30, 31 };
						short *ch_dat = (short*)((unsigned char *)lpBuffer + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[channel]);
						int val = *ch_dat;
						int first_pkg_id = *((int*) lpBuffer + 1); 

						// extract all data points from VALID packages
						for(int pack = 0; pack < nNumberOfBytesToWrite/432; ++pack){
							// check if the package is valid - ok except for the lost packages
							int npkg_id = *((int*) lpBuffer + pack * 432/4 + 1);

							//VALIDATE: TTL pulse every second
							//if (npkg_id % 8000 == 0){
								//ttl_status = 0xFF - ttl_status;
								//Out32(0x0378, ttl_status);
							//}
							
							for(batch = 0; batch < 3; ++batch){
								ch_dat = (short*)((unsigned char *)lpBuffer + pack*432 + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[channel]);
								signalh[signal_pos] = (int)(*ch_dat);
								signal_pos = (signal_pos + 1) % SIG_BUF_LEN;
							}
						}

						// detect peak or trough of the sine wave
						const int filter_width = 21;
						// [20; 50] for sum abs diff
						// 7 for less counter
						const int thold = 16; //20 * filter_width;
						int sum = 0;
						// TODO: running difference
						for (int shift = 1; shift < filter_width/2; ++shift){
							//sum += abs(signalh[(signal_pos + shift - filter_width/2) % 1024] - signalh[(signal_pos - shift - filter_width/2) % 1024]);
							sum += signalh[(signal_pos + shift - filter_width/2) % SIG_BUF_LEN] < signalh[(signal_pos - filter_width/2) % SIG_BUF_LEN];
							sum += signalh[(signal_pos - shift - filter_width/2) % SIG_BUF_LEN] < signalh[(signal_pos - filter_width/2) % SIG_BUF_LEN];
						}
						int peak = sum >= thold;
						
						// VALIDATE decoding: compare periodic
						// val = signalh[( signal_pos - 1 ) % SIG_BUF_LEN] - signalh[(signal_pos - 241 ) % SIG_BUF_LEN];
						
						val = scaleToScreen(val);

						SDL_SetRenderTarget(renderer, texture);
						if (x_prev > 1){
							// DISPLAY all points / TTL on the peak
							for (int i=0; i<nsamples && x_prev < SCREEN_WIDTH; ++i, ++x_prev){
								peak = (signalh[(signal_pos - nsamples + i) % SIG_BUF_LEN] > 0) && (signalh[(signal_pos - 10 - nsamples + i) % SIG_BUF_LEN] < -1000);
								
								// if peak and cooldown has passed, send TTL
								if (peak && (first_pkg_id + i) - last_peak >= 50){
									Out32(0x0378, 0xFF);
									last_peak = first_pkg_id + i;
								}else if (first_pkg_id - last_peak < 100){
									Out32(0x0378, 0x00);
								}
								
								val = scaleToScreen(signalh[(signal_pos - nsamples + i) % SIG_BUF_LEN]);
								//drawLine(renderer, x_prev, val_prev, x_prev + 1, val, peak);
								val_prev = val;
							}

							//SDL_SetRenderTarget(renderer, NULL);
							//SDL_RenderCopy(renderer, texture, NULL, NULL);
							//SDL_RenderPresent(renderer);

							// not to hang up
							SDL_PumpEvents();
						}
						else{
							// reset screen
							SDL_SetRenderTarget(renderer, texture);
							SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
							SDL_RenderClear(renderer);
							SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
							SDL_RenderDrawLine(renderer, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
							SDL_RenderPresent(renderer);
						}
						
						// update variables
						x_prev = (x_prev + 1) % SCREEN_WIDTH;
						val_prev = val;

					/* ADD YOUR CODE HERE: REROUTE THE USB PACKET */           
                                   /* for now we just
write the packet to the .BIN file, just like dacq,
                 but this may be changed to whatever we want */
              //retval = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
			  retval = nNumberOfBytesToWrite;
                         }  else /* not USB packet, just proxy */{
								retval = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
							}
          return retval;
      }
  """     
  return C

def proxy_CloseHandle():
  C = """
      BOOL WINAPI proxy_CloseHandle( HANDLE handle)
      {
          BOOL retval = CloseHandle(handle);
          /* did we close the BIN file? */
          if ((h_BIN_FILE != (HANDLE) NULL) && (handle == h_BIN_FILE) &&
retval) {

               /* ADD YOUR CODE HERE:                  * ADDITIONAL
CLEAN-UP IF DACQ IS CLOSING A BIN-FILE
                */

          }
          return retval;
      }
  """     
  return C



#go through the dump from DUMPBIN and extract any reference to kernel32.dll
def process_import_dump(f):
  retval = list()
  f = open(f)
  strlist = f.readlines()
  f.close()
  while len(strlist) > 0:
      sp = strlist[0].split()
      if len(sp) == 1:
          if sp[0] == "kernel32.dll":
              strlist.pop(0)
              strlist.pop(0)
              strlist.pop(0)
              strlist.pop(0)
              strlist.pop(0)
              strlist.pop(0)
              while (strlist[0].split()):
                  entry = strlist[0].split()[1];                         
                  retval.append(entry)
                  strlist.pop(0)
      strlist.pop(0)   
  #print retval	  
  return retval


# ================================================================================================================================================================
# use dumpbin to find the binary imports in dacqUSB.exe and DacqUSB.dll
os.system(r"dumpbin /imports dacqUSB.exe > dacqusb_imports.txt")
os.system(r"dumpbin /imports DacqUSB.dll > dacqusbdll_imports.txt")

# get the imports from kernel32.dll
k32imports = list()
k32imports.extend( process_import_dump("dacqusb_imports.txt") )
k32imports.extend( process_import_dump("dacqusbdll_imports.txt") )

k32imports.extend(['CreateFileA', 'GetLogicalProcessorInformation', 'IsWow64Process', 'GetFinalPathNameByHandleW', 'LocaleNameToLCID', 'GetNativeSystemInfo'])

k32imports_entries = set(k32imports)
k32imports = sorted([s for s in k32imports_entries])

#k32imports.remove('time')
k32imports.remove('Unload')

# print k32imports

# write the def-file for the api hijacking DLL
f = open("Mod__k32.def","wt")
f.write("LIBRARY\n")
f.write("EXPORTS\n")
overridden = ["CreateFileA", "WriteFile", "CloseHandle", "CreateFileW"]
for s in k32imports:
  if not s in overridden:        f.write("\t%s=kernel32.%s\n" % (s,s))
  else:
      f.write("\t%s=proxy_%s\n" % (s,s))
      msg = 'override ' + s
      print msg
f.close()

# create the source code file
f = open("Mod__k32.cpp","wt")
f.write( proxy_Includes() )    #include and linker pragmas
f.write( proxy_Globals() )     #global variable(s) needed for the hijack
f.write( proxy_CreateFileA() ) #hijack of CreateFileA
f.write( proxy_CreateFileW() ) #hijack of CreateFileA
# !!! was mistake: f.write ( starting in the comment
f.write( proxy_WriteFile() )   #hijack of WriteFile
f.write( proxy_CloseHandle() ) #hijack of CloseHandle
f.write("\n\n")
f.close()

# compile the overriding DLL with Microsoft Visual C++
# os.system('cl -O2 -W3 -LD Mod__k32.c inpout32.cpp Mod__k32.def /link /DYNAMICBASE "SDL2.lib"')
# OPTIONS:
# /D "UNICODE" - use unicode calls ...W
os.system('cl -O2 -W3 -LD  /D "UNICODE" /Gd Mod__k32.cpp Mod__k32.def /link /INCREMENTAL /DYNAMICBASE "SDL2.lib" /DYNAMICBASE inpout.lib /DYNAMICBASE Advapi32.lib /EXPORT:Opendriver /OPT:NOREF')

# clean up os.system("del Mod__k32.def");
os.system("del Mod__k32.obj");
os.system("del Mod__k32.lib");
os.system("del Mod__k32.exp");
#os.system("del Mod__k32.c");
os.system("del dacqusbdll_imports.txt");
os.system("del dacqusb_imports.txt");

# !!! - this writing first (to be clear what is Mod_USB in the next block)
# write the new Mod_USB.dll file
# !!! error: DacqUSB.dll instead of Mod_USB.dll
# f = open("Mod_USB.dll","rb")

f = open("DacqUSB.dll","rb")
bytecode = f.read()
f.close()
bytecode = string.replace(bytecode,"kernel32.dll","Mod__k32.dll");
f = open("Mod_USB.dll","wb")
f.write(bytecode) # I hope this doesn't violate Axona's copyright !!!
f.close()

# write the new mod_DacqUSB.exe file
f = open("dacqUSB.exe","rb")
bytecode = f.read()
f.close()
bytecode = string.replace(bytecode,"kernel32.dll","Mod__k32.dll");
bytecode = string.replace(bytecode,"DacqUSB.dll","Mod_USB.dll");
f = open("mod_dacqUSB.exe","wb")
f.write(bytecode) # I hope this doesn't violate Axona's copyright !!!
f.close()

# Now we have Mod_dacqUSB.exe and Mod_USB.dll, and are ready to run dacq with
# modified functionality!!!