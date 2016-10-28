/*
 * CluReaderClusteringProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "CluReaderClusteringProcessor.h"
#include "Utils.h"

CluReaderClusteringProcessor::CluReaderClusteringProcessor(LFPBuffer *buffer)
: CluReaderClusteringProcessor(buffer,
		buffer->config_->spike_files_[0] + "clu",
		buffer->config_->spike_files_[0] + "res"
		){

}

CluReaderClusteringProcessor::CluReaderClusteringProcessor(LFPBuffer *buffer, const std::string& clu_path,
		const std::string& res_path_base)
: LFPProcessor(buffer)
, clu_path_(clu_path)
, res_path_(res_path_base)
{
	max_clust_.resize(buffer->tetr_info_->tetrodes_number());

	if (!Utils::FS::FileExists(clu_path) || !Utils::FS::FileExists(res_path_)){
		Log("WARNING: clu and/or res files doesn't exist!");
		return;
	}

	files_exist_ = true;

	clu_stream_.open(clu_path_);
	res_stream_.open(res_path_);

	std::ifstream cluster_number_shifts_stream_(buffer->config_->spike_files_[0] + std::string("cluster_shifts"));
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
	if (!files_exist_){
		return;
	}

	if (clu_stream_.eof() && current_session_ < (int)buffer->session_shifts_.size() - 1){
		current_session_ ++;

		clu_path_ = buffer->config_->spike_files_[current_session_] + "clu";
		res_path_ = buffer->config_->spike_files_[current_session_] + "res";
		if (!Utils::FS::FileExists(clu_path_) || !Utils::FS::FileExists(res_path_)){
			Log("WARNING: clu and/or res files doesn't exist!");
			files_exist_ = false;
			return;
		}

		clu_stream_.close();
		res_stream_.close();
		clu_stream_.open(clu_path_);
		res_stream_.open(res_path_);
	}

	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_no_disp_pca){
		Spike * spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

		while(res < spike->pkg_id_ && !clu_stream_.eof()){
			clu_stream_ >> clust;
			res_stream_ >> res;
			res += buffer->session_shifts_[current_session_];
		}

		if (res == spike->pkg_id_){
			spike->cluster_id_ = std::max<int>(-1, clust - cluster_shifts_[spike->tetrode_]);
//			buffer->UpdateWindowVector(spike);
//			buffer->cluster_spike_counts_(spike->tetrode_, spike->cluster_id_) += 1;

			// to not use same entry again for other tetrode with the same time
			// otherwise next spike from other tetrode should be next
			if (spike->cluster_id_ > -1){
				//dummy cell objects
				if ((int)buffer->cells_[spike->tetrode_].size() < spike->cluster_id_ + 1){
					buffer->cells_[spike->tetrode_].resize(spike->cluster_id_ + 1);
				}

				clu_stream_ >> clust;
				res_stream_ >> res;
				res += buffer->session_shifts_[current_session_];
			}
		}
		else{
		}

		buffer->spike_buf_pos_clust_++;
	}
}
