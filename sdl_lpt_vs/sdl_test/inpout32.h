#include "stdafx.h"
#include <windows.h>
#include <WinBase.h>
#include <winioctl.h>
#include <conio.h>
#include <winsvc.h>
#include <minwindef.h>
#include <stdio.h>
#include <tchar.h>


int __declspec(dllexport) Opendriver();
void __declspec(dllexport) Out32(short PortAddress, short data);