/*
 * LPTTriggerProcessor.h
 *
 *  Created on: Aug 15, 2014
 *      Author: igor
 */

#ifndef LPTTRIGGERPROCESSOR_H_
#define LPTTRIGGERPROCESSOR_H_

#include "LFPProcessor.h"

class LPTTriggerProcessor: public LFPProcessor {
	unsigned int channel_;

public:
	LPTTriggerProcessor(LFPBuffer *buffer);
	virtual ~LPTTriggerProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* LPTTRIGGERPROCESSOR_H_ */
