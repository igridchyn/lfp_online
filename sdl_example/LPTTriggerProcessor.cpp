/*
 * LPTTriggerProcessor.cpp
 *
 *  Created on: Aug 15, 2014
 *      Author: igor
 */

#include "LPTTriggerProcessor.h"
#ifdef _WIN32
#include "inpout32.h"
#endif

LPTTriggerProcessor::LPTTriggerProcessor(LFPBuffer *buffer)
	: LFPProcessor(buffer)
	, channel_(buffer->config_->getInt("lpt.trigger.channel"))
	, trigger_cooldown_(buffer->config_->getInt("lpt.trigger.cooldown"))
	, trigger_start_delay_(buffer->config_->getInt("lpt.trigger.start.delay") * buffer->SAMPLING_RATE)
{
	Log("Constructor start");

	// TODO : read from config
	trigger_type_ = LPTTriggerType::HighSynchronyTrigger;
	timestamp_log_.open("trigger_timestamp.txt");

#ifdef _WIN32
	//Opendriver();

	// way 2
	hInpOutDll = LoadLibrary("InpOut32.DLL");
	if (hInpOutDll != NULL)
	{
		gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
		gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
		gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
		gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");
	}
	else{
		buffer->Log("ERROR: cannot load inpout32.DLL");
	}
#endif

	setLow();
	Log("Constructor done");
}

LPTTriggerProcessor::~LPTTriggerProcessor() {
	setLow();
	timestamp_log_.close();
}

void LPTTriggerProcessor::setLow() {
	//Out32(0x0378, 0xFF);
	//Out32(0xE050, 0xFF);

	// way2
#ifdef _WIN32
	gfpOut32(iPort, 0x00);
#endif


	Log("INFO: LPT Trigger: detect DROP at ", buffer->buf_pos_trig_);
}

void LPTTriggerProcessor::setHigh() {
	//Out32(0x0378, 0x00);
	//Out32(0xE050, 0x00);

	//way 2
#ifdef _WIN32
	gfpOut32(iPort, 0xFF);
#endif

	Log("INFO: LPT Trigger: detect CLIMB at ", buffer->buf_pos_trig_);
}

void LPTTriggerProcessor::process() {
	// check if need to turn off first
	if (LPT_is_high_ && buffer->last_pkg_id - last_trigger_time_ > pulse_length_){
		setLow();
		LPT_is_high_ = false;
	}

	// TODO detect start not since 0 but since first package id
	if (buffer->last_pkg_id < trigger_start_delay_){
		return;
	}
	else if (average_spikes_in_synchrony_tetrodes_ < 0){
		average_spikes_in_synchrony_tetrodes_ = buffer->AverageSynchronySpikesWindow();

		std::stringstream ss;
		ss << "Start inhibition, average number of spikes on synchrony tetrodes in population window of length " << buffer->POP_VEC_WIN_LEN << " = " << average_spikes_in_synchrony_tetrodes_;
		Log(ss.str());
	}

	switch(trigger_target_){
	case LPTTriggerTarget::LPTTargetLFP:
		while(buffer->buf_pos_trig_ < buffer->buf_pos){

			switch(trigger_type_){
			case DoubleThresholdCrossing:
				if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] > 0 &&
						buffer->signal_buf[channel_][buffer->buf_pos_trig_] < 0)
				{
					setLow();
				}
				if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] < -2400 &&
						buffer->signal_buf[channel_][buffer->buf_pos_trig_] > -2400)
				{
					setHigh();
				}
				break;
			default:
				Log("Chosen trigger type is not possible to use with LFP as a target");
				break;
			}

			buffer->buf_pos_trig_ ++;
		}
		break;

	case LPTTriggerTarget::LPTTargetSpikes:
		// TODO switch while and switch
		while(buffer->spike_buf_pos_lpt_ < buffer->spike_buf_nows_pos){

			switch(trigger_type_){
			case LPTTriggerType::HighSynchronyTrigger:
				if (buffer->IsHighSynchrony(average_spikes_in_synchrony_tetrodes_) && buffer->last_pkg_id - last_trigger_time_ > trigger_cooldown_){
					last_trigger_time_ = buffer->last_pkg_id;
					timestamp_log_ << last_trigger_time_ << "\n";
					timestamp_log_.flush();

					setHigh();
					LPT_is_high_ = true;
				}
				break;
			default:
				Log("Chosen trigger type is not possible to use with spikes as a target");
				break;
			}

			buffer->spike_buf_pos_lpt_ ++;
		}

		buffer->buf_pos_trig_ ++;
		break;
	}


}

std::string LPTTriggerProcessor::name() {
	return "LPTTriggerProcessor";
}
