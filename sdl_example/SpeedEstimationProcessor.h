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

	// for estimation of the mean number of spikes in a fixed time window
	OnlineEstimator<unsigned int, unsigned long long> mean_spike_number_estimator_;
	unsigned int current_window_spikes_ = 0;
	unsigned int last_window_end_ = 0;
	const bool ESTIMATE_WINDOW_SPIKE_NUMBER;
	const unsigned int WIN_LEN;
	const double RUNNING_SPEED_THOLD;
	const unsigned int SN_ESTIMATE_START;
	const unsigned int SN_ESTIMATE_END;
	bool sn_estimate_reported_ = false;

	SpeedEstimationProcessor(LFPBuffer *buffer);
	virtual ~SpeedEstimationProcessor();

	// LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "Speed Estimator"; }
};

#endif /* SPEEDESTIMATIONPROCESSOR_H_ */
