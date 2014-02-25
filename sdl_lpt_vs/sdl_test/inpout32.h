#include "stdafx.h"
#include <windows.h>
#include <WinBase.h>
#include <winioctl.h>
#include <conio.h>
#include <winsvc.h>
#include <minwindef.h>

int Opendriver(BOOL bX64);
void _stdcall Out32(short PortAddress, short data);