/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDClusteringProcessor.h"

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes)
	: LFPProcessor(buf)
	, MIN_SPIKES(num_spikes) {
	// TODO Auto-generated constructor stub

	const unsigned int tetrn = buf->tetr_info_->tetrodes_number;

	kdtrees_.resize(tetrn);
	ann_points_.resize(tetrn);
	total_spikes_.resize(tetrn);
	obs_spikes_.resize(tetrn);
	knn_cache_.resize(tetrn);
	spike_place_fields_.resize(tetrn);

	for (int t = 0; t < tetrn; ++t) {
		ann_points_[t] = annAllocPts(MIN_SPIKES, DIM);
		spike_place_fields_[t].reserve(MIN_SPIKES);
	}
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

void KDClusteringProcessor::build_pax_(const unsigned int tetr, const unsigned int spikei) {
	arma::mat pf(NBINS, NBINS, arma::fill::zeros);

	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			double kern_sum = .0f;

			for (int ni = 0; ni < NN_K; ++ni) {
				kern_sum += kern_(spikei, knn_cache_[tetr][spikei][ni], tetr);
			}

			pf(xb, yb) = kern_sum;
		}
	}

	spike_place_fields_[tetr][spikei] = pf;
}

double inline KDClusteringProcessor::kern_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr) {
	// TODO: implement efficiently
	double sum = .0f;
	sum += (obs_spikes_[tetr][spikei1]->x - obs_spikes_[tetr][spikei2]->x) / X_STD;
	sum += (obs_spikes_[tetr][spikei1]->y - obs_spikes_[tetr][spikei2]->y) / Y_STD;

	ANNcoord *pcoord1 = ann_points_[tetr][spikei1], *pcoord2 = ann_points_[tetr][spikei2];
	for (int d = 0; d < DIM; ++d, ++pcoord1, ++pcoord2) {
		sum += (*pcoord1 - *pcoord2) * (*pcoord1 - *pcoord2);
	}

	return - sum / 2.0;
}

void KDClusteringProcessor::process(){
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
		const unsigned int tetr = spike->tetrode_;

		if (total_spikes_[tetr] >= MIN_SPIKES){
			// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes
			kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);

			ANNdist *dists = new ANNdist[NN_K];
			// call kNN for each point
			for (int p = 0; p < total_spikes_[tetr]; ++p) {
				ANNidx *nnIdx = new ANNidx[NN_K];

				kdtrees_[tetr]->annkSearch(ann_points_[tetr][p], NN_K, nnIdx, dists, NN_EPS);
				// TODO: cast/copy to unsigned short array?
				knn_cache_[tetr].push_back(nnIdx);
			}
			delete dists;



			// compute p(a_i, x) for all spikes (from neighbours
			for (int p = 0; p < total_spikes_[tetr]; ++p) {
				// DEBUG
				if (!(p % 500)){
					std::cout << p << " place fields built ...\n";
					if (p > 0)
						exit(0);
				}

				build_pax_(tetr, p);
			}
		}
		else{
			obs_spikes_[tetr].push_back(spike);

			for (int pc=0; pc < 3; ++pc) {
				// TODO: tetrode channels
				for(int chan=0; chan < 4; ++chan){
					ann_points_[tetr][total_spikes_[tetr]][chan * 3 + pc] = spike->pc[chan][pc];
				}
			}

			total_spikes_[tetr] ++;
		}

		buffer->spike_buf_pos_clust_ ++;
	}
}
