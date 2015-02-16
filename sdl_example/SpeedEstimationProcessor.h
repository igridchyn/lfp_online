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
	const unsigned int ESTIMATION_RADIUS = 8;

	SpeedEstimationProcessor(LFPBuffer *buffer);
	virtual ~SpeedEstimationProcessor();

	// LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "Spped Estimator"; }
};

#endif /* SPEEDESTIMATIONPROCESSOR_H_ */
