/*
 * LPTTriggerProcessor.cpp
 *
 *  Created on: Aug 15, 2014
 *      Author: igor
 */

#include "LPTTriggerProcessor.h"

LPTTriggerProcessor::LPTTriggerProcessor(LFPBuffer *buffer)
	: LFPProcessor(buffer)
	, channel_(buffer->config_->getInt("lpt.trigger.channel"))
{
	// TODO Auto-generated constructor stub

}

LPTTriggerProcessor::~LPTTriggerProcessor() {
	// TODO Auto-generated destructor stub
}

void LPTTriggerProcessor::process() {
	while(buffer->buf_pos_trig_ < buffer->buf_pos){
		if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] > 0 &&
				buffer->signal_buf[channel_][buffer->buf_pos_trig_] < 0)
		{
			// set to low
			std::cout << "set low at " << buffer->buf_pos_trig_ << "\n";
		}

		if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] < 0 &&
				buffer->signal_buf[channel_][buffer->buf_pos_trig_] > 0)
		{
			// set to high
			std::cout << "set high at " << buffer->buf_pos_trig_ << "\n";
		}

		buffer->buf_pos_trig_ ++;
	}
}
