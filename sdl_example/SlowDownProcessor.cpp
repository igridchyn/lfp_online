/*
 * SlowDownProcessor.cpp
 *
 *  Created on: Jun 13, 2014
 *      Author: igor
 */

#include "SlowDownProcessor.h"

SlowDownProcessor::SlowDownProcessor(LFPBuffer* buffer, const unsigned int& wait_time)
 : LFPProcessor(buffer)
, WAIT_TIME_MS(wait_time){
	// TODO Auto-generated constructor stub

}

SlowDownProcessor::~SlowDownProcessor() {
	// TODO Auto-generated destructor stub
}

void SlowDownProcessor::process() {
	usleep(1000 * WAIT_TIME_MS);
}
