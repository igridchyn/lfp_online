/*
 * CluReaderClusteringProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "CluReaderClusteringProcessor.h"

CluReaderClusteringProcessor::CluReaderClusteringProcessor(LFPBuffer *buffer)
: CluReaderClusteringProcessor(buffer,
		buffer->config_->getString("out.path.base") + "all.clu",
		buffer->config_->getString("out.path.base") + "all.res"
		){

}

CluReaderClusteringProcessor::CluReaderClusteringProcessor(LFPBuffer *buffer, const std::string& clu_path,
		const std::string& res_path_base)
: LFPProcessor(buffer)
, clu_path_(clu_path)
, res_path_(res_path_base)
{
	max_clust_.resize(buffer->tetr_info_->tetrodes_number());

	clu_stream_.open(clu_path_);
	res_stream_.open(res_path_);

	std::ifstream cluster_number_shifts_stream_(buffer->config_->getString("out.path.base") + "cluster_shifts.txt");
	while(!cluster_number_shifts_stream_.eof()){
		unsigned int shift;
		cluster_number_shifts_stream_ >> shift;
		cluster_shifts_.push_back(shift);
	}
}

CluReaderClusteringProcessor::~CluReaderClusteringProcessor() {
	clu_stream_.close();
	res_stream_.close();
}

void CluReaderClusteringProcessor::process() {
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_){
		Spike * spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

		while(res < spike->pkg_id_ && !clu_stream_.eof()){
			clu_stream_ >> clust;
			res_stream_ >> res;
		}

		if (res == spike->pkg_id_){
			spike->cluster_id_ = clust - cluster_shifts_[spike->tetrode_];
//			buffer->UpdateWindowVector(spike);
//			buffer->cluster_spike_counts_(spike->tetrode_, spike->cluster_id_) += 1;

			if(spike->cluster_id_ < -1){
				int a = 0;
				int b = a;
			}

			// to not use same entry again for other tetrode with the same time
			clu_stream_ >> clust;
			res_stream_ >> res;
		}
		else{
		}

		buffer->spike_buf_pos_clust_++;
	}
}
