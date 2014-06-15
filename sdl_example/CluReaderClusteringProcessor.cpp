/*
 * CluReaderClusteringProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "CluReaderClusteringProcessor.h"

CluReaderClusteringProcessor::CluReaderClusteringProcessor(LFPBuffer *buffer, const std::string& clu_path, const std::string& res_path_base, int *tetrodes)
: LFPProcessor(buffer)
, clu_path_(clu_path)
, res_path_(res_path_base)
{
		max_clust_.resize(buffer->tetr_info_->tetrodes_number);

		int dum_ncomp;
		for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
			clu_streams_.push_back(new std::ifstream(clu_path_ + Utils::NUMBERS[tetrodes[t]]));
			res_streams_.push_back(new std::ifstream(res_path_ + Utils::NUMBERS[tetrodes[t]]));
			// read number of clusters given in the beginning of the file
			*(clu_streams_[t]) >> max_clust_[t];

			buffer->population_vector_window_[t].resize(max_clust_[t] - 1);
		}
}

CluReaderClusteringProcessor::~CluReaderClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

void CluReaderClusteringProcessor::process() {
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_){
		// read from clu.{tetr} cluster number
		Spike * spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

		int clust, res = 0;

		while(res < spike->pkg_id_ && !clu_streams_[spike->tetrode_]->eof()){
			*(clu_streams_[spike->tetrode_]) >> clust;
			*(res_streams_[spike->tetrode_]) >> res;
		}

		if (res == spike->pkg_id_){
			spike->cluster_id_ = clust - 3;
			if (clust < 3){
				// unknown cluster
				spike->cluster_id_ = -1;
				spike->discarded_ = true;
			}
			else{
				buffer->UpdateWindowVector(spike);
				buffer->cluster_spike_counts_(spike->tetrode_, spike->cluster_id_) += 1;
			}
//			std::cout << spike->tetrode_ << " " << spike->pkg_id_ << " " << clust << "\n";
		}
		else{
//			std::cout << "missing clu for res = " << res << "\n";
		}

		buffer->spike_buf_pos_clust_++;

//		if (spike->pkg_id_ > 10000)
//			exit(1);
	}
}
