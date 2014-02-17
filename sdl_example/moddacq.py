import os
import string


# the following functions generates the bypass code in ANSI C

def proxy_Includes():
  C = """
      #include <windows.h>
	  #include "SDL.h"
	  
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
	  SDL_Window *window = NULL;	
	  SDL_Texture *texture = NULL;
	  SDL_Renderer *renderer = NULL;
	  
	  const int CHUNK_SIZE = 432; // bytes
	  int CHANNEL = 1;
	  const int HEADER_LEN = 32; // bytes
	  const int BLOCK_SIZE = 64 * 2; // bytes
	  
	  short val_prev = 1;
	  int x_prev = 1;

      BOOL APIENTRY DllMain( HANDLE hModule, DWORD reason, LPVOID
lpReserved)
      {
	  

	  
          switch (reason)
          {
       case DLL_PROCESS_ATTACH:
				  
				  MessageBoxA(0, "DacqUSB has been hacked!!!", "Oops!", 0);
                  break;
       case DLL_THREAD_ATTACH:
       case DLL_THREAD_DETACH:
       case DLL_PROCESS_DETACH:
           break;
          }
          return TRUE;
      }
	  
	  void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2){
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
		}
  """
  return C

def proxy_CreateFileA():
  C = """
      HANDLE WINAPI proxy_CreateFileA( LPCSTR filename, DWORD access,
DWORD sharing,            LPSECURITY_ATTRIBUTES sa, DWORD creation,
DWORD attributes, HANDLE template )
      {
          const char *tmpc;
          HANDLE *tmph;
          BOOL openbin = FALSE;
          /* is dacq opening a .BIN file for writing? */
		  MessageBoxA(0, "Creating File!!!", "Oops!", 0);
          if( (access & GENERIC_WRITE) && (strlen(filename) > 4)) {
              tmpc = filename + strlen(filename) - 4;
              if ( !strcmp(tmpc,".BIN") || !strcmp(tmpc,".bin") ) openbin = TRUE;
          }
          /* open the file */
          tmph = CreateFileA(filename, access, sharing, sa, creation,
attributes, template);
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
      HANDLE WINAPI proxy_CreateFileW( LPCSTR filename, DWORD access,
DWORD sharing,            LPSECURITY_ATTRIBUTES sa, DWORD creation,
DWORD attributes, HANDLE template )
      {
          const char *tmpc;
          HANDLE *tmph;
          BOOL openbin = FALSE;
          /* is dacq opening a .BIN file for writing? */
		  //MessageBoxA(0, "Creating File W!!!", "Oops!", 0);
		  tmpc = filename;
		  //MessageBoxW(0, tmpc, "Oops!", 0);
		  char buf[10];
		  itoa(wcslen(filename), buf, 10);
		  //MessageBoxA(0, buf, "Oops!", 0);
		  
          if(wcslen(filename) > 4) {	
		      LPCSTR fileext;
			  LPCSTR ebin = L".BIN";
			  //LPCSTR ebin = L"\x002E\x0042\x0049\x004E";
              fileext = filename + 2*wcslen(filename) - 8;		
			 // MessageBoxW(0, fileext, "Oops!", 0);
              if ( !wcscmp(fileext, ebin) || !wcscmp(fileext, L".bin")){
				 //MessageBoxA(0, "Creating W a BIN file", "Oops!", 0);
				 
				 check_bin++;
				 
				 if (check_bin > 1){
				 	window = SDL_CreateWindow("SDL2 Test", 50, 50, 800, 600, 0);
					renderer = SDL_CreateRenderer(window, -1, 0); // SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
					texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 800, 600);
	  
					SDL_SetRenderTarget(renderer, texture);
					SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
					SDL_RenderClear(renderer);
					
					//const char* error = SDL_GetError();
					//MessageBoxA(0, error, "Error", 0);
					
					// ???
					SDL_RenderPresent(renderer);
				}
							 
				openbin = TRUE;
			  }
          }
          /* open the file */		
		  //MessageBoxW(0, filename, "Oops!", 0);
          tmph = CreateFileW(filename, access, sharing, sa, creation,
attributes, template);
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

						
						//char buf [10];
						//itoa(nNumberOfBytesToWrite, buf, 10);
						//MessageBoxA(0, buf, "Oops!", 0);
						
						
												// display the current value
						int channel = 2;	
						int batch = 0;
						const int CH_MAP[] = { 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 24, 25, 26, 27, 28, 29, 30, 31 };
						short *ch_dat = (short*)((unsigned char *)lpBuffer + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[channel]);
						short val = *ch_dat;
						
						// TRANSFORM FOR DISPLAY
						// 11000, 5
						const int SHIFT = 5000;
						int plot_scale = 20;
						val = val + SHIFT;
						val = val > 0 ? val / plot_scale : 1;
						val = val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;
						
						//const char* error = SDL_GetError();
						//MessageBoxA(0, error, "Error", 0);
						
						// DISPLAY - ERROR AFTER THIS
						// ? check validity of renderer and texture ?
						SDL_SetRenderTarget(renderer, texture);
						
						// ??? 
						if (x_prev > 1){
							drawLine(renderer, x_prev, val_prev, x_prev + 1, val);

							SDL_SetRenderTarget(renderer, NULL);
							SDL_RenderCopy(renderer, texture, NULL, NULL);
							SDL_RenderPresent(renderer);

							// not to hang up
							SDL_PumpEvents();
						}
						else{
							// reset screen
							SDL_SetRenderTarget(renderer, texture);
							SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
							SDL_RenderClear(renderer);
							SDL_RenderPresent(renderer);
						}
						
						// update variables
						x_prev = (x_prev + 1) % SCREEN_WIDTH;
						val_prev = val;

					/* ADD YOUR CODE HERE: REROUTE THE USB PACKET */           
                                   /* for now we just
write the packet to the .BIN file, just like dacq,
                 but this may be changed to whatever we want */
              retval = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
lpNumberOfBytesWritten, lpOverlapped);
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

print k32imports

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
f = open("Mod__k32.c","wt")
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
# os.system("cl -O2 -W3 -LD Mod__k32.c Mod__k32.def -I\"C:\Program Files (x86)\Windows Kits\8.1\Include\um\"");
os.system('cl -O2 -W3 -LD Mod__k32.c Mod__k32.def -I"D:\data\igor\src\sdl_test\SDL2" -I"C:\Program Files (x86)\Windows Kits\8.1\Include\um" -I"C:\Program Files (x86)\Windows Kits\8.1\Include\shared" -I"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include" /link /LIBPATH:"C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\\x86" /LIBPATH:"D:\data\igor\src\sdl_test\lib\\x86" /DYNAMICBASE "SDL2.lib" /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib"')

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