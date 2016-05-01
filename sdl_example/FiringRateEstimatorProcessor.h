/*
 * FiringRateEstimatorProcessor.h
 *
 *  Created on: Mar 29, 2015
 *      Author: igor
 */

#ifndef FIRINGRATEESTIMATORPROCESSOR_H_
#define FIRINGRATEESTIMATORPROCESSOR_H_

#include "LFPProcessor.h"

class FiringRateEstimatorProcessor : public virtual LFPProcessor {
	unsigned int fr_estimate_delay_ = 0;
//	std::vector<unsigned int> spike_numbers_;
	bool wait_speed_ = false;
	double speed_thold_ = .0;

public:
	FiringRateEstimatorProcessor(LFPBuffer *buf);
	virtual ~FiringRateEstimatorProcessor();

	virtual void process();
	virtual std::string name() { return "Firing Rate Estimator"; }
};

#endif /* FIRINGRATEESTIMATORPROCESSOR_H_ */
