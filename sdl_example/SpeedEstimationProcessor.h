/*
 * SpeedEstimationProcessor.h
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#ifndef SPEEDESTIMATIONPROCESSOR_H_
#define SPEEDESTIMATIONPROCESSOR_H_

#include "LFPProcessor.h"

class SpeedEstimationProcessor: public LFPProcessor {
public:
	std::ofstream dump_;

	SpeedEstimationProcessor(LFPBuffer *buffer);
	virtual ~SpeedEstimationProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* SPEEDESTIMATIONPROCESSOR_H_ */
