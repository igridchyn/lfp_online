//
//  SpikeDetectorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Jozsef Csicsvari, Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"
#include "SpikeDetectorProcessor.h"

//#define SSE2
//
//#ifdef SSE2
//#include <pmmintrin.h>
//#include <xmmintrin.h>
//#endif

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer)
:SpikeDetectorProcessor(buffer,
		buffer->config_->getString("spike.detection.filter.path").c_str(),
		buffer->config_->getFloat("spike.detection.nstd"),
		buffer->config_->getInt("spike.detection.refractory")
		){
	filt_pos = buffer->BUF_HEAD_LEN;
	det_pos = buffer->BUF_HEAD_LEN;
}

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float nstd, const int refractory)
: LFPProcessor(buffer)
, nstd_(nstd)
, refractory_(refractory)
, min_power_samples_(buffer->config_->getInt("spike.detection.min.power.samples", 20000))
, DET_THOLD_CALC_RATE_(buffer->config_->getInt("spike.detection.thold.rate", 1))
{
    // load spike filter
    std::ifstream filter_stream;
    filter_stream.open(filter_path);
    
    int fpos = 0;
    while(!filter_stream.eof()){
        filter_stream >> filter[fpos++];
        
        filter_int_[fpos - 1] = 8192 * filter[fpos - 1];
        
        // DEBUG
        // printf("filt: %f\n", filter[fpos-1]);
    }
    filter_len = fpos - 1;
    
    filt_pos = buffer->HEADER_LEN;
    
    for (size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
        buffer->last_spike_pos_[t] = - (int)refractory_;
    }
    
    //printf("filt len: %d\n", filter_len);
    thresholds_ = new int[buffer->CHANNEL_NUM];
    memset(thresholds_, 0, sizeof(int) * buffer->CHANNEL_NUM);
}

void SpikeDetectorProcessor::process(){
	process_tetrode(-1);
}

void SpikeDetectorProcessor::process_tetrode(int tetrode_to_process)
{
    // printf("Spike detect...");
    
    // to start detection by threshold from this position after filtering + power computation
    // int det_pos = filt_pos;
    
	if (tetrode_to_process < 0){
		for (unsigned int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {

			if (!buffer->is_valid_channel(channel) || buffer->buf_pos < filter_len/2)
				continue;

			filter_channel(channel);
		}
	} else {
		for (unsigned int ci = 0; ci < buffer->tetr_info_->tetrode_channels[tetrode_to_process].size(); ++ci){
			filter_channel(buffer->tetr_info_->tetrode_channels[tetrode_to_process][ci]);
		}
	}
    
    filt_pos = buffer->buf_pos - filter_len / 2;
    
    // TODO !!! check if fine with rewinds
    if (filt_pos < buffer->HEADER_LEN){
        filt_pos = buffer->HEADER_LEN;
    }
    
    // DETECT only after enough samples for power estimation
    if (buffer->powerEstimators_[0].n_samples() < min_power_samples_){
//    	if (buffer->last_pkg_id > 250000){
//    		std::cout << "returning after 125k'\n";
//    	}
        return;
    }
    
    if (tetrode_to_process < 0){
		for (unsigned int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
			if (!buffer->is_valid_channel(channel) || (buffer->last_pkg_id % DET_THOLD_CALC_RATE_))
				continue;

		   update_threshold(channel);
		}
    } else {
    	for (unsigned int ci = 0; ci < buffer->tetr_info_->tetrode_channels[tetrode_to_process].size(); ++ci){
    		update_threshold(buffer->tetr_info_->tetrode_channels[tetrode_to_process][ci]);
    	}
    }

    if (tetrode_to_process < 0){
		for (unsigned int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
			if (!buffer->is_valid_channel(channel))
				continue;
			int threshold = thresholds_[channel];
			int tetrode = buffer->tetr_info_->tetrode_by_channel[channel];

			detect_spikes(channel, threshold, tetrode, tetrode_to_process);
		}
    } else {
    	for (unsigned int ci = 0; ci < buffer->tetr_info_->tetrode_channels[tetrode_to_process].size(); ++ci){
    		unsigned int channel = buffer->tetr_info_->tetrode_channels[tetrode_to_process][ci];
    		int threshold = thresholds_[channel];
    		int tetrode = buffer->tetr_info_->tetrode_by_channel[channel];

    		detect_spikes(channel, threshold, tetrode, tetrode_to_process);
    	}
    }
    
    det_pos = filt_pos;
}

void SpikeDetectorProcessor::desync() {
	// rewind if during async detection can go out of bounds
	// TODO !!! compute from chunk size
	if (buffer->spike_buf_pos > buffer->SPIKE_BUF_LEN - buffer->tetr_info_->tetrodes_number() * 300){
		buffer->Rewind();
	}
}

void SpikeDetectorProcessor::sync() {
}

void SpikeDetectorProcessor::filter_channel(unsigned int channel) {
	for (unsigned int fpos = filt_pos; fpos < buffer->buf_pos - filter_len/2; ++fpos) {
		// filter with high-pass spike filter
		int filtered = 0;
		signal_type *chan_sig_buf = buffer->signal_buf[channel] + fpos - filter_len/2;

		// SSE implementation with 8-bit filter and signal

		int filtered_long = 0;
		for (unsigned int j=0; j < filter_len; ++j, chan_sig_buf++) {
			filtered_long += *(chan_sig_buf) * filter_int_[j];
		}

#ifdef CHAR_SIGNAL
		// * 256 / 8192
		filtered = filtered_long >> 5;
#else
		filtered = filtered_long / 100000000.0f;
#endif

		buffer->filtered_signal_buf[channel][fpos] = filtered;

		// power in window of 4
		unsigned long long pw = 0;
		for(int i=0; i<4 ;++i){
			pw += buffer->filtered_signal_buf[channel][fpos-i] * buffer->filtered_signal_buf[channel][fpos-i];
		}
		buffer->power_buf[channel][fpos] = sqrt(pw);

		// TODO: !!! INVESTIGATE NEGITIVE THRESHOLD
		// std estimation
		if (buffer->powerEstimatorsMap_[channel] != nullptr)
			buffer->powerEstimatorsMap_[channel]->push(buffer->power_buf[channel][fpos]);

		// DEBUG
		//            if (channel == 8){
		//                printf("std estimate: %d\n", (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate());
		//            }
	}
}

void SpikeDetectorProcessor::update_threshold(unsigned int channel) {
	 thresholds_[channel] = (int)(buffer->powerEstimatorsMap_[channel]->get_std_estimate() * nstd_);

	 // TMPDEBUG
	 if (thresholds_[channel] < 0){
		 thresholds_[channel] = std::numeric_limits<int>::max();
		 Log("WARNING: THRESHOLD < 0 at channel", channel);
	 }
}

void SpikeDetectorProcessor::detect_spikes(const unsigned int & channel, const int & threshold, const int & tetrode, const int & tetrode_to_process) {
	for (unsigned int dpos = det_pos; dpos < buffer->buf_pos - filter_len/2; ++dpos) {
		// detection via threshold nstd * std
		unsigned int spike_pos = buffer->last_pkg_id - buffer->buf_pos + dpos;
		if (buffer->power_buf[channel][dpos] > threshold && spike_pos - buffer->last_spike_pos_[tetrode] >= refractory_ - 1)
		{
			// printf("Spike: %d...\n", spike_pos);
			// printf("%d ", spike_pos);

			buffer->last_spike_pos_[tetrode] = spike_pos + 1;
			Spike *spike = nullptr;
			{
				// TODO: !!! lock private and used withing AddSpike
				std::lock_guard<std::mutex> lk(spike_add_mtx_);
				spike = buffer->spike_buffer_[buffer->spike_buf_pos];
				// in parallel mode - rewind is done at desync
				buffer->AddSpike(spike, tetrode_to_process < 0);
			}

			buffer->FreeFeaturesMemory(spike);
			buffer->FreeWaveshapeMemory(spike);
			buffer->FreeFinalWaveshapeMemory(spike);
			if (!spike->extra_features_){
				buffer->AllocateExtraFeaturePointerMemory(spike);
			}
			spike->init(spike_pos + 1, tetrode);// new Spike(spike_pos + 1, tetrode);

			// DEBUG
			//                if (spike_pos + 1 > 250000){
			//                	std::cout << "PKG: " << spike_pos + 1 << "\n";
			//                }

			// DEBUG
			buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Spike detected\n");

			// DEBUG 1) not ordered; 2) multiple spikes around one pos on one tetrode (have 2 buffers?)
			//                std::cout << "Spike at tetrode " << tetrode << " at pos " << spike_pos + 1 << "\n";

			// set coords
			// find position
			// !!! TODO: interpolate, wait for next if needed [separate processor ?]
			while(buffer->positions_buf_[buffer->pos_buf_spike_pos_].pkg_id_ < spike_pos && buffer->pos_buf_spike_pos_ < buffer->pos_buf_pos_){
				buffer->pos_buf_spike_pos_++;
			}

			if (buffer->pos_buf_spike_pos_ > 0){
				spike->x = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1].x_pos();
				spike->y = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1].y_pos();
			}
			else{
				spike->x = buffer->pos_unknown_;
				spike->y = buffer->pos_unknown_;
			}
		}
	}
}
