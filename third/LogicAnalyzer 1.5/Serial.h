
#pragma once

#include "Analyzer.h"

class CSerial : public CAnalyzer
{
	// for usage in s_BitDef:
	#define BIT_SEARCH_HIGH  '+'  // Only Async protocol: Search a High level in front of start bit
	#define BIT_SEARCH_LOW   '-'  // Only Async protocol: Search the Start Bit 

	#define BIT_START        '<'  
	#define BIT_STOP         '>'
	#define BIT_PARITY       'P'
	#define BIT_ACK          'A'

public:
	enum eProtocol // See Wikipedia!
	{
		// ATTENTION: Same order as in Combobox!!
		E_Proto_None  = 0,
		E_Proto_Async = 1,
		E_Proto_I2C,
		E_Proto_PS2,
		E_Proto_SPI,
		E_Proto_InfraRed,   // All infrared remote controls send serial data
		E_Proto_SmartCard   // Various smartcards use a simple CLOCK and DATA line for data transfer
	};

	enum eParity
	{
		E_ParNone = -1,
		E_ParEven = FALSE,  // the sum of all bits plus the parity bit is even (0)
		E_ParOdd  = TRUE    // the sum of all bits plus the parity bit is odd  (1)
	};

	enum eDataBits
	{
		E_5Data = 5,
		E_6Data = 6,
		E_7Data = 7,
		E_8Data = 8,
		E_9Data = 9
	};

	enum eDataOrder
	{
		E_MSB,   // Most  significant bit first
		E_LSB    // Least significant bit first
	};

	enum eStopBits
	{
		E_1Stop = 1,
		E_2Stop = 2  // only asynchronous protocols (e.g. RS232)
	};

	enum eAcknowlg
	{
		E_AckNone = 0, // Acknowledge is not in use
		E_AckLast = 1, // PS/2 Host-to-Device mode uses ACK Bit after Stop bit 
		// may be more ACK types will be required here for other protocols
	};

	enum eClock // only synchronous protocols (e.g. I2C) have a clock line
	{
		E_ClkPos =  1, // read data line when clock goes positive 
		E_ClkNeg = -1  // read data line when clock goes negative
	};

	void SetProtocol(eProtocol e_Protocol, eParity e_Parity, eDataBits e_DataBits, eStopBits e_StopBits, DWORD u32_Baudrate);

protected:

	// ===================================================================================

	// The MultiLine protocols like PS/2 or I2C use only the first item of the array
	// The Asynchronous Protocol needs a separate value for each line
	class cDecode
	{
	public:
		void SetLine(DWORD u32_Line)
		{
			u32_CurLine = u32_Line;
		}
		void ResetAll()
		{
			for (DWORD L=0; L<PORT_COUNT*8; L++)
			{
				Reset(L);
			}
			u32_CurLine = 0;
		}
		// Reset only current line
		void Reset() 
		{
			Reset(u32_CurLine);
		}
		// Reset any line
		void Reset(DWORD L) 
		{
			u16_ByteData[L] = 0;
			u32_BitIdx  [L] = 0;
			  d_BitPos  [L] = 0;
		}
		// Go to the next bit
		inline void NextBit()
		{
			u32_BitIdx[u32_CurLine] ++;
		}
		// Get/Set the decoded data
		inline WORD* ByteData()
		{
			return &u16_ByteData[u32_CurLine];
		}
		// Get/Set the sample time for the bit (Async protocol only)
		inline double* BitPos()
		{
			return &d_BitPos[u32_CurLine];
		}
		// Get the current bit
		inline char GetCurBit()
		{
			if (u32_BitIdx[u32_CurLine] < (DWORD)s_BitDef.GetLength())
				return s_BitDef.GetAt(u32_BitIdx[u32_CurLine]);
			
			ASSERT(0);
			return 0;
		}
		// The first bit ?
		inline BOOL IsFirst()
		{
			return (u32_BitIdx[u32_CurLine] == 0);
		}
		// The last bit ?
		inline BOOL IsLast()
		{
			return ((int)u32_BitIdx[u32_CurLine] >= s_BitDef.GetLength() -1);
		}
		
		CString s_BitDef;

	private:
		DWORD u32_CurLine;
		WORD  u16_ByteData[PORT_COUNT * 8];
		DWORD u32_BitIdx  [PORT_COUNT * 8];
		double  d_BitPos  [PORT_COUNT * 8];
	};

	// ===================================================================================

	enum ePrintPos
	{
		E_CurveLeft,
		E_CurveCenter,
		E_FooterLeft,
		E_FooterCenter
	};

	void BuildBitDefinition();
	BOOL ProcessCurBit(BOOL b_DataLevel, DWORD* pu32_Color, CString* ps_Print);
	void PrintText(LPCTSTR s_Text, double d_Elapsed, DWORD u32_Color, DWORD u32_Line, ePrintPos e_Pos);
	void PrintFooterHtml(LPCTSTR s_Text, DWORD Line, BOOL* pb_Glitch);

	void AnalyzeAsync();
	void AnalyzeI2C();
	void AnalyzePS2();
	void AnalyzeSPI();
	void AnalyzeInfraRed();
	void AnalyzeSmartCard();

	// These functions override the stub in the class CAnalyzer
	void    ResetSerial(kBlock* pk_CurBlock);
	void    AnalyzeSerial();
	CString GetLineDescription(DWORD u32_Line);

	eProtocol    me_Protocol;
	eParity      me_Parity;
	eDataBits    me_DataBits;
	eDataOrder   me_DataOrder;
	eStopBits    me_StopBits;
	eAcknowlg    me_Acknowlg;
	eClock       me_Clock;
	DWORD      mu32_Baudrate;
	cDecode      mi_Decode;
};


