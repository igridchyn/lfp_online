/*
 * KDClusteringProcessor.h
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#ifndef KDCLUSTERINGPROCESSOR_H_
#define KDCLUSTERINGPROCESSOR_H_

#include "LFPProcessor.h"
#include <armadillo>

class KDClusteringProcessor: public LFPProcessor {
	// per tetrode
	std::vector<unsigned int> total_spikes_;
	std::vector<arma::mat> observations_;
	std::vector<std::vector<Spike *> > obs_spikes_;

	const unsigned int MIN_SPIKES;

public:
	KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes, const unsigned int depth);
	virtual ~KDClusteringProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* KDCLUSTERINGPROCESSOR_H_ */
