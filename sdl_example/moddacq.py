import os
import string


# the following functions generates the bypass code in ANSI C

def proxy_Includes():
  C = """
      #include <windows.h>
	  
	  #include "Config.h"
	  #include "LFPBuffer.h"
	  #include "LFPPipeline.h"
	 
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
	  unsigned char ttl_status = 0xFF;
	  int last_peak = 0;
	 
	  int *signalh = NULL;

	  // new
	  LFPPipeline *pipeline = NULL;
	  LFPBuffer *buf = NULL;
	  Config *config = NULL;
	  unsigned char *block = NULL;
	  FILE *f = NULL;
	  int CHUNK_SIZE;
	  
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
		  
          if(wcslen(filename) > 4) {
		  
		      LPCWSTR fileext;
			  LPCWSTR ebin = L".BIN";
              fileext = filename + wcslen(filename) - 4;
			  
              if ( !wcscmp(fileext, ebin) || !wcscmp(fileext, L".bin")){
				 check_bin++;
				 MessageBoxW(0, filename, L"Creating BIN W!", 0);
				 if (check_bin > 2){
					if(pipeline){
						delete pipeline;
						pipeline = nullptr;
						delete buf;
						buf = nullptr;
						delete config;
						config = nullptr;
					}
				 
					config = new Config("d:/Igor/soft/lfp_online/sdl_example/Res/online.conf");
					buf = new LFPBuffer(config);
					pipeline = new LFPPipeline(buf);
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
			int nsamples = nNumberOfBytesToWrite / 432;
			
			// buf->log_stream << "write " << nsamples << " packages ...\\n";
			
			if (nsamples > 0){
#ifdef PIPELINE_THREAD
				{
					std::lock_guard<std::mutex> lk(pipeline->mtx_data_add_);
					buf->add_data((unsigned char*)lpBuffer, nNumberOfBytesToWrite);
					pipeline->data_added_ = true;
				}
				pipeline->cv_data_added_.notify_one();
#else
			
				buf->add_data((unsigned char*)lpBuffer, nNumberOfBytesToWrite);
				pipeline->process();
#endif
			}
			retval = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

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
# os.system('cl -O2 /w -LD Mod__k32.c inpout32.cpp Mod__k32.def /link /DYNAMICBASE "SDL2.lib"')
# OPTIONS:
# /D "UNICODE" - use unicode calls ...W
os.system('cl -O2 /w -LD  /D "UNICODE" /Gd Mod__k32.cpp Mod__k32.def /link /INCREMENTAL /DYNAMICBASE "lfponlinevs.lib" /DYNAMICBASE "SDL2.lib" /DYNAMICBASE inpout.lib /DYNAMICBASE Advapi32.lib /EXPORT:Opendriver /OPT:NOREF')

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