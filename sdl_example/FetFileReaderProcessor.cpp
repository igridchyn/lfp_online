/*
 * FetFileReaderProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "FetFileReaderProcessor.h"

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer)
:FetFileReaderProcessor(buffer,
		buffer->config_->getString("dat.path.base") + "fet.",
		buffer->config_->tetrodes,
		buffer->config_->getInt("fet.file.reader.window", 2000)
		){

}

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer, const std::string fet_path_base, const std::vector<int>& tetrode_numbers, const unsigned int window_size)
: LFPProcessor(buffer)
, fet_path_base_(fet_path_base)
, WINDOW_SIZE(window_size){
	// number of feature files that still have spike records
	num_files_with_spikes_ = buffer->tetr_info_->tetrodes_number;
	file_over_.resize(num_files_with_spikes_);

	int dum_ncomp;
	for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
		fet_streams_.push_back(new std::ifstream(fet_path_base_ + Utils::NUMBERS[tetrode_numbers[t]]));
		// read number of records per spike in the beginning of the file
		*(fet_streams_[t]) >> dum_ncomp;
		Spike *tspike = readSpikeFromFile(t);
		while(tspike == NULL && !file_over_[t]){
			tspike = readSpikeFromFile(t);
		}
		last_spikies_.push_back(tspike);
	}
}

FetFileReaderProcessor::~FetFileReaderProcessor() {
	for (int i = 0; i < fet_streams_.size(); ++i) {
		fet_streams_[i]->close();
	}
}

// returns NULL if spike time < 0
Spike* FetFileReaderProcessor::readSpikeFromFile(const unsigned int tetr){
	Spike *spike = new Spike(0, 0);

	spike->tetrode_ = tetr;
	spike->cluster_id_ = -1;

	const int ntetr = 4;
	const int npc = 3;

	spike->pc = new float*[ntetr];

	std::ifstream& fet_stream = *(fet_streams_[tetr]);

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

	// ??? what is the threshold
	if (stime < 400)
		return NULL;

	spike->pkg_id_ = stime >= 0 ? stime : 0;
	spike->aligned_ = true;

	if (fet_stream.eof()){
		num_files_with_spikes_ --;
		file_over_[tetr] = true;
	}

	return spike;
}

void FetFileReaderProcessor::process() {
	// TODO: sampling rate ? - how defined ? by external loop ?
	int last_spike_pkg_id = last_pkg_id_;

	// PROFILING
//	if (last_pkg_id_ > 1000000){
//		std::cout << "Exit from FetFile Reader - for profiling...\n";
//		exit(0);
//	}

	while(last_spike_pkg_id - last_pkg_id_ < WINDOW_SIZE && num_files_with_spikes_ > 0){
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
		buffer->AddSpike(last_spikies_[earliest_spike_tetrode_]);
		last_spike_pkg_id = earliest_spike_time_;

		// advance with the corresponding file reading and check for the end of file [change flag + cache = number of available files]
		Spike *spike = readSpikeFromFile(earliest_spike_tetrode_);
		last_spikies_[earliest_spike_tetrode_] = spike;

		// set coords
		// find position
		// !!! TODO: interpolate, wait for next if needed [separate processor ?]
		while(buffer->positions_buf_[buffer->pos_buf_spike_pos_][4] < spike->pkg_id_ && buffer->pos_buf_spike_pos_ < buffer->pos_buf_pos_){
			buffer->pos_buf_spike_pos_++;
		}
		// TODO: average of two LED coords
		if (buffer->pos_buf_spike_pos_ > 0){
			spike->x = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1][0];
			spike->y = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1][1];
		}

		// TODO: buffer rewind
	}

	// for next processor - clustering
	buffer->spike_buf_pos_unproc_ = buffer->spike_buf_pos;
	last_pkg_id_ = last_spike_pkg_id;

	// TODO: check for conflict with other processors
	buffer->last_pkg_id = last_pkg_id_;

	buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id);
}
