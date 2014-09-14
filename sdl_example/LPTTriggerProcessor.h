/*
 * LPTTriggerProcessor.h
 *
 *  Created on: Aug 15, 2014
 *      Author: igor
 */

#ifndef LPTTRIGGERPROCESSOR_H_
#define LPTTRIGGERPROCESSOR_H_

#include "LFPProcessor.h"

#ifdef _WIN32
	#include "windows.h"
#endif

class LPTTriggerProcessor: public LFPProcessor {
	unsigned int channel_;

#ifdef _WIN32
	typedef void(__stdcall *lpOut32)(short, short);
	typedef short(__stdcall *lpInp32)(short);
	typedef BOOL(__stdcall *lpIsInpOutDriverOpen)(void);
	typedef BOOL(__stdcall *lpIsXP64Bit)(void);

	lpOut32 gfpOut32;
	lpInp32 gfpInp32;
	lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
	lpIsXP64Bit gfpIsXP64Bit;
	HINSTANCE hInpOutDll;
#endif

public:
	LPTTriggerProcessor(LFPBuffer *buffer);
	virtual ~LPTTriggerProcessor();

	// LFPProcessor
	virtual std::string name();
	virtual void process();
};

#endif /* LPTTRIGGERPROCESSOR_H_ */
