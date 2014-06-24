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
	const unsigned int NN_K = 100;
	const unsigned int NN_EPS = 0.1;
	const unsigned int NBINS = 30;
	// TODO float
	const unsigned int BIN_SIZE = 20;

	// normalized to have average feature std / per tetrode
	std::vector<arma::Mat<int>> coords_normalized_;

	const int X_STD = 100;
	const int Y_STD = 100;

	// TODO: test for integer overflow in KDE operations
	const unsigned int MULT_INT = 1024;

	const bool SAVE = true;
	const bool LOAD = false;
	const std::string BASE_PATH;

	const unsigned int SAMPLING_RATE = 5;

	// TODO: !!! take from tetrode info? (channel nu,bner * 3 ???)
	const unsigned int N_FEAT = 12;

	std::vector<ANNkd_tree*> kdtrees_;
	std::vector<ANNpointArray> ann_points_;

	// for integer computations with increased precision (multiplier = MULT_INT)
	std::vector< int **  > ann_points_int_;
	std::vector< arma::Mat<int> > spike_coords_int_;

	// knn in the train set [tetrode][point]
	std::vector<std::vector<ANNidx*> > knn_cache_;

	// p(a, x) for each point in the set ( ? + medians between nearest neighbours ?)
	// !!! indexing should be the same as in obs_spikes_
	std::vector<std::vector<arma::mat> > spike_place_fields_;

	std::vector<bool> pf_built_;
	unsigned int n_pf_built_ = 0;

	// tmp - to estimate std
	std::vector< arma::mat > obs_mats_;

	// build p(a_i, x)
	void build_pax_(const unsigned int tetr, const unsigned int spikei);
	int inline kern_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr, const int& x, const int& y);

public:
	KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes, const std::string base_path);
	virtual ~KDClusteringProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* KDCLUSTERINGPROCESSOR_H_ */
