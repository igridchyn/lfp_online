/*
 * LPTTriggerProcessor.h
 *
 *  Created on: Aug 15, 2014
 *      Author: igor
 */

#ifndef LPTTRIGGERPROCESSOR_H_
#define LPTTRIGGERPROCESSOR_H_

#include "LFPProcessor.h"
#include <fstream>

#ifdef _WIN32
	#include "windows.h"
#endif

enum LPTTriggerType{
	HighSynchronyTrigger,
	DoubleThresholdCrossing
};

enum LPTTriggerTarget{
	LPTTargetLFP,
	LPTTargetSpikes
};

class LPTTriggerProcessor: public LFPProcessor {
	unsigned int channel_;
	LPTTriggerType trigger_type_;
	LPTTriggerTarget trigger_target_ = LPTTriggerTarget::LPTTargetSpikes;

	short iPort = 0xE050;

	bool LPT_is_high_ = false;
	void setHigh();
	void setLow();

	unsigned int last_trigger_time_ = 0;
	unsigned int pulse_length_ = 400;
	unsigned int trigger_cooldown_ = 800;

	std::ofstream timestamp_log_;

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
