/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDClusteringProcessor.h"

KDClusteringProcessor::KDClusteringProcessor() {
	// TODO Auto-generated constructor stub

}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

void KDClusteringProcessor::process(){
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
		const unsigned int tetr = spike->tetrode_;

		obs_spikes_[tetr][total_spikes_[tetr] ++] = spike;

		if (total_spikes_[tetr] >= MIN_SPIKES){
			// sort in all coordinates and build k-d tree
		}

		buffer->spike_buf_pos_clust_ ++;
	}
}
