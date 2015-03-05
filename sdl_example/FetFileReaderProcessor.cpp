/*
 * FetFileReaderProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "FetFileReaderProcessor.h"
#include <boost/filesystem.hpp>

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer)
:FetFileReaderProcessor(buffer,
		buffer->config_->getInt("spike.reader.window", 2000)
		){

}

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer, const unsigned int window_size)
: LFPProcessor(buffer)
, WINDOW_SIZE(window_size)
, read_spk_(buffer->config_->getBool("spike.reader.spk.read", false))
, read_whl_(buffer->config_->getBool("spike.reader.whl.read", false))
, binary_(buffer->config_->getBool("spike.reader.binary", false))
, report_rate_(buffer->SAMPLING_RATE * 60 * 5)
, num_files_with_spikes_(buffer->tetr_info_->tetrodes_number())
, FET_SCALING(buffer->config_->getFloat("spike.reader.fet.scaling", 5.0)){
	// number of feature files that still have spike records
	file_over_.resize(num_files_with_spikes_);

	if (buffer->config_->spike_files_.size() == 0){
		Log("ERROR: 0 spike files in the list");
		exit(LFPONLINE_ERROR_SPIKE_FILES_LIST_EMPTY);
	}

	// estimate duration of all files together
	// TODO: implement for binary as well
	unsigned long total_dur = 0;
	if (!binary_){
		for (unsigned int f=0; f < buffer->config_->spike_files_.size(); ++f){
			std::string path = buffer->config_->spike_files_[f] + "fet." + Utils::NUMBERS[buffer->config_->tetrodes[0]];
			std::ifstream ffet(path);
			int d;
			while (!ffet.eof()){
				ffet >> d;
			}
			ffet.close();
			total_dur += d;
		}

		unsigned int dur_sec = total_dur / buffer->SAMPLING_RATE;
		buffer->log_string_stream_ << "Approximate duration: " << dur_sec / 60 << " min, " << dur_sec % 60 << " sec\n";
		buffer->Log();
		buffer->input_duration_ = total_dur;
	}

	openNextFile();
}

FetFileReaderProcessor::~FetFileReaderProcessor() {
	for (unsigned int i = 0; i < fet_streams_.size(); ++i) {
		fet_streams_[i]->close();
	}
}

// returns nullptr if spike time < 0
Spike* FetFileReaderProcessor::readSpikeFromFile(const unsigned int tetr){
	Spike *spike = new Spike(0, 0);

	spike->tetrode_ = tetr;
	spike->cluster_id_ = -1;

	const int chno = buffer->tetr_info_->channels_number(tetr);

	const unsigned int fetn = buffer->feature_space_dims_[tetr];

	spike->pc = new float[fetn];
	spike->num_channels_ = chno;

	std::ifstream& fet_stream = *(fet_streams_[tetr]);

	if (!binary_){
		for (unsigned int fet=0; fet < fetn; ++fet) {
			fet_stream >> spike->pc[fet];
			spike->pc[fet] /= FET_SCALING; // default = 5.0
		}
	}
	else{
		fet_stream.read((char*)spike->pc, fetn * sizeof(float));
	}

	if (!binary_){
		fet_stream >> spike->peak_to_valley_1_;
		fet_stream >> spike->peak_to_valley_2_;
		fet_stream >> spike->intervalley_;
		fet_stream >> spike->power_;
	}
	else{
		fet_stream.read((char*) &spike->peak_to_valley_1_, sizeof(float));
		fet_stream.read((char*) &spike->peak_to_valley_2_, sizeof(float));
		fet_stream.read((char*) &spike->intervalley_, sizeof(float));
		fet_stream.read((char*) &spike->power_, sizeof(float));
	}

	spike->num_channels_ = chno;

	if (read_spk_){
		std::ifstream& spk_stream = *(spk_streams_[tetr]);
		spike->waveshape = new int*[chno];

		for (int c=0; c < chno; ++c){
			spike->waveshape[c] = new int[128];

			if (!binary_){
				for (int w=0; w < 128; ++w){
					spk_stream >> spike->waveshape[c][w];
				}
			}
			else{
				spk_stream.read((char*)spike->waveshape[c], 128 * sizeof(float));
			}
		}
	}

	int stime;
	if (!binary_){
		fet_stream >> stime;
	}
	else{
		fet_stream.read((char*)&stime, sizeof(int));
	}

	if (fet_stream.eof()){
		num_files_with_spikes_ --;
		buffer->log_string_stream_ << "Out of spikes in tetorde " << tetr << ", " << num_files_with_spikes_ << " files left\n";
		buffer->Log();
		file_over_[tetr] = true;
	}

	// ??? what is the threshold
	if (stime < 400)
		return nullptr;

	spike->pkg_id_ = (stime >= 0 ? stime : 0) + shift_;
	spike->aligned_ = true;

	return spike;
}

void FetFileReaderProcessor::openNextFile() {
	if (current_file_ < (int)buffer->config_->spike_files_.size() - 1){
		current_file_ ++;

		int num_files_ = fet_streams_.size();
		for (int i=0; i < num_files_; ++i){
			fet_streams_[i]->close();
			delete fet_streams_[i];
			if (read_spk_){
				spk_streams_[i]->close();
				delete spk_streams_[i];
			}
			if (read_whl_){
				whl_file_->close();
				delete whl_file_;
			}

			file_over_[i] = false;
		}
		fet_streams_.clear();
		spk_streams_.clear();

		num_files_with_spikes_ = buffer->tetr_info_->tetrodes_number();

		fet_path_base_ = buffer->config_->spike_files_[current_file_];
		std::vector<int>& tetrode_numbers = buffer->config_->tetrodes;

		std::string extapp = binary_ ? "b" : "";

		int dum_ncomp;
		for (size_t t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
			std::string path = fet_path_base_ + "fet" + extapp + "." + Utils::NUMBERS[tetrode_numbers[t]];
			if (!boost::filesystem::exists(path)){
				Log(std::string("ERROR: ") + path + " doesn't exist or is not available!");
				exit(LFPONLINE_ERROR_FET_FILE_DOESNT_EXIST_OR_UNAVAILABLE);
			}

			fet_streams_.push_back(new std::ifstream(path, binary_ ? std::ofstream::binary : std::ofstream::in));
			if (read_spk_){
				spk_streams_.push_back(new std::ifstream(fet_path_base_ + "spk" + extapp + "." + Utils::NUMBERS[tetrode_numbers[t]], binary_ ? std::ofstream::binary : std::ofstream::in));
			}

			if (read_whl_){
				whl_file_ = new std::ifstream(fet_path_base_ + "whl");
				int pos_first_pkg_id = -1;
				(*whl_file_) >> pos_first_pkg_id;
				buffer->pos_first_pkg_ = pos_first_pkg_id;
			}

			// read number of records per spike in the beginning of the file
			if (!binary_){
				*(fet_streams_[t]) >> dum_ncomp;
			}
			Spike *tspike = readSpikeFromFile(t);
			while(tspike == nullptr && !file_over_[t]){
				tspike = readSpikeFromFile(t);
			}
			last_spikies_.push_back(tspike);
		}

		buffer->pipeline_status_ = PIPELINE_STATUS_READ_FET;
	}
}

void FetFileReaderProcessor::process() {
	int last_spike_pkg_id = last_pkg_id_;

	if (num_files_with_spikes_ == 0){
		if (current_file_ < (int)buffer->config_->spike_files_.size() - 1){
			shifts_.push_back(last_pkg_id_);
			shift_ = last_pkg_id_;
			Log(std::string("Out of spikes in file ") +  buffer->config_->spike_files_[current_file_]);
			openNextFile();
		}
		else{
			if (buffer->pipeline_status_ != PIPELINE_STATUS_INPUT_OVER){
				std::stringstream ss;
				ss << "End of input files, file duration: " << last_pkg_id_ / buffer->SAMPLING_RATE / 60 << " min " << (last_pkg_id_ / buffer->SAMPLING_RATE) % 60 << " sec \n";
				Log(ss.str());
			}
			buffer->pipeline_status_ = PIPELINE_STATUS_INPUT_OVER;
			return;
		}
	}

	// PROFILING
//	if (last_pkg_id_ > 1000000){
//		std::cout << "Exit from FetFile Reader - for profiling...\n";
//		exit(0);
//	}

	// read pos from whl
	unsigned int last_pos_pkg_id = last_pkg_id_;
	while(read_whl_ && last_pos_pkg_id < last_pkg_id_ + WINDOW_SIZE && !whl_file_->eof()){
		SpatialInfo *pos_entry = buffer->positions_buf_ + buffer->pos_buf_pos_;
		(*whl_file_) >> pos_entry->x_small_LED_ >> pos_entry->y_small_LED_  >> pos_entry->x_big_LED_ >> pos_entry->x_big_LED_ >> pos_entry->pkg_id_;

		buffer->pos_buf_pos_ ++;
		last_pos_pkg_id = pos_entry->pkg_id_;
		// TODO !!! rewind
	}

	while(last_spike_pkg_id - last_pkg_id_ < WINDOW_SIZE && num_files_with_spikes_ > 0){
		// find the earliest spike
		int earliest_spike_tetrode_ = -1;
		unsigned int earliest_spike_time_ = std::numeric_limits<unsigned int>::max();

		for (size_t t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
			if (file_over_[t]){
				continue;
			}

			if (last_spikies_[t]->pkg_id_ < earliest_spike_time_){
				earliest_spike_tetrode_ = t;
				earliest_spike_time_ = last_spikies_[t]->pkg_id_;
			}
		}

		// add the earliest spike to the buffer and
		// UPDATE pkg_id to inuclude the shift
		buffer->AddSpike(last_spikies_[earliest_spike_tetrode_]);
		last_spike_pkg_id = earliest_spike_time_;

		// advance with the corresponding file reading and check for the end of file [change flag + cache = number of available files]
		Spike *spike = readSpikeFromFile(earliest_spike_tetrode_);
		if (spike == nullptr)
			continue;

		last_spikies_[earliest_spike_tetrode_] = spike;

		// set coords
		// find position
		// !!! TODO: interpolate, wait for next if needed [separate processor ?]
		while(buffer->positions_buf_[buffer->pos_buf_spike_pos_].pkg_id_ < spike->pkg_id_ && buffer->pos_buf_spike_pos_ < buffer->pos_buf_pos_){
			buffer->pos_buf_spike_pos_++;
		}

		if (buffer->pos_buf_spike_pos_ > 0){
			spike->x = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1].x_pos();
			spike->y = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1].y_pos();
		}

		// TODO: buffer rewind
	}

	// for next processor - clustering
	buffer->spike_buf_pos_unproc_ = buffer->spike_buf_pos;
	last_pkg_id_ = last_spike_pkg_id;

	buffer->last_pkg_id = last_pkg_id_;

	buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id);

	if (last_spike_pkg_id - last_reported_ > report_rate_){
		buffer->log_string_stream_ << "Loaded spikes for the first " << last_reported_ / report_rate_ * 5 << " minutes of recording...\n";
		buffer->Log();
		last_reported_ = last_spike_pkg_id;
	}
}
