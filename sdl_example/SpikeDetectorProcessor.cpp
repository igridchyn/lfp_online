//
//  SpikeDetectorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"
#include "SpikeDetectorProcessor.h"

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer)
:SpikeDetectorProcessor(buffer,
		buffer->config_->getString("spike.detection.filter.path").c_str(),
		buffer->config_->getFloat("spike.detection.nstd"),
		buffer->config_->getInt("spike.detection.refractory")
		){
}

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float nstd, const int refractory)
: LFPProcessor(buffer)
,last_processed_id(0)
, nstd_(nstd)
, refractory_(refractory)
, REFR_LEN(buffer->SAMPLING_RATE/1000)
{
    // load spike filter
    std::ifstream filter_stream;
    filter_stream.open(filter_path);
    
    int fpos = 0;
    while(!filter_stream.eof()){
        filter_stream >> filter[fpos++];
        
        filter_int_[fpos - 1] = 100000000 * filter[fpos - 1];
        
        // DEBUG
        // printf("filt: %f\n", filter[fpos-1]);
    }
    filter_len = fpos - 1;
    
    filt_pos = buffer->HEADER_LEN;
    
    for (int t=0; t < buffer->tetr_info_->tetrodes_number; ++t) {
        buffer->last_spike_pos_[t] = - refractory_;
    }
    
    //printf("filt len: %d\n", filter_len);
    thresholds_ = new int[64];
    memset(thresholds_, 0, sizeof(int) * 64);
}

void SpikeDetectorProcessor::process()
{
    // for all channels
    // TODO: parallelize
    
    // printf("Spike detect...");
    
    // to start detection by threshold from this position after filtering + power computation
    // int det_pos = filt_pos;
    
    
    
    for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
        
        if (!buffer->is_valid_channel(channel))
            continue;
        
        for (int fpos = filt_pos; fpos < buffer->buf_pos - filter_len/2; ++fpos) {

            // SPEEDUP ? filter every 2nd sample ?
//            if (fpos % 2 && fpos > filt_pos){
//                buffer->filtered_signal_buf[channel][fpos] = buffer->filtered_signal_buf[channel][fpos - 1];
//                continue;
//            }
            
            // filter with high-pass spike filter
            float filtered = 0;
            short *chan_sig_buf = buffer->signal_buf[channel] + fpos - filter_len/2;
            
            long long filtered_long = 0;
            for (int j=0; j < filter_len; ++j, chan_sig_buf++) {
                filtered_long += *(chan_sig_buf) * filter_int_[j];
            }
            filtered = filtered_long / 100000000.0f;
            
            buffer->filtered_signal_buf[channel][fpos] = filtered;
            
            // power in window of 4
            float pw = 0;
            for(int i=0;i<4;++i){
                pw += buffer->filtered_signal_buf[channel][fpos-i] * buffer->filtered_signal_buf[channel][fpos-i];
            }
            buffer->power_buf[channel][fpos] = sqrt(pw);
            
            // std estimation
            if (buffer->powerEstimatorsMap_[channel] != NULL)
                buffer->powerEstimatorsMap_[channel]->push((float)sqrt(pw));
            
            // DEBUG
//            if (channel == 8){
//                printf("std estimate: %d\n", (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate());
//            }
        }
    }
    
    filt_pos = buffer->buf_pos - filter_len / 2;
    
    //
    if (filt_pos < buffer->HEADER_LEN)
        filt_pos = buffer->HEADER_LEN;
    
    // DETECT only after enough samples for power estimation
    if (buffer->powerEstimators_[0].n_samples() < 200000)
        return;
    
    // TODO: do not estiamte every time
    for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
        if (!buffer->is_valid_channel(channel))
            continue;
        
        // TODO: make sure there's a single estimator for all channels of the tetrode
        thresholds_[channel] = (int)(buffer->powerEstimatorsMap_[channel]->get_std_estimate() * nstd_);
    }

    for (int dpos = det_pos; dpos < buffer->buf_pos - filter_len/2; ++dpos) {
        for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
            
            if (!buffer->is_valid_channel(channel))
                continue;
            
            int threshold = thresholds_[channel];
//            int threshold = (int)(43.491423979974343 * nstd_);
            
            int tetrode = buffer->tetr_info_->tetrode_by_channel[channel];
            
            // detection via threshold nstd * std
            int spike_pos = buffer->last_pkg_id - buffer->buf_pos + dpos;
            if (buffer->power_buf[channel][dpos] > threshold && spike_pos - buffer->last_spike_pos_[tetrode] >= refractory_ - 1)
            {
                // printf("Spike: %d...\n", spike_pos);
                // printf("%d ", spike_pos);
                
                buffer->last_spike_pos_[tetrode] = spike_pos + 1;
                // TODO: reinit
                if (buffer->spike_buffer_[buffer->spike_buf_pos] != NULL)
                    delete buffer->spike_buffer_[buffer->spike_buf_pos];
                
                // TODO: delete oold spikes or reuse the structure
                Spike *spike = new Spike(spike_pos + 1, tetrode);
                buffer->spike_buffer_[buffer->spike_buf_pos] = spike;
                buffer->spike_buf_pos++;
                
                // DEBUG 1) not ordered; 2) multiple spikes around one pos on one tetrode (have 2 buffers?)
//                std::cout << "Spike at tetrode " << tetrode << " at pos " << spike_pos + 1 << "\n";
                
                // set coords
                // find position
                // !!! TODO: interpolate, wait for next if needed [separate processor ?]
                while(buffer->positions_buf_[buffer->pos_buf_spike_pos_][4] < spike_pos && buffer->pos_buf_spike_pos_ < buffer->pos_buf_pos_){
                    buffer->pos_buf_spike_pos_++;
                }
                // TODO: average of two LED coords
                spike->x = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1][0];
                spike->y = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1][1];
                
                // check if rewind is requried
                if (buffer->spike_buf_pos == buffer->SPIKE_BUF_LEN - 1){
                    // TODO: !!! delete the rest of spikes !
                    memcpy(buffer->spike_buffer_, buffer->spike_buffer_ + buffer->spike_buf_pos - buffer->SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*buffer->SPIKE_BUF_HEAD_LEN);
                    
                    buffer->spike_buf_no_rec = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_no_rec);
                    buffer->spike_buf_nows_pos = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_nows_pos);
                    buffer->spike_buf_pos_unproc_ = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_pos_unproc_);
                    buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_no_disp_pca);
                    buffer->spike_buf_pos_out = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_pos_out);
                    
                    buffer->spike_buf_pos = buffer->SPIKE_BUF_HEAD_LEN;

                    std::cout << "Spike buffer rewind (at pos " << buffer->buf_pos <<  ")!\n";
                }
            }
        }
    }
    
    det_pos = filt_pos;
}

