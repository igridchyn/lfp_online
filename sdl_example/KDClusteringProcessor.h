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
#include <ANN/ANN.h>

class KDClusteringProcessor: public LFPProcessor {
	// per tetrode
	std::vector<unsigned int> total_spikes_;
	std::vector<arma::mat> observations_;
	std::vector<std::vector<Spike *> > obs_spikes_;

	const unsigned int MIN_SPIKES;

	// TODO: configurableize
	const unsigned int DIM = 12;

	// TODO: parametrize
	const unsigned int NN_K = 1000;
	const unsigned int NN_EPS = 0.1;
	const unsigned int NBINS = 20;
	const unsigned int BIN_SIZE = 5;

	const double X_STD = 100;
	const double Y_STD = 100;

	std::vector<ANNkd_tree*> kdtrees_;
	std::vector<ANNpointArray> ann_points_;

	// knn in the train set [tetrode][point]
	std::vector<std::vector<ANNidx*> > knn_cache_;

	// p(a, x) for each point in the set ( ? + medians between nearest neighbours ?)
	// !!! indexing should be the same as in obs_spikes_
	std::vector<std::vector<arma::mat> > spike_place_fields_;

	// build p(a_i, x)
	void build_pax_(const unsigned int tetr, const unsigned int spikei);
	double kern_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr);

public:
	KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes, const unsigned int depth);
	virtual ~KDClusteringProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* KDCLUSTERINGPROCESSOR_H_ */
