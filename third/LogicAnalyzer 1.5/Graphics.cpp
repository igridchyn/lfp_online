
//-----------------------------------------------------------------------------
//     Author : ElmueSoft (www.netcult.ch/elmue)
//-----------------------------------------------------------------------------

/*
----------------------------------------------------------------------------------
Using these conventions results in better readable code and less coding errors !
----------------------------------------------------------------------------------

     cName  for generic class definitions
     CName  for MFC     class definitions
     tName  for type    definitions
     eName  for enum    definitions
     kName  for struct  definitions

    e_Name  for enum variables
    E_Name  for enum constant values

    i_Name  for instances of classes
    h_Name  for handles

    M_Name  for macros
    T_Name  for Templates
    t_Name  for TCHAR or LPTSTR

    s_Name  for strings
   sa_Name  for Ascii strings
   sw_Name  for Wide (Unicode) strings
   bs_Name  for BSTR
    f_Name  for function pointers
    k_Name  for contructs (struct)

    b_Name  BOOL,BOOL 1 Bit

   s8_Name    signed  8 Bit (char)
  s16_Name    signed 16 Bit (SHORT)
  s32_Name    signed 32 Bit (LONG, int)
  s64_Name    signed 64 Bit (LONGLONG)

   u8_Name  unsigned  8 Bit (BYTE)
  u16_Name  unsigned 16 bit (WORD, WCHAR)
  u32_Name  unsigned 32 Bit (DWORD, UINT)
  u64_Name  unsigned 64 Bit (ULONGLONG)

    d_Name  for double

  ----------------

    m_Name  for member variables of a class (e.g. ms32_Name for int member variable)
    g_Name  for global (static) variables   (e.g. gu16_Name for global WORD)
    p_Name  for pointer                     (e.g.   ps_Name  *pointer to string)
   pp_Name  for pointer to pointer          (e.g.  ppd_Name **pointer to double)
*/

#include "stdafx.h"
#include "LogicAnalyzer.h"
#include "Graphics.h"

extern CLogicAnalyzerApp gi_App;

CGraphics::CGraphics()
{
	CleanUp(TRUE);

	COLORREF c_Cols[16];
	c_Cols[E_PaletteBackgr]  = 0x000000; // background
	c_Cols[E_PaletteRaster]  = 0x606060; // grid raster
	c_Cols[E_PaletteText]    = 0xFFFFDD; // text color for lables
	c_Cols[E_PaletteGreen]   = 0x00FF00; // green (correct)
	c_Cols[E_PaletteRed]     = 0xFF0000; // red   (error)
	c_Cols[E_PaletteLine0]   = 0x3EB0FF; // input line 0 (used for all Ports)
	c_Cols[E_PaletteLine1]   = 0xFFFF00; // input line 1 (used for all Ports)
	c_Cols[E_PaletteLine2]   = 0xFF00A5; // input line 2 (used for all Ports)
	c_Cols[E_PaletteLine3]   = 0x40E0D0; // input line 3 (used for all Ports)
	c_Cols[E_PaletteLine4]   = 0xFFA500; // input line 4 (used for all Ports)
	c_Cols[E_PaletteLine5]   = 0xDDA0DD; // input line 5 (used for all Ports)
	c_Cols[E_PaletteLine6]   = 0x495AFF; // input line 6 (used for all Ports)
	c_Cols[E_PaletteLine7]   = 0x77AA00; // input line 7 (used for all Ports)
	c_Cols[E_PaletteGlitch]  = 0x770000; // dark red background color for glitches
	c_Cols[E_PaletteUnusedB] = 0x000088; // unused
	c_Cols[E_PaletteUnusedC] = 0xAAAAAA; // unused

	SetPalette(c_Cols);
}

CGraphics::~CGraphics()
{
	CleanUp(FALSE);
}

void CGraphics::CleanUp(BOOL b_OnlyInit)
{
	if (!b_OnlyInit)
	{
		if (mpi_OldFont) mi_DC.SelectObject(mpi_OldFont);
		mi_Font.DeleteObject();

		if (mpi_OldPen) mi_DC.SelectObject(mpi_OldPen);
		mi_Pen.DeleteObject();

		if (mpi_OldBitmap) mi_DC.SelectObject(mpi_OldBitmap);
		mi_Bitmap.DeleteObject();

		mi_DC.DeleteDC();
	}

	mpi_OldBitmap = NULL;
	mpi_OldPen    = NULL;
	mpi_OldFont   = NULL;
	ms_FontName   = "Verdana";
	ms32_FontSize = 12;
	mu32_CurColor = 1;
	me_CurMode    = E_LineSolid;
	mpi_GifFile   = 0;
}

// Returns the oscilloscope raster for the given steps (1, 2, 2.5, 5, 10)
CGraphics::eRasterMode CGraphics::GetRasterMode(double d_Step)
{
	switch ((int)(d_Step * 1000.0))
	{
		case 1000: return CGraphics::E_Oszi_10; break;
		case 2000: return CGraphics::E_Oszi_5;  break;
		case 2500: return CGraphics::E_Oszi_4;  break;
		case 5000: return CGraphics::E_Oszi_2;  break;
		default:   return CGraphics::E_Oszi_1;  break; // only used for odd user defined intervals
	}
}

// Loads the 16 palette colors
// Must be called before CreateNewDiagram()
// pc_Colors[E_PaletteBackgr] is the background color
void CGraphics::SetPalette(COLORREF* pc_Colors)
{
	for (int i=0; i<16; i++)
	{
		mc_Palette[i] = pc_Colors[i];

		// Convert 0xRRGGBB --> 0xBBGGRR
		mk_Palette[i].rgbBlue  = GetBValue(pc_Colors[i]);
		mk_Palette[i].rgbGreen = GetGValue(pc_Colors[i]);
		mk_Palette[i].rgbRed   = GetRValue(pc_Colors[i]);
		mk_Palette[i].rgbReserved = 0;
	}
}

DWORD CGraphics::GetPaletteColor(DWORD u32_PaletteIndex)
{
	if (u32_PaletteIndex > 0xF)
		return -1;

	return mc_Palette[u32_PaletteIndex];
}

// initializes a new diagram
DWORD CGraphics::CreateNewDiagram(DWORD u32_Width, DWORD u32_Height)
{
	if (u32_Width > 0xFFF0 || u32_Height > 0xFFF0)
		return ERROR_INVALID_PARAMETER;

	ms32_PixHeight = u32_Height;
	ms32_PixWidth  = u32_Width;

	ms32_RasterXstart = 0;
	ms32_RasterXend   = u32_Width;

	// The Bitmap rows start in memory DWORD padded (1 DWORD = 8 Pixel)
	ms32_PadWidth  = u32_Width;
	if (ms32_PadWidth & 7)
		ms32_PadWidth = ((ms32_PadWidth /8) +1) *8;
	
	CleanUp(FALSE);

	BITMAPINFO k_Info = { 0 };
	k_Info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	k_Info.bmiHeader.biHeight      = u32_Height;
	k_Info.bmiHeader.biWidth       = u32_Width;
	k_Info.bmiHeader.biPlanes      = 1;
	k_Info.bmiHeader.biBitCount    = 4;
	k_Info.bmiHeader.biCompression = BI_RGB	;
	k_Info.bmiHeader.biClrUsed     = 16;

	mi_DC.CreateCompatibleDC(NULL);

	HBITMAP h_Bitmap = CreateDIBSection(mi_DC, (BITMAPINFO*)&k_Info, DIB_PAL_COLORS, (void**)&mpu8_PixelData, 0, 0);
	if (!h_Bitmap)
		return GetLastError();

	mi_Bitmap.Attach(h_Bitmap);
	mi_DC.SetBkMode (TRANSPARENT);

	mpi_OldBitmap = mi_DC.SelectObject(&mi_Bitmap);

	// Store the palette
	SetDIBColorTable(mi_DC, 0, 16, mk_Palette);

	// Fill the background black
	return FillRectangle(0, 0, ms32_PixWidth, ms32_PixHeight, E_PaletteBackgr);
}

// returns width and height
SIZE CGraphics::GetDimensions()
{
	SIZE k_Size;
	k_Size.cx = ms32_PixWidth;
	k_Size.cy = ms32_PixHeight;
	return k_Size;
}

// Defines the color for further pen drawing operations (ePalette)
DWORD CGraphics::SetColor(DWORD u32_PaletteIndex)
{
	if (u32_PaletteIndex > 0xF)
		return ERROR_INVALID_PARAMETER;

	if (mu32_CurColor == u32_PaletteIndex && mpi_OldPen)
		return 0;

	mu32_CurColor = u32_PaletteIndex;

	if (mpi_OldPen) mi_DC.SelectObject(mpi_OldPen);
	mi_Pen.DeleteObject();
	mpi_OldPen = 0;
	return 0;
}

void CGraphics::SetLineMode(enum eLineMode e_Mode)
{
	if (me_CurMode == e_Mode && mpi_OldPen)
		return;

	me_CurMode = e_Mode;

	if (mpi_OldPen) mi_DC.SelectObject(mpi_OldPen);
	mi_Pen.DeleteObject();
	mpi_OldPen = 0;
}

void CGraphics::SetFont(CString s_FontName, int s32_FontSize)
{
	if (ms_FontName == s_FontName && ms32_FontSize == s32_FontSize && mpi_OldFont)
		return;

	ms_FontName   = s_FontName;
	ms32_FontSize = s32_FontSize;

	if (mpi_OldFont) mi_DC.SelectObject(mpi_OldFont);
	mi_Font.DeleteObject();
	mpi_OldFont = 0;
}

void CGraphics::DrawDot(int s32_X, int s32_Y)
{
	SetPixel(mi_DC, s32_X, s32_Y, mc_Palette[mu32_CurColor]);
}

// Draws a line between two points using the current color and drawing mode
// The destination point itself is not drawn! (see MSDN)
void CGraphics::DrawLine(int s32_X1, int s32_Y1, // from
						 int s32_X2, int s32_Y2) // to
{
	if (s32_X1 == s32_X2 && s32_Y1 == s32_Y2)
		return;

	if (!mpi_OldPen)
	{
		mi_Pen.CreatePen(me_CurMode, 1, mc_Palette[mu32_CurColor]);
		mpi_OldPen = (CPen*)mi_DC.SelectObject(&mi_Pen);
	}

	MoveToEx(mi_DC, s32_X1, s32_Y1, 0);
	LineTo  (mi_DC, s32_X2, s32_Y2);
}

void CGraphics::DrawRectangle(int s32_X, int s32_Y, int s32_Width, int s32_Height)
{
	DrawLine(s32_X,           s32_Y,            s32_X+s32_Width, s32_Y);
	DrawLine(s32_X+s32_Width, s32_Y,            s32_X+s32_Width, s32_Y+s32_Height);
	DrawLine(s32_X+s32_Width, s32_Y+s32_Height, s32_X,           s32_Y+s32_Height);
	DrawLine(s32_X,           s32_Y+s32_Height, s32_X,           s32_Y);
}

// Fill the given rectangle with a color
DWORD CGraphics::FillRectangle(int s32_X, int s32_Y, int s32_Width, int s32_Height, DWORD u32_PaletteIndex)
{
	if (u32_PaletteIndex > 0xF)
		return ERROR_INVALID_PARAMETER;

	COLORREF c_Color = mc_Palette[u32_PaletteIndex];
	mi_DC.FillSolidRect(s32_X, s32_Y, s32_Width, s32_Height, c_Color);
	return 0;
}

// Draw an oscilloscope raster
void CGraphics::DrawRaster(int s32_Xstart, int s32_Ystart, 
						   int s32_Xend,   int s32_Yend, 
						   int s32_Xstep,  int s32_Ystep, 
						   eRasterMode e_RasterX, eRasterMode e_RasterY)
{
	// Round to the next cell
	if (s32_Xend % s32_Xstep)
		s32_Xend = ((s32_Xend/s32_Xstep) +1) *s32_Xstep;

	ms32_RasterXstart = s32_Xstart;
	ms32_RasterXend   = s32_Xend;

	DWORD u32_Count = 0;
	for (int X=s32_Xstart; X<=s32_Xend; X+=s32_Xstep)
	{
		SetRasterLineMode(u32_Count++, e_RasterX);
		DrawLine(X, s32_Ystart, X, s32_Yend);
	}

	u32_Count = 0;
	for (int Y=s32_Yend; Y>=s32_Ystart; Y-=s32_Ystep)
	{
		SetRasterLineMode(u32_Count++, e_RasterY);
		DrawLine(s32_Xstart, Y, s32_Xend, Y);
	}
}

void CGraphics::SetRasterLineMode(DWORD u32_Count, eRasterMode e_Raster)
{
	switch (e_Raster)
	{
		case E_Oszi_1:
			SetLineMode(E_LineDotted);
			break;

		case E_Oszi_2:
			switch (u32_Count % 2)
			{
				case 0:  SetLineMode(E_LineSolid);  break;
				default: SetLineMode(E_LineDotted); break;
			}
			break;

		case E_Oszi_4:
			switch (u32_Count % 4)
			{
				case 0:  SetLineMode(E_LineSolid);  break;
				default: SetLineMode(E_LineDotted); break;
			}
			break;

		case E_Oszi_5:
			switch (u32_Count % 5)
			{
				case 0:  SetLineMode(E_LineSolid);  break;
				default: SetLineMode(E_LineDotted); break;
			}
			break;

		case E_Oszi_10:
			switch (u32_Count % 10)
			{
				case 0:  SetLineMode(E_LineSolid);  break;
				case 5:  SetLineMode(E_LineDashed); break;
				default: SetLineMode(E_LineDotted); break;
			}
			break;

		default:
			SetLineMode(E_LineSolid);
			break;
	}
}

// If e_Pos == E_Only_Measure --> the width and height are returned in k_Size
SIZE CGraphics::DrawText(LPCTSTR s_Text, int s32_X, int s32_Y, int s32_Width, int s32_Height, eTextPos e_Pos)
{
	SIZE k_Size = { 0 };

	if (!s_Text[0]) return k_Size; // String empty

	if (!mpi_OldFont)
	{
		mi_Font.CreateFont(-ms32_FontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, ms_FontName);
		mpi_OldFont = (CFont*)mi_DC.SelectObject(&mi_Font);
	}

	mi_DC.SetTextColor(mc_Palette[mu32_CurColor]);

	RECT k_Rect = { s32_X, s32_Y, s32_X+s32_Width, s32_Y+s32_Height };
	mi_DC.DrawText(s_Text, &k_Rect, e_Pos);

	// Measure the width and height of the text without drawing it
	if (e_Pos == E_OnlyMeasure)
	{
		k_Size.cx = k_Rect.right  - k_Rect.left;
		k_Size.cy = k_Rect.bottom - k_Rect.top;
	}
	return k_Size;
}

// This function should be called after DrawRaster() !
// Prints a time scale in the form "15:26:37.435.021" with microsecond resolution
// d_Time must be passed as seconds since 00:00 o'clock
DWORD CGraphics::DrawTimeAxis(double d_Time, int s32_Yoff, int s32_Height, int s32_Xstep, double d_Step)
{
	// Measure the width and height of the time string
	CString s_Time = gi_App.mi_Func.FormatExactTime(0.0);
	SIZE    k_Size = DrawText(s_Time, 0, 0, 0xFFFF, 0xFFFF, E_OnlyMeasure);

	// The time string (plus space) does not fit
	if (s32_Xstep < k_Size.cx * 3 / 2)
		return ERROR_INVALID_PARAMETER;

	for (int X=ms32_RasterXstart; X<ms32_RasterXend-k_Size.cx; X+=s32_Xstep, d_Time+=d_Step)
	{
		s_Time = gi_App.mi_Func.FormatExactTime(d_Time);
		DrawText(s_Time, X, s32_Yoff, 200, s32_Height, E_MiddleLeft);
	}
	return 0;
}


/*###########################################################################
#############################################################################
#############################################################################

                            ___________________

                               GIF Algorithm
                            ___________________

Copyright (C) 1989 by Jef Poskanzer.

Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
Lempel-Ziv compression modifications by David Rowley (mgardi@watdcsu.waterloo.edu)
Adapted for Visual C++ by Elmü <elmue@gmx.de>

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.  This software is provided "as is" without express or
implied warranty.

The Graphics Interchange Format(c) is the Copyright property of CompuServe Incorporated.  
GIF(sm) is a Service Mark property of CompuServe Incorporated.


Based on: File compression IEEE Computer, June 1984.

By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
             Jim McKie               (decvax!mcvax!jim)
             Steve Davies            (decvax!vax135!petsd!peora!srd)
             Ken Turkowski           (decvax!decwrl!turtlevax!ken)
             James A. Woods          (decvax!ihnp4!ames!jaw)
             Joe Orost               (decvax!vax135!petsd!joe)
             
You'll find the latest release of Netpbm at http://freshmeat.net
There search for "NETPBM"  !!


#############################################################################
#############################################################################
###########################################################################*/


void CGraphics::Abort()
{
	mb_Abort = TRUE;
}

// ms32_PixHeight, ms32_PixWidth (graphic dimensions) have to be set before calling this function !

// GIF data stream begins at the upper left edge of the image
DWORD CGraphics::SaveAsGif(CString s_Path)
{
	GdiFlush(); // Assure that drawing operations are written to pu8_PixelData

	mb_Abort = FALSE;
	DWORD u32_Err;
	BOOL  b_Transparent  = FALSE; // if TRUE : Palette index 0 has to be the transparent color !
	int s32_BitsPerPixel = 4;     // 16 Color GIF   (maximum 8 for 256 colors)

	// This file will always be closed in the CFileEx destructor when this function exits
	CFileEx i_GifFile;
	mpi_GifFile = &i_GifFile;

	if (u32_Err = i_GifFile.Open(s_Path, CFileEx::E_WriteCreate))
		return u32_Err;

	// current X and Y position
	ms32_PixPosX = 0;
	ms32_PixPosY = ms32_PixHeight -1;;

	// ************* create GIF header *******************

	// Write the Magic header
	if (b_Transparent) strcpy(ms8_FileOut, "GIF89a");
	else               strcpy(ms8_FileOut, "GIF87a");
        
	DWORD i = strlen(ms8_FileOut);

	// Write out the image width and height
	ms8_FileOut[i++] = ms32_PixWidth  & 255;
	ms8_FileOut[i++] = ms32_PixWidth  / 256;

	ms8_FileOut[i++] = ms32_PixHeight & 255;
	ms8_FileOut[i++] = ms32_PixHeight / 256;


	/* Packet Bits
	Hi    Lo
	PCCCSTTT

	P   = 1 : global Palette present
	CCC = Color Resolution (Bits per color -1, it does't matter how many 
	                        of the palette colors are really used)
	S   = 1 : Palette is ordered by decreasing color importance
	TTT = global Palette size -1 (TTT = 4 means 32 colors) */

	int s32_Packet  = 0x80; // Yes, there is a palette
	    s32_Packet |= (s32_BitsPerPixel - 1) << 4;
	    s32_Packet |= (s32_BitsPerPixel - 1);

	ms8_FileOut[i++] = s32_Packet; // Packet Bits 
	ms8_FileOut[i++] = 0; // background color index
	ms8_FileOut[i++] = 0; // aspect ratio (relation width / height of a pixel) not used

	// write palette
	int s32_PaletteSize = 1 << s32_BitsPerPixel;
	for (int p=0; p<s32_PaletteSize; ++p) 
	{
		ms8_FileOut[i++] = (char)((mc_Palette[p] / 0x10000) & 0xFF); // Red
		ms8_FileOut[i++] = (char)((mc_Palette[p] /   0x100) & 0xFF); // Green
		ms8_FileOut[i++] = (char)((mc_Palette[p]          ) & 0xFF); // Blue
	}

	// Write out extension for transparent colour index, if necessary.
	if (b_Transparent) 
	{
		ms8_FileOut[i++] = '!';         // graphic extension block introducer
		ms8_FileOut[i++] = (char) 0xf9; // Graphic Control Extension ID
		ms8_FileOut[i++] = 4;           // Block size
		                 
		/* again : Packet Bits
		Hi    Lo
		RRRDDDUT

		RRR   : reserved
		DDD   : Disposal (rendering background)
		U = 1 : User input expected (Carriage return, Mouse click)
		T = 1 : Transparency given  */
	    
		ms8_FileOut[i++] = 1; // Packet Bits 

		ms8_FileOut[i++] = 0; // Delay Lo (1/100 seconds)
		ms8_FileOut[i++] = 0; // Delay Hi (1/100 seconds)
		
		ms8_FileOut[i++] = E_PaletteBackgr; // transparent background color index
		ms8_FileOut[i++] = 0; // block terminator
	}

	ms8_FileOut[i++] =  ',';  // Image separator

	// Write the Image header
	ms8_FileOut[i++] = 0;     // left Offset position Lo (not used)
	ms8_FileOut[i++] = 0;     // left Offset position Hi (not used)

	ms8_FileOut[i++] = 0;     //  top Offset position Lo (not used)
	ms8_FileOut[i++] = 0;     //  top Offset position Hi (not used)

	ms8_FileOut[i++] = ms32_PixWidth  & 0xFF;
	ms8_FileOut[i++] = ms32_PixWidth  / 256;  // image width

	ms8_FileOut[i++] = ms32_PixHeight & 0xFF;
	ms8_FileOut[i++] = ms32_PixHeight / 256;  // image height


	/* again : Packet Bits
	Hi    Lo
	LISRRTTT

	L = 1 : local palette present
	I = 1 : Interlaced
	S = 1 : ordered Palette
	RR    : reserved
	TTT   : size of local palette -1 or zero if not present */
	ms8_FileOut[i++] = 0;  // Packet Bits 

	// initial code size
	int s32_InitCodeSize = s32_BitsPerPixel;
	if (s32_BitsPerPixel <= 1) s32_InitCodeSize = 2;

	ms8_FileOut[i++] = s32_InitCodeSize;  // LZW initial Code Size

	if (u32_Err = i_GifFile.Write(ms8_FileOut, i))
		return u32_Err;
	
	// ************* LZW compression *****************

	if (u32_Err = LzwCompress(s32_InitCodeSize +1))
		return u32_Err;

	// ************* close File *****************
	i = 0;
	ms8_FileOut[i++] = 0;   // Block terminator
	ms8_FileOut[i++] = ';'; // GIF file terminator

	if (u32_Err = i_GifFile.Write(ms8_FileOut, i))
		return u32_Err;

	i_GifFile.Close();

	if (mb_Abort) DeleteFile(s_Path);

	CleanUp(FALSE);
	return 0;
}


// Lempel - Ziv - Welch  Algorithm:  
// Use open addressing double hashing (no chaining) on the
// prefix code / next character combination.  We do a variant of Knuth's
// algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
// secondary probe.  Here, the modular division first probe is gives way
// to a faster exclusive-or manipulation.  Also do block compression with
// an adaptive reset, whereby the code table is cleared when the compression
// ratio decreases, but after the table fills.  The variable-length output
// codes are re-sized at this point, and a special CLEAR code is generated
// for the decompressor.  Late addition:  construct the table according to
// file size for noticeable speed improvement on small files.  Please direct
// questions about this implementation to ames!jaw.
DWORD CGraphics::LzwCompress(DWORD u32_Init_Bits)
{
	DWORD u32_Err;
	int s32_PixCol   = 0;
	int s32_fCode    = 0;
	int s32_Disp     = 0;
	int i            = 0;

	  mb_Clear       = FALSE;
	mu32_Accum       = 0;
	ms32_AccuBits    = 0;

	mu32_MaxBits     = 12;                // max bits/code
	mu32_MaxMaxCode  = 1 << mu32_MaxBits;  // 4096 this code should NEVER be generated

	mu32_InitBits    = u32_Init_Bits;
	mu32_NumBits     = mu32_InitBits;
	mu32_MaxCode     = (1 << mu32_NumBits) - 1;

	mu32_ClearCode   = 1 << (mu32_InitBits - 1); // Clear code is defined as 2^Codesize
	                                           // tells decoder to process as if a new data stream was starting
	mu32_EofCode     = mu32_ClearCode + 1;       // End code stopps all processing of the stream
	mu32_FirstFree   = mu32_ClearCode + 2;       // the first available compression code

	mu32_BlkCount    = 0;

	int s32_Entry    = GetNextPixel();

	int  s32_hShift = 0;
	for (s32_fCode = __HashSize; s32_fCode < 65536; s32_fCode *=2)
	{
		s32_hShift ++;
	}

	s32_hShift = 8 - s32_hShift;  // set hash code range bound

	// clear hash table (DWORD = BYTE *4)
	memset(ms32_HashTab, -1, __HashSize *4);

	if (u32_Err = BitStreamOut(mu32_ClearCode))
		return u32_Err;

	while ((s32_PixCol = GetNextPixel()) != -1)
	{
		if (mb_Abort)
			return 0;

		s32_fCode = (s32_PixCol << mu32_MaxBits) + s32_Entry;
		i = (s32_PixCol << s32_hShift) ^ s32_Entry;    // xor hashing

		if (ms32_HashTab[i] == s32_fCode) 
		{
			s32_Entry = mu16_CodeTab[i];
			continue;
		} 
		else if (ms32_HashTab[i] < 0) goto nomatch; // empty slot
            
		s32_Disp = __HashSize - i;  // secondary hash (after G. Knott)
		if (i == 0) s32_Disp = 1;

	probe:
		if ((i -= s32_Disp) < 0) i += __HashSize;

		if (ms32_HashTab[i] == s32_fCode) 
		{
			s32_Entry = mu16_CodeTab[i];
			continue;
		}

		if (ms32_HashTab[i] > 0) goto probe;

	nomatch:
		if (u32_Err = BitStreamOut(s32_Entry))
			return u32_Err;

		s32_Entry = s32_PixCol;

		if (mu32_FirstFree < mu32_MaxMaxCode) 
		{
			mu16_CodeTab[i] = (WORD)(mu32_FirstFree ++);   // code -> hashtable
			ms32_HashTab[i] = s32_fCode;
		} 
		else 
		{
			memset(ms32_HashTab, -1, __HashSize *4); // clear hash table (DWORD = BYTE *4)
			mu32_FirstFree = mu32_ClearCode + 2;
			mb_Clear = TRUE;

			if (u32_Err = BitStreamOut(mu32_ClearCode))
				return u32_Err;
		}
	}

	if (u32_Err = BitStreamOut(s32_Entry))
		return u32_Err;
	if (u32_Err = BitStreamOut(mu32_EofCode))
		return u32_Err;
	
	return 0;
}


// generates a bit stream of up to 12 Bits from the given code.
// Output in units of one byte

// Maintain a BITS character long buffer (so that 8 codes will
// fit in it exactly).  Use the VAX insv instruction to insert each
// code in turn.  When the buffer fills up empty it and start over.
DWORD CGraphics::BitStreamOut(DWORD u32_Code)
{
	const static DWORD u32_Masks[] = { 0x0000, 
                                       0x0001, 0x0003, 0x0007, 0x000F,
                                       0x001F, 0x003F, 0x007F, 0x00FF,
                                       0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                       0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

	DWORD u32_Err;
	mu32_Accum &= u32_Masks[ms32_AccuBits];

	if (ms32_AccuBits) mu32_Accum |= (u32_Code << ms32_AccuBits);
	else               mu32_Accum  =  u32_Code;

	ms32_AccuBits += mu32_NumBits;

	// output the next 8 Bits
	while (ms32_AccuBits >= 8) 
	{
		if (u32_Err = PackBlock((char)(mu32_Accum & 0xff)))
			return u32_Err;

		mu32_Accum    >>= 8;
		ms32_AccuBits  -= 8;
	}

	// If the next entry is going to be too big for the code size,
	// then increase it, if possible.
	if (mu32_FirstFree > mu32_MaxCode || mb_Clear) 
	{
		if (mb_Clear) 
		{
			mu32_NumBits = mu32_InitBits;
			mu32_MaxCode = (1 << mu32_NumBits) - 1;
			mb_Clear = FALSE;
		} 
		else 
		{
			mu32_NumBits ++;
			if (mu32_NumBits == mu32_MaxBits) mu32_MaxCode = mu32_MaxMaxCode;
			else                              mu32_MaxCode = (1 << mu32_NumBits) - 1;
		}
	}

	if (u32_Code == mu32_EofCode) 
	{
		// At EOF, write the rest of the buffer.
		while (ms32_AccuBits > 0) 
		{
			if (u32_Err = PackBlock((char)(mu32_Accum & 0xff)))
				return u32_Err;

			mu32_Accum    >>= 8;
			ms32_AccuBits  -= 8;
		}
		return FlushBlock();
    }
	return 0;
}

// Add a character to the end of the current packet, and if it is 254
// characters, flush the packet to disk preceded by packet length.
DWORD CGraphics::PackBlock(char s8_Chr)
{
	ms8_FileOut[++ mu32_BlkCount] = s8_Chr; // don't write at position 0 ! (here comes the block length)

	if (mu32_BlkCount >= 254) 
		return FlushBlock();
	else
		return 0;
}

// Flush the packet to disk, and reset the counter
DWORD CGraphics::FlushBlock()
{
	if (!mu32_BlkCount) 
		return 0;

	ms8_FileOut[0] = (char)(mu32_BlkCount); // Block length

	mu32_BlkCount ++;
	DWORD u32_Err = mpi_GifFile->Write(ms8_FileOut, mu32_BlkCount);

	mu32_BlkCount = 0;
	return u32_Err;
}

// Returns the palette index for all pixels (one by one beginning at the upper left)
// Return  -1 if End
int CGraphics::GetNextPixel()
{
	if (ms32_PixPosY < 0) 
		return -1;
	
	// The Bitmap rows start in memory DWORD padded (1 DWORD = 8 Pixel)
	DWORD u32_MemPos = ms32_PadWidth * ms32_PixPosY + ms32_PixPosX;
	int s32_Ret = mpu8_PixelData[u32_MemPos / 2];

	// is pixel at even position ?	
	if (!(ms32_PixPosX & 1)) s32_Ret >>= 4;

	ms32_PixPosX ++;
	if (ms32_PixPosX >= ms32_PixWidth) 
	{
		ms32_PixPosX = 0;
		ms32_PixPosY --;
	}
	return s32_Ret & 0x0F;
}

