//
//  SpikeDetectorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"
#include <fstream>

void SpikeDetectorProcessor::process()
{
    // for all channels
    // TODO: parallelize
    
    // printf("Spike detect...");
    
    // to start detection by threshold from this position after filtering + power computation
    int det_pos = filt_pos;
    
    for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
        
        if (!buffer->is_valid_channel(channel))
            continue;
        
        for (int fpos = filt_pos; fpos < buffer->buf_pos - filter_len/2; ++fpos) {
            
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
            //                // printf("std estimate: %d\n", (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate());
            //            }
        }
    }
    
    //float threshold = (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate() * nstd_;
    // DEBUG
    // TODO: !!! dynamic threshold from the estimators
    int threshold = (int)(43.491423979974343 * nstd_);
    
    for (int dpos = det_pos; dpos < buffer->buf_pos - filter_len/2; ++dpos) {
        for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
            
            if (!buffer->is_valid_channel(channel))
                continue;
            
            // detection via threshold nstd * std
            int spike_pos = buffer->last_pkg_id - buffer->buf_pos + dpos;
            if (buffer->power_buf[channel][dpos] > threshold && spike_pos - buffer->last_spike_pos_ >= refractory_ - 1)
            {
                // printf("Spike: %d...\n", spike_pos);
                // printf("%d ", spike_pos);
                
                buffer->last_spike_pos_ = spike_pos + 1;
                buffer->spike_buffer_[buffer->spike_buf_pos] = new Spike(spike_pos + 1, buffer->tetr_info_->tetrode_by_channel[channel]);
                buffer->spike_buf_pos++;
                
                // check if rewind is requried
                if (buffer->spike_buf_pos == buffer->SPIKE_BUF_HEAD_LEN - 1){
                    // TODO: !!! delete the rest of spikes !
                    memcpy(buffer->spike_buffer_ + buffer->SPIKE_BUF_HEAD_LEN, buffer->spike_buffer_ + buffer->SPIKE_BUF_HEAD_LEN - 1 - buffer->SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*buffer->SPIKE_BUF_HEAD_LEN);
                    
                    buffer->spike_buf_no_rec = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_no_rec);
                    buffer->spike_buf_nows_pos = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_nows_pos);
                    buffer->spike_buf_pos_unproc_ = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_pos_unproc_);
                    buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_no_disp_pca);
                    buffer->spike_buf_pos_out = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_pos_out);
                    
                    buffer->spike_buf_pos = buffer->SPIKE_BUF_HEAD_LEN;
                }
            }
        }
    }
    
    filt_pos = buffer->buf_pos - filter_len / 2;
    
    //
    if (filt_pos < buffer->HEADER_LEN)
        filt_pos = buffer->HEADER_LEN;
}