﻿import os
import string


# the following functions generates the bypass code in ANSI C

def proxy_Includes():
  C = """
      #include <windows.h>  
      #pragma comment(lib, "kernel32")
      #pragma comment(lib, "user32")
  """
  return C

def proxy_Globals():
  C = """
      HANDLE h_BIN_FILE = (HANDLE) NULL;
             /* YOU CAN INJECT ARBITRARY CODE INTO dacqUSB inside DllMain */
	
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
      HANDLE WINAPI proxy_CreateFileW( LPCWSTR filename, DWORD access,
DWORD sharing,            LPSECURITY_ATTRIBUTES sa, DWORD creation,
DWORD attributes, HANDLE template )
      {
          HANDLE *tmph;
          BOOL openbin = FALSE;
          /* is dacq opening a .BIN file for writing? */
		  
          if(wcslen(filename) > 4) {	
		      LPCWSTR fileext;
              fileext = filename + wcslen(filename) - 4;
              if ( !wcscmp(fileext, L".BIN") || !wcscmp(fileext, L".bin")){
				 MessageBoxA(0, "Creating a BIN file", "Oops!", 0);
				 openbin = TRUE;
			  }
          }
          /* open the file */
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
	  int counter_message = 0;
      BOOL WINAPI proxy_WriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD
nNumberOfBytesToWrite,
          LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped )
      {
          BOOL retval;
          /* are dacq writng to a .BIN file? */
          if ((h_BIN_FILE != (HANDLE) NULL) && (hFile == h_BIN_FILE)) {
			if (counter_message == 0){
				char buf [10];
				_itoa_s(nNumberOfBytesToWrite, buf, 10, 10);
				MessageBoxA(0, buf, "Package of length (bytes) == ", 0);
				counter_message++;
			}

					/* ADD YOUR CODE HERE: REROUTE THE USB PACKET */           
                                   /* for now we just
write the packet to the .BIN file, just like dacq,
                 but this may be changed to whatever we want */
              retval = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
lpNumberOfBytesWritten, lpOverlapped);
                         }  else /* not USB packet, just proxy */{
								retval = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
lpNumberOfBytesWritten, lpOverlapped);}
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
              while (strlist[0].split()):
                strlist.pop(0)
              strlist.pop(0)				

              while (strlist[0].split()):
                  entry = strlist[0].split()[-1];
                  retval.append(entry)
                  strlist.pop(0)
      strlist.pop(0)
  return retval


# use dumpbin to find the binary imports in dacqUSB.exe and DacqUSB.dll
os.system(r"dumpbin /imports dacqUSB.exe > dacqusb_imports.txt")
os.system(r"dumpbin /imports DacqUSB.dll > dacqusbdll_imports.txt")

# get the imports from kernel32.dll
k32imports = list()
k32imports.extend( process_import_dump("dacqusb_imports.txt") )
k32imports.extend( process_import_dump("dacqusbdll_imports.txt") )
k32imports_entries = set(k32imports)
k32imports = sorted([s for s in k32imports_entries])

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
f.write( proxy_WriteFile() )   #hijack of WriteFile
f.write( proxy_CloseHandle() ) #hijack of CloseHandle
f.write("\n\n")
f.close()

# compile the overriding DLL with Microsoft Visual C++
#os.system("cl -O2 -W3 -LD Mod__k32.c Mod__k32.def");
os.system('cl -O2 -W3 -LD Mod__k32.c Mod__k32.def -I"C:\Program Files (x86)\Windows Kits\8.1\Include\um" -I"C:\Program Files (x86)\Windows Kits\8.1\Include\shared" -I"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include" /link /LIBPATH:"C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\\x86" /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib"')

# clean up 
os.system("del Mod__k32.def");
os.system("del Mod__k32.obj");
os.system("del Mod__k32.lib");
os.system("del Mod__k32.exp");
os.system("del Mod__k32.c");
os.system("del dacqusbdll_imports.txt");
os.system("del dacqusb_imports.txt");

# write the new mod_DacqUSB.exe file
f = open("dacqUSB.exe","rb")
bytecode = f.read()
f.close()
bytecode = string.replace(bytecode,"kernel32.dll","Mod__k32.dll");
bytecode = string.replace(bytecode,"DacqUSB.dll","Mod_USB.dll");
f = open("mod_dacqUSB.exe","wb")
f.write(bytecode) # I hope this doesn't violate Axona's copyright !!!
f.close()

# write the new Mod_USB.dll file
f = open("DacqUSB.dll","rb")
bytecode = f.read()
f.close()
bytecode = string.replace(bytecode,"kernel32.dll","Mod__k32.dll");
f = open("Mod_USB.dll","wb")
f.write(bytecode) # I hope this doesn't violate Axona's copyright !!!
f.close()

# Now we have Mod_dacqUSB.exe and Mod_USB.dll, and are ready to run dacq with
# modified functionality!!!