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
	unsigned i=0;
	while(!cluster_number_shifts_stream_.eof()){
		unsigned int shift;
		cluster_number_shifts_stream_ >> shift;

		if (cluster_number_shifts_stream_.eof())
			break;

		cluster_shifts_.push_back(shift);

		buffer->global_cluster_number_shfit_[i] = shift;

		if (i > 0){
			buffer->clusters_in_tetrode_[i-1] = buffer->global_cluster_number_shfit_[i] - buffer->global_cluster_number_shfit_[i-1];
		}

		i++;
	}
//	cluster_shifts_.erase(cluster_shifts_.end() - 1);

	std::string frpath = clu_path + ".frs";
	if (Utils::FS::FileExists(frpath)){
		std::ifstream frates(frpath);
		int cur_tet = 0;
		int cur_clu_glob = 0;
		float frate;
		buffer->cluster_firing_rates_.resize(buffer->tetr_info_->tetrodes_number());
		buffer->cluster_firing_rates_[0].resize(50);
		while (!frates.eof()){
			while (cur_tet < buffer->tetr_info_->tetrodes_number() - 1 && cur_clu_glob > buffer->global_cluster_number_shfit_[cur_tet + 1]){
				cur_tet ++;
				buffer->cluster_firing_rates_[cur_tet].resize(50);
			}

			frates >> frate;

			buffer->cluster_firing_rates_[cur_tet][cur_clu_glob - buffer->global_cluster_number_shfit_[cur_tet]] = frate;
			cur_clu_glob ++;
		}

		// DEBUG
		for (unsigned int t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
			for (unsigned int c=0; c < 20; ++c){
				std::cout << "Firing rate for cluster " << c << " at tetrode " << t << " = " << buffer->cluster_firing_rates_[t][c] << "\n";
			}
		}
	} else {
		Log("WARNING: FR FILE NOT FOUND !");
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

	if (buffer->clu_reset_){
		current_session_ = 0;
		buffer->spike_buf_pos_clust_ = 0;
		res = 0;

		clu_stream_.close();
		res_stream_.close();
		clu_stream_.open(buffer->config_->spike_files_[0] + "clu");
		res_stream_.open(buffer->config_->spike_files_[0] + "res");

		Log("RE-LOADING CLU...");

		cluster_shifts_.clear();
		std::ifstream cluster_number_shifts_stream_(buffer->config_->spike_files_[0] + std::string("cluster_shifts"));
		unsigned i=0;
		while(!cluster_number_shifts_stream_.eof()){
			unsigned int shift;
			cluster_number_shifts_stream_ >> shift;

			if (cluster_number_shifts_stream_.eof())
				break;

			cluster_shifts_.push_back(shift);

			buffer->global_cluster_number_shfit_[i] = shift;

			if (i > 0){
				buffer->clusters_in_tetrode_[i-1] = buffer->global_cluster_number_shfit_[i] - buffer->global_cluster_number_shfit_[i-1];
			}

			i++;
		}
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

		// !!!!!!! WORKAROUND CAUSED BY BUG
		// TODO FIND OUT REASON WHY THIS HAPPENS - EOF?
		if (res > buffer->session_shifts_[current_session_]){
			clu_stream_ >> clust;
			res_stream_ >> res;
			res += buffer->session_shifts_[current_session_];
		}
	}

	// for reset
	unsigned int changed_id = 0;

	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_){
		Spike * spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

		// sorry
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

			// !!!!!!! WORKAROUND CAUSED BY BUG
			// TODO FIND OUT REASON WHY THIS HAPPENS - EOF?
			if (res > buffer->session_shifts_[current_session_]){
				clu_stream_ >> clust;
				res_stream_ >> res;
				res += buffer->session_shifts_[current_session_];
			}
		}

		while(res < spike->pkg_id_ && !clu_stream_.eof()){
			clu_stream_ >> clust;
			res_stream_ >> res;
			res += buffer->session_shifts_[current_session_];
		}

		if (res == spike->pkg_id_ && (spike->tetrode_ == (int)cluster_shifts_.size()-1 || clust <= cluster_shifts_[spike->tetrode_ + 1])){
			int newid = std::max<int>(-1, clust - cluster_shifts_[spike->tetrode_]);

			if (spike->cluster_id_ != newid)
				changed_id ++;

			spike->cluster_id_ = newid;
//			buffer->UpdateWindowVector(spike);
//			buffer->cluster_spike_counts_(spike->tetrode_, spike->cluster_id_) += 1;

			// to not use same entry again for other tetrode with the same time
			// otherwise next spike from other tetrode 	should be next
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

	if (buffer->clu_reset_){
		buffer->clu_reset_ = false;
		Log("DONE CLU RESET, ID CHANGED: ", changed_id);
		// buffer->ResetAC(-1);
	}
}
