/*
 * FetFileReaderProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "FetFileReaderProcessor.h"

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer, const std::string fet_path_base, int *tetrode_numbers)
: LFPProcessor(buffer)
, fet_path_base_(fet_path_base){
	int dum_ncomp;
	for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
		fet_streams_.push_back(new std::ifstream(fet_path_base_ + Utils::NUMBERS[tetrode_numbers[t]]));
		// read number of records per spike in the beginning of the file
		*(fet_streams_[t]) >> dum_ncomp;
	}

	// number of feature files that still have spike records
	num_files_with_spikes_ = buffer->tetr_info_->tetrodes_number;
	file_over_.resize(num_files_with_spikes_);
	last_spikies_.resize(num_files_with_spikes_);
}

FetFileReaderProcessor::~FetFileReaderProcessor() {
	// TODO Auto-generated destructor stub
}

void FetFileReaderProcessor::process() {
	// TODO: sampling rate ? - how defined ? by external loop ?
	int last_spike_pkg_id = last_pkg_id_;

	while(last_spike_pkg_id - last_pkg_id_ < SAMPLING_RATE && num_files_with_spikes_ > 0){
		// find the earliest spike
		int earliest_spike_tetrode_ = -1;
		unsigned int earliest_spike_time_ = std::numeric_limits<unsigned int>::max();

		for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
			if (file_over_[t]){
				continue;
			}

			if (last_spikies_[t]->pkg_id_ < earliest_spike_time_){
				earliest_spike_tetrode_ = t;
				earliest_spike_time_ = last_spikies_[t]->pkg_id_;
			}
		}

		// add the earliest spike to the buffer and
		buffer->spike_buffer_[buffer->spike_buf_pos++] = last_spikies_[earliest_spike_tetrode_];

		// advance with the corresponding file reading and check for the end of file [change flag + cache = number of available files]

		Spike *spike = new Spike(0, 0);
		const int ntetr = 4;
		const int npc = 3;

		spike->pc = new float*[ntetr];

		std::ifstream& fet_stream = *(fet_streams_[earliest_spike_tetrode_]);

		for (int t=0; t < ntetr; ++t) {
			spike->pc[t] = new float[npc];
			for (int pc=0; pc < npc; ++pc) {
				fet_stream >> spike->pc[t][pc];
				spike->pc[t][pc] /= 5;
			}
		}

		// TODO: read position
		int dummy;
		for (int d=0; d < 4; ++d) {
			fet_stream >> dummy;
		}

		int stime;
		fet_stream >> stime;
		spike->pkg_id_ = stime;
		spike->aligned_ = true;

		if (fet_stream.eof()){
			num_files_with_spikes_  --;
			file_over_[earliest_spike_tetrode_] = true;
		}

		// TODO: add_spike() + buffer rewind
		buffer->spike_buffer_[buffer->spike_buf_pos++] = spike;
		// for clustering
		buffer->spike_buf_pos_unproc_++;
	}

	buffer->spike_buf_pos_clust_ = buffer->spike_buf_pos;
}
