
#pragma once

#include "afxtempl.h"
#include "Port.h"
#include "Graphics.h"

class CAnalyzer  
{
	// Only for debugging purposes:
	#ifdef _DEBUG
		#define SHOW_SAMPLES  FALSE // Show all sample indices in the footer (decrease raster unit to be able to read them!)
		#define SHOW_IMAGES   FALSE // Show all images with an own blue border (for debugging split images)
	#endif

	#define MIN_GLITCH_LEN        5     // Don't show glitches that are shorter than 5 raster units in a separate block
	#define SHOW_SINGLE_GLITCHES  FALSE // Show long glitches in a separate block even if in the block no line changes its level


	#define TIME_SCALE_CELLS  20 // Every n-th cell print a X axis time label (must be a multiple of 2,4,5 and 10)

	#define MAX_IMG_WIDTH  16000 // maximum pixel width for all images (GIF images max width = 65535 pixel)
	#define CELL_WIDTH        15 // the pixel width  of the raster cells
	#define LABEL_WIDTH       60 // the pixel width  of the label text "Data 0" in front of each oscilloscope line
	#define IDLE_WIDTH       120 // the pixel width  of the display "Idle: 55,4 ms" at the start and end of a line

	#define CELL_HEIGHT       30 // the pixel height of the raster cells
	#define TIME_HEIGHT       15 // the pixel height of the time above
	#define CURVE_HEIGHT      20 // the pixel height of an oscilloscope line
	#define FOOTER_HEIGHT     16 // the pixel height of the footer with the analyzed bits and bytes

public:
	enum eDrawMode // Bit Flags
	{
		E_DrawImg    = 1,   // Draw Gif Images
		E_DrawFooter = 2,   // Draw Html Footer with decoded Bytes
		E_DrawAll    = E_DrawImg | E_DrawFooter
	};

	CAnalyzer();
	void    Analyze(eDrawMode e_DrawMode, double d_Unit, double d_Idle, double d_AnalyzeStart, double d_AnalyzeEnd, BOOL b_DeleteOldFiles, CString s_HtmlHeading);
	void    Abort();
	void    AnalyzeThread();
	void    GetUnits(double* pd_Unit, double* pd_Idle);
	CString GetProgress();

protected:
	struct kInputLines
	{
		DWORD u32_Port[PORT_COUNT*8]; // The line corresponds to Port X
		DWORD u32_Bit [PORT_COUNT*8]; // The line corresponds to Bit  Y
		DWORD u32_Count;              // The count of lines that have/don't have activity
	};

	struct kBlock
	{
		DWORD u32_FirstSample;  // The first sample in mpk_Memory
		DWORD u32_LastSample;   // The last  sample in mpk_Memory
		double  d_Xstart;       // very long output is split into multiple images -> start of image 
		double  d_Xend;         // very long output is split into multiple images -> end   of image 
		double  d_XendExtra;    // Additional Space at the end for serial protocol
		double  d_IdleBefore;   // The idle interval between the last and this block
		double  d_IdleAfter;    // The idle interval between this and the next block
		BOOL    b_IsEmpty;      // TRUE if no level change and no glitch longer than MIN_GLITCH_LEN in the block
		BOOL    b_HasLabel;     // All blocks have a label with the line names except split images
	};

	struct kAxisX
	{
		int s32_Label;        // The X position where the label starts
		int s32_CurveStart;   // The X position where the coloured oscilloscope line starts
		int s32_RasterStart;  // The X position where the grid raster starts
		int s32_ExtraSpace;   // The X position where the additional space starts (md_ExtraSpace)
		int s32_RasterEnd;    // The X position where the grid raster ends
		int s32_TotalWidth;   // The X position where the coloured oscilloscope line ends
	};

	struct kAxisY
	{
		int s32_TimeStart;                // The Y position where the time starts
		int s32_RasterStart;              // The Y position of the first raster line
		int s32_LineStart [PORT_COUNT*8]; // The Y position where the active Line [Index] starts
		int s32_CurveStart[PORT_COUNT*8]; // The Y position where the active Curve[Index] starts
		int s32_RasterEnd;                // The Y position of the last raster line where bits and bytes are displayed
		int s32_TotalHeight;              // The Y position where the image ends
	};

	void    AnalyzeMain();
	void    PreParseData(double* pd_Shortest);
	BOOL    PrintBlock(kBlock* pk_CurBlock, CString* ps_Html, CGraphics::eRasterMode e_RasterX);
	BOOL    PrintDiagram(CString* ps_Html, CGraphics::eRasterMode e_RasterX);
	CString PrintHtmlHead();
	void    GetAxisX();
	void    GetAxisY();
	void    GetAllSampleBlocks();
	BOOL    GetNextSampleBlock();
	int     GetActiveBit(DWORD u32_ActiveLine, DWORD u32_Sample);
	BOOL    PrintLine(DWORD u32_ActiveLine);
	void    DrawIdleInterval(double d_Idle, int s32_Xstart, int s32_Xend, int s32_Ypos, DWORD u32_Bit);
	
	// These functions are overridden in the derived class CSerial
	virtual void AnalyzeSerial();
	virtual void ResetSerial(kBlock* pk_CurBlock);
	virtual CString GetLineDescription(DWORD u32_Line);

	CArray<kBlock,kBlock> mi_Blocks;
	BOOL             mb_DeleteOldFiles;
	CString          ms_HtmlHeading;
	double           md_Unit;
	double           md_Idle;
	double           md_ClockTime;
	double           md_AnalyzeStartAbs;
	double           md_AnalyzeEndAbs;
	double           md_AnalyzeStartRel;
	double           md_AnalyzeEndRel;
	double           md_ExtraSpace;
	kInputLines      mk_Active;
	kInputLines      mk_Inactive;
	kBlock           mk_Block;
	kAxisX           mk_AxisX;
	kAxisY           mk_AxisY;
	CGraphics        mi_Graph;
	CPort::kSample*  mpk_Samples;
	DWORD            mu32_Samples;
	DWORD            mu32_PortMask;
	HANDLE           mh_Thread;
	BOOL             mb_Abort;
	DWORD            mu32_HtmlFileIdx;
	DWORD            mu32_ImgFileIdx;
	BOOL             mb_IndividualFooter;           // A space inside the image for decoded Bytes below the line
	eDrawMode        me_DrawMode;
	CString          ms_FooterHtml[PORT_COUNT * 8]; // One HTML footer for each active line
	CString          ms_Progress;
};

