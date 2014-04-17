//
//  SpikeAlignmentProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 17/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"


void SpikeAlignmentProcessor::process(){
    // try to populate unpopulated spikes -
    while (buffer->spike_buf_nows_pos < buffer->spike_buf_pos &&
           buffer->spike_buffer_[buffer->spike_buf_nows_pos]->pkg_id_ < buffer->last_pkg_id - 25 - Spike::WL_LENGTH/2){
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_nows_pos];
        int num_of_ch = buffer->tetr_info_->channels_numbers[spike->tetrode_];
        spike->waveshape = new int*[num_of_ch];
        spike->num_channels_ = num_of_ch;
        
        for (int ch=0; ch < num_of_ch; ++ch){
            // for reconstructed ws
            spike->waveshape[ch] = new int[128];
            memset(spike->waveshape[ch], 0, sizeof(int)*128);
            
            // copy signal for the channel from buffer to spike object
            //memcpy(spike->waveshape[ch], buffer->signal_buf[buffer->tetr_info_->tetrode_channels[spike->tetrode_][ch]] + buffer->buf_pos - (buffer->last_pkg_id - spike->pkg_id_) - Spike::WL_LENGTH / 2, Spike::WL_LENGTH * sizeof(int));
            
            memcpy(spike->waveshape[ch], buffer->filtered_signal_buf[buffer->tetr_info_->tetrode_channels[spike->tetrode_][ch]] + buffer->buf_pos - (buffer->last_pkg_id - spike->pkg_id_) - Spike::WL_LENGTH / 2 - 1, Spike::WL_LENGTH * sizeof(int));
            
            
            // DEBUG
            //            printf("Waveshape, %d, ch. %d: ", spike->pkg_id_, ch);
            //            for (int i=0; i<spike->WL_LENGTH;++i){
            //                printf("%d ", spike->waveshape[ch][i]);
            //            }
            //            printf("\n");
            
        }
        
        // find max value in the given range
        int max_val = spike->waveshape[0][0];
        int peak_time=0;
        int max_chan = 0;
        
        // TODO: pointers
        for(int i=0;i<spike->num_channels_;i++){
            for(int j=1;j<Spike::WL_LENGTH-1;j++)
            {
                if (spike->waveshape[i][j] < max_val){
                    max_val=spike->waveshape[i][j];
                    peak_time=j;
                    max_chan=i;
                }
            }
        }
        
        // TODO: shift buffer or create one more for aligned spikes
        if ((peak_time==1) && (spike->waveshape[max_chan][0] < max_val)){
            buffer-> spike_buf_nows_pos++;
            spike->discarded_ = true;
            continue; /* not a peak */ // - if peak in the beginning, but previous value is larger
        }
        
        if ((peak_time==(Spike::WL_LENGTH-2))&&(spike->waveshape[max_chan][Spike::WL_LENGTH-1] < max_val)){
            buffer-> spike_buf_nows_pos++;
            spike->discarded_ = true;
            continue; /* not a peak */ // - if peak in the end, but the last value is larger
        }
        
        // new peak position
        int peak_pos = spike->pkg_id_ - Spike::WL_LENGTH/2 + peak_time;
        
        // if not within refractory period from previous spike
        
        if (peak_pos - prev_spike_pos_ > 16)
    	{
            if (prev_spike_pos_ > 0){
                // printf("Aligned spike pos: %d\n", prev_spike_pos_);
                printf("%d ", prev_spike_pos_);
                
                for(int ch=0; ch < num_of_ch; ++ch){
                    memcpy(prev_spike_->waveshape[ch], buffer->filtered_signal_buf[buffer->tetr_info_->tetrode_channels[prev_spike_->tetrode_][ch]] + buffer->buf_pos - (buffer->last_pkg_id - prev_spike_pos_) - Spike::WS_LENGTH_ALIGNED/ 2 - 1, Spike::WS_LENGTH_ALIGNED * sizeof(int));
                    
                    // DEBUG
                    //                    printf("Waveshape, %d, ch. %d: ", spike->pkg_id_, ch);
                    //                    for (int i=0; i<spike->WS_LENGTH_ALIGNED;++i){
                    //                        printf("%d ", spike->waveshape[ch][i]);
                    //                    }
                    //                    printf("\n");
                }
                prev_spike_->aligned_ = true;
            }
            
            prev_spike_pos_ = peak_pos;
            prev_max_val_ = max_val;
            prev_spike_ = spike;
    	}
        // ??? otherwise - replace with current, if larger  ??? - still spiking
        else{
            if (max_val < prev_max_val_){
                prev_spike_->discarded_ = true;
                
                prev_spike_pos_ = peak_pos;
                prev_max_val_ = max_val;
                prev_spike_ = spike;
            }
            else{
                // current within refractory and with smaller amplitude -> discard
                spike->discarded_ = true;
            }
        }
        
        buffer-> spike_buf_nows_pos++;
    }
}
