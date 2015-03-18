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
	EnvironmentDominance,

	DoubleThresholdCrossing,
	RegularFalshes
};

enum LPTTriggerTarget{
	LPTTargetLFP,
	LPTTargetSpikes
};

class LPTTriggerProcessor: public LFPProcessor {
	unsigned int channel_;
	LPTTriggerType trigger_type_;
	LPTTriggerTarget trigger_target_ = LPTTriggerTarget::LPTTargetSpikes;

	short iPort = 0xD050;

	bool LPT_is_high_ = false;
	void setHigh();
	void setLow();

	unsigned int last_trigger_time_ = 0;
	unsigned int pulse_length_ = 0;
	unsigned int trigger_cooldown_;
	// in samples
	unsigned int trigger_start_delay_ = 0;

	// this is fixed after start delay and used for synchrony deteciton later
	double average_spikes_in_synchrony_tetrodes_ = -1.0f;

	std::ofstream timestamp_log_;

	double dominance_confidence_threshold_ = .0;
	// pointer to SWR / synchrony events
	unsigned int swr_ptr_ = 0;
	unsigned int sync_min_duration_ = std::numeric_limits<unsigned int>::max();
	unsigned int sync_max_duration_ = 0;

	// depends on the strategy
	unsigned int & spike_buf_limit_ptr_;

	unsigned int last_synchrony_ = 0;

	// STATS
	unsigned int events_inhibited_thold_ = 0;
	unsigned int events_inhibited_timeout_ = 0;
	unsigned int events_allowed_thold_ = 0;
	unsigned int events_allowed_timeout_ = 0;

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

	inline double environment_dominance_confidence_();
};

#endif /* LPTTRIGGERPROCESSOR_H_ */
