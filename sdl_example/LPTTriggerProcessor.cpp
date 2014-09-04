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
{
#ifdef _WIN32
	Opendriver();
#endif
}

LPTTriggerProcessor::~LPTTriggerProcessor() {
}

void LPTTriggerProcessor::process() {
	while(buffer->buf_pos_trig_ < buffer->buf_pos){
		if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] > 10000 &&
				buffer->signal_buf[channel_][buffer->buf_pos_trig_] < 10000)
		{
			// set to low
			std::cout << "set low at " << buffer->buf_pos_trig_ << "\n";
			buffer->log_stream << "INFO: LPT Trigger: detect DROP at " << buffer->buf_pos_trig_ << "\n";
#ifdef _WIN32
			Out32(0x0378, 0xFF);
#endif
		}

		if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] < -10000 &&
				buffer->signal_buf[channel_][buffer->buf_pos_trig_] > -10000)
		{
			// set to high
			std::cout << "set high at " << buffer->buf_pos_trig_ << "\n";
			buffer->log_stream << "INFO: LPT Trigger: detect CLIMB at " << buffer->buf_pos_trig_ << "\n";
#ifdef _WIN32
			Out32(0x0378, 0x00);
#endif
		}

		buffer->buf_pos_trig_ ++;
	}
}
