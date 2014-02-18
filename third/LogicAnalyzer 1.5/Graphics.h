
#pragma once

#include "FileEx.h"

class CGraphics
{
public:

	enum ePalette
	{
		E_PaletteBackgr = 0, // Palette index for background color
		E_PaletteRaster,     // Palette index for grid lines
		E_PaletteText,       // Palette index for text labels
		E_PaletteGreen,      // Palette index for green text (correct)
		E_PaletteRed,        // Palette index for red   text (error)
		E_PaletteLine0,      // Palette index for the first input line
		E_PaletteLine1,
		E_PaletteLine2,
		E_PaletteLine3,
		E_PaletteLine4,
		E_PaletteLine5,
		E_PaletteLine6,
		E_PaletteLine7,
		E_PaletteGlitch,
		E_PaletteUnusedB,
		E_PaletteUnusedC,
	};

	enum eLineMode
	{
		E_LineSolid  = PS_SOLID,
		E_LineDashed = PS_DASH,
		E_LineDotted = PS_DOT
	};

	enum eRasterMode
	{
		E_OnlySolid, // draws only solid  lines
		E_Oszi_1,    // draws only dotted lines
		E_Oszi_2,    // every  2. line is solid
		E_Oszi_4,    // every  4. line is solid
		E_Oszi_5,    // every  5. line is solid
		E_Oszi_10    // every 10. line is solid and every 5. line is dashed
	};

	enum eTextPos
	{
		E_TopLeft      = DT_TOP     | DT_LEFT   | DT_SINGLELINE,
		E_TopCenter    = DT_TOP     | DT_CENTER | DT_SINGLELINE,
		E_TopRight     = DT_TOP     | DT_RIGHT  | DT_SINGLELINE,
		E_MiddleLeft   = DT_VCENTER	| DT_LEFT   | DT_SINGLELINE,
		E_MiddleCenter = DT_VCENTER | DT_CENTER | DT_SINGLELINE,
		E_MiddleRight  = DT_VCENTER | DT_RIGHT  | DT_SINGLELINE,
		E_BottomLeft   = DT_BOTTOM  | DT_LEFT   | DT_SINGLELINE,
		E_BottomCenter = DT_BOTTOM  | DT_CENTER | DT_SINGLELINE,
		E_BottomRight  = DT_BOTTOM  | DT_RIGHT  | DT_SINGLELINE,
		// only measure the with and height of the string without drawing it
		E_OnlyMeasure  = DT_CALCRECT | E_TopLeft
	};

	CGraphics();
	virtual ~CGraphics();		
	DWORD CreateNewDiagram(DWORD u32_Width, DWORD u32_Height);
	SIZE  GetDimensions();
	DWORD SetColor(DWORD u32_PaletteIndex);
	void  SetPalette(COLORREF* pc_Colors);
	DWORD GetPaletteColor(DWORD u32_PaletteIndex);
	void  SetLineMode(enum eLineMode e_Mode);
	void  SetFont(CString s_FontName, int s32_FontSize);
	void  DrawLine(int s32_X1, int s32_Y1, int s32_X2, int s32_Y2);
	void  DrawDot(int s32_X, int s32_Y);
	SIZE  DrawText(LPCTSTR s_Text, int s32_X, int s32_Y, int s32_Width, int s32_Height, eTextPos e_Pos);
	void  DrawRectangle(int s32_X, int s32_Y, int s32_Width, int s32_Height);
	DWORD FillRectangle(int s32_X, int s32_Y, int s32_Width, int s32_Height, DWORD u32_PaletteIndex);
	void  DrawRaster(int s32_Xstart, int s32_Ystart, int s32_Xend, int s32_Yend, int s32_Xstep, int s32_Ystep, eRasterMode e_RasterX, eRasterMode e_RasterY);
	DWORD DrawTimeAxis(double d_Time, int s32_Yoff, int s32_Height, int s32_Xstep, double d_Step);
	DWORD SaveAsGif(CString s_Path);
	void  Abort();
	static eRasterMode GetRasterMode(double d_Step);

private:
	void CleanUp(BOOL b_OnlyInit);
	void SetRasterLineMode(DWORD u32_Count, eRasterMode e_Raster);

	COLORREF mc_Palette[16];
	RGBQUAD  mk_Palette[16];

	DWORD   mu32_CurColor;
	eLineMode me_CurMode;

	BYTE*   mpu8_PixelData;
	CBitmap   mi_Bitmap;
	CBitmap* mpi_OldBitmap;
	CPen      mi_Pen;
	CPen*    mpi_OldPen;
	CFont     mi_Font;
	CFont*   mpi_OldFont;
	CString   ms_FontName;
	int     ms32_FontSize;
	CDC       mi_DC;

	int ms32_PixHeight;  // size of resulting Bitmap Image
	int ms32_PixWidth;
	int ms32_PadWidth;

	int ms32_RasterXstart;
	int ms32_RasterXend;

	// ********** GIF stuff ****************

	CFileEx* mpi_GifFile;

	DWORD PackBlock(char s8_Chr);
	DWORD BitStreamOut(DWORD u32_Code);
	DWORD LzwCompress(DWORD u32_Init_Bits);
	DWORD FlushBlock();
	int   GetNextPixel();

	int ms32_PixPosX;
	int ms32_PixPosY;
	
	DWORD mu32_Accum;
	int   ms32_AccuBits;

	DWORD mu32_InitBits;
	DWORD mu32_NumBits;    // number of bits/code
	DWORD mu32_MaxBits;    // user settable max # bits/code
	DWORD mu32_MaxCode;    // maximum code, given n_bits
	DWORD mu32_MaxMaxCode; // should NEVER generate this code

	#define __HashSize  5003  //  (80% occupancy is 4096 = maxmaxcode)
	
	int  ms32_HashTab[__HashSize];
	WORD mu16_CodeTab[__HashSize];

	DWORD mu32_ClearCode;
	DWORD mu32_EofCode;
	DWORD mu32_FirstFree;  // first unused entry

	BOOL  mb_Clear;
	BOOL  mb_Abort;

	DWORD mu32_BlkCount;   // Number of characters so far in this 'packet'
	char  ms8_FileOut[1000];
};


