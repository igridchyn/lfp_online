/*
 * SlowDownProcessor.h
 *
 *  Created on: Jun 13, 2014
 *      Author: igor
 */

#ifndef SLOWDOWNPROCESSOR_H_
#define SLOWDOWNPROCESSOR_H_

#include "LFPProcessor.h"

// wait given amount of time at each iteration - for debugging and on-line output evaluation
class SlowDownProcessor: public LFPProcessor {
	const unsigned int WAIT_TIME_MS;
	const unsigned int SLOW_START;

public:
	SlowDownProcessor(LFPBuffer* buffer, const unsigned int& wait_time, const unsigned int& slow_start);
	virtual ~SlowDownProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* SLOWDOWNPROCESSOR_H_ */
