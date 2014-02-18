
#pragma once

class CPort  
{
	#ifdef _DEBUG
		// Enable only ONE of the following defines at a time
		#define SAVE_CAPTURE_TO_FILE    0  // In debug mode -> save captured data into file
		#define LOAD_CAPTURE_FROM_FILE  0  // In debug mode -> load captured data from file
	#endif


	// Defines how long a Context Switch must be to be detected as glitch.
	// If this value is 3 this means that a Context Switch must be longer than the triple of the
	// normal execution time of the Capture Loop.
	#define MIN_GLITCH_LENGTH  3.0


	// TRUE  -> start time axis at clock time  "17:35:48"
	// FALSE -> start time axis always at zero "00:00:00"
	#define CLOCK_TIME   TRUE 


	// ********************************************************************************
	// * The compiler is so intelligent that it will compile (1<<7) as a CONSTANT 128 *
	// * This value will NOT be calculated at run-time !!                             *
	// ********************************************************************************


	// ############### ALL PORTS ###################

	// The offset from the Port base address
	#define DATA_OFFSET   0
	#define STATUS_OFFSET 1
	#define CTRL_OFFSET   2
	#define PORT_COUNT    3

	// For usage in arrays:
	#define PORT_NAME_LIST   "Data", "Status", "Ctrl"

	// ########### DATA,STATUS,CTRL PORTS ###########

	// For internal use in u32_PortMask: contains the port(s) that the user has enabled
	#define DATA_MASK   (1 << DATA_OFFSET)
	#define STATUS_MASK (1 << STATUS_OFFSET)
	#define CTRL_MASK   (1 << CTRL_OFFSET)

	// On the Data Port nothing is inverted
	#define DATA_INVERTED   ((0))
	// On the Status Port the bits 2,7 are inverted
	#define STATUS_INVERTED ((1<<2)|(1<<7))
	// On the Control Port the bits 0,1,3,5 are inverted
	#define CTRL_INVERTED   ((1<<0)|(1<<1)|(1<<3)|(1<<5))

	// On the data port all bits are used
	#define DATA_USED_AS_INPUT    0xFF
	// On the status port the bits 3,4,5,6,7 are used
	#define STATUS_USED_AS_INPUT  ((1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7))
	// On the control port the bits 0,1,2,3 are used
	#define CTRL_USED_AS_INPUT    ((1<<0)|(1<<1)|(1<<2)|(1<<3))

	// Combine all 3 ports into one DWORD (17 bits set)
	#define ALL_USED_AS_INPUT  (((DATA_USED_AS_INPUT)   << (8 * DATA_OFFSET))   |  \
	                            ((STATUS_USED_AS_INPUT) << (8 * STATUS_OFFSET)) |  \
								((CTRL_USED_AS_INPUT)   << (8 * CTRL_OFFSET)))

	// Here you can define which Pin corresponds to which data line
	#define DATA_LINE_0   (1 << 0) // Pin  2
	#define DATA_LINE_1   (1 << 1) // Pin  3
	#define DATA_LINE_2   (1 << 2) // Pin  4
	#define DATA_LINE_3   (1 << 3) // Pin  5
	#define DATA_LINE_4   (1 << 4) // Pin  6
	#define DATA_LINE_5   (1 << 5) // Pin  7
	#define DATA_LINE_6   (1 << 6) // Pin  8
	#define DATA_LINE_7   (1 << 7) // Pin  9

	#define STATUS_LINE_0 (1 << 6) // Pin 10
	#define STATUS_LINE_1 (1 << 7) // Pin 11
	#define STATUS_LINE_2 (1 << 5) // Pin 12
	#define STATUS_LINE_3 (1 << 4) // Pin 13
	#define STATUS_LINE_4 (1 << 3) // Pin 15

	#define CTRL_LINE_0   (1 << 0) // Pin  1
	#define CTRL_LINE_1   (1 << 1) // Pin 14
	#define CTRL_LINE_2   (1 << 2) // Pin 16
	#define CTRL_LINE_3   (1 << 3) // Pin 17

	// The control port stores in it's highest bit if the capture data was affected by a glitch
	#define CTRL_GLITCH   (1 << 7)

	// ######### EXTENDED CONTROL REGISTER ##########

	#define ECR_OFFSET    0x402

	// The modes in which the Parallel Port can work are set in the Control Register
	enum eOpMode
	{
		E_Std_Mode      = (0 << 5),
		E_Byte_Mode     = (1 << 5),
		E_SPP_Fifo_Mode = (2 << 5),
		E_ECP_Fifo_Mode = (3 << 5),
		E_EPP_Mode      = (4 << 5),
		E_FifoTest_Mode = (6 << 5),
		E_Config_Mode   = (7 << 5),
	};


public:
	#pragma pack(1) // Safe memory (Byte aligned)
	struct kSample
	{
		double  d_Elapsed;          // The time since the begin of caputuring in seconds
		BYTE   u8_Data[PORT_COUNT]; // The data of the 3 ports
	};
	#pragma pack()

	DWORD BeepSpeaker(double d_Frequ, DWORD u32_Duration);
	void  InitializePorts(WORD u16_BasePort);
	DWORD CreateSpeedDiagram(WORD u16_BasePort, DWORD u32_PortMask);
	DWORD MeasureMaxParallelSpeed(WORD u16_BasePort, DWORD u32_PortMask, double* pd_Times, int s32_Loops, double* pd_Shortest, double* pd_Longest);
	DWORD GetPortData(WORD u16_BasePort, DWORD u32_PortMask, BYTE* pu8_Mem);
	int   GetPortBit (BYTE* pu8_Mem, DWORD u32_PortOffset, DWORD u32_Bit);
	DWORD GetCapturedBytes();
	void  GetCaptureBuffer(kSample** ppk_Samples, DWORD* pu32_Samples, DWORD* pu32_PortMask, double* pd_ClockTime);
	void  GetCaptureTime(double* pd_StartTime, double* pd_EndTime);
	DWORD GetCaptureLastError();
	BOOL  StartCapture(WORD u16_BasePort, DWORD u32_MemSize, DWORD u32_PortMask);
	void  StopCapture();
	void  CaptureThread();

private:
	struct kMemory
	{
		kSample*   pk_Samples;
		DWORD      u32_MemSize;  // Bytes
		kMemory()
		{
			memset(this, 0, sizeof(kMemory));
		}
		~kMemory()
		{
			HeapFree(GetProcessHeap(), 0, pk_Samples);
		}
	};

	struct kCapture
	{
		DWORD      u32_Samples;  // count of kSample's
		SYSTEMTIME k_StartTime;
		WORD       u16_BasePort;
		DWORD      u32_PortMask;
		DWORD      u32_LastError;
		BOOL       b_Abort;

		kCapture()
		{
			memset(this, 0, sizeof(kCapture));
		}
	};

	BOOL AllocateMem(DWORD u32_MemSize);
	DWORD DrawSpeedGraphic(CString s_GifFile, double* pd_Values, DWORD u32_Count, DWORD u32_LoopsPerPixel, double d_Longest);

	kMemory  mk_Memory;
	kCapture mk_Capture;
	HANDLE   mh_Thread;
};


