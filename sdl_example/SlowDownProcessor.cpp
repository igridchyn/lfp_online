/*
 * SlowDownProcessor.cpp
 *
 *  Created on: Jun 13, 2014
 *      Author: igor
 */

#include "SlowDownProcessor.h"

SlowDownProcessor::SlowDownProcessor(LFPBuffer* buffer)
:SlowDownProcessor(buffer,
		buffer->config_->getInt("sd.wait.milliseconds"),
		buffer->config_->getInt("sd.start")
		){

}

SlowDownProcessor::SlowDownProcessor(LFPBuffer* buffer, const unsigned int& wait_time, const unsigned int& slow_start)
 : LFPProcessor(buffer)
, WAIT_TIME_MS(wait_time)
, SLOW_START(slow_start){
	// TODO Auto-generated constructor stub

}

SlowDownProcessor::~SlowDownProcessor() {
	// TODO Auto-generated destructor stub
}

void SlowDownProcessor::process() {
	if (buffer->last_pkg_id > SLOW_START){
		usleep(1000 * WAIT_TIME_MS);
	}
}
