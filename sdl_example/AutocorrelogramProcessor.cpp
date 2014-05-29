//
//  AutocorrelogramProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 27/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "AutocorrelogramProcessor.h"

AutocorrelogramProcessor::AutocorrelogramProcessor(LFPBuffer *buf)
: SDLSingleWindowDisplay("Autocorrelogramms", 1000, 250)
, SDLControlInputProcessor(buf) {
    const unsigned int tetrn = buf->tetr_info_->tetrodes_number;
    
    autocorrs_.resize(tetrn);
    total_counts_.resize(tetrn);
    spike_times_buf_.resize(tetrn);
    spike_times_buf_pos_.resize(tetrn);
    reported_.resize(tetrn);
    
    for (int i=0; i < tetrn; ++i) {
        autocorrs_[i].resize(MAX_CLUST);
        total_counts_[i].resize(MAX_CLUST);
        spike_times_buf_[i].resize(MAX_CLUST);
        spike_times_buf_pos_[i].resize(MAX_CLUST);
        reported_[i].resize(MAX_CLUST);
        
        for (int c=0; c < MAX_CLUST; ++c) {
            autocorrs_[i][c].resize(NBINS);
            spike_times_buf_[i][c].resize(ST_BUF_SIZE);
        }
    }
}

void AutocorrelogramProcessor::process(){
    while(spike_buf_pos_auto_ < buffer->spike_buf_pos){
        Spike *spike = buffer->spike_buffer_[spike_buf_pos_auto_];
        
        if (spike->discarded_){
            spike_buf_pos_auto_++;
            continue;
        }
        
        if (spike->cluster_id_ == -1){
            break;
        }
        
        unsigned int tetrode = spike->tetrode_;
        unsigned int cluster_id = spike->cluster_id_;
        unsigned int stime = spike->pkg_id_;
        
        // update autocorrelation bins
        std::vector<unsigned int>& prev_spikes = spike_times_buf_[tetrode][cluster_id];
        for (unsigned int bpos = 0; bpos < ST_BUF_SIZE; ++bpos) {
            
            if(prev_spikes[bpos] == 0)
                continue;
            
            // 2 ms bins
            // TODO: configurable bitrate
            unsigned int bin = (stime - prev_spikes[bpos]) / (24 * 2 );
            if (bin >= NBINS){
                continue;
            }
            
            autocorrs_[tetrode][cluster_id][bin]++;
            total_counts_[tetrode][cluster_id]++;
        }
        
        
        // replace new spike with last in the buf and advance pointer
        prev_spikes[spike_times_buf_pos_[tetrode][cluster_id]] = stime;
        if(spike_times_buf_pos_[tetrode][cluster_id] == ST_BUF_SIZE - 1){
            spike_times_buf_pos_[tetrode][cluster_id] = 0;
        }
        else{
            spike_times_buf_pos_[tetrode][cluster_id]++;
        }
        
        spike_buf_pos_auto_++;
        
        // report
        // TODO: plot
        if (total_counts_[tetrode][cluster_id] >= NBINS * 200 && !reported_[tetrode][cluster_id]){
            std::cout << "Autocorr for cluster " << cluster_id << " at tetrode " << tetrode << "\n";
            for (int b=0; b < NBINS; ++b) {
                std::cout << autocorrs_[tetrode][cluster_id][b] << " ";
            }
            std::cout << "\n";
            reported_[tetrode][cluster_id] = true;
            
            plotAC(tetrode, cluster_id);
        }
    }
}

void AutocorrelogramProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = display_tetrode;
    
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    
    for (int c=0; c < MAX_CLUST; ++c) {
        if(reported_[display_tetrode_][c]){
            plotAC(display_tetrode_, c);
        }
    }
    
    SDL_SetRenderTarget(renderer_, NULL);
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    SDL_RenderPresent(renderer_);
}

void AutocorrelogramProcessor::process_SDL_control_input(const SDL_Event& e){
    // TODO: implement
}

// plot the autocorrelogramms function
void AutocorrelogramProcessor::plotAC(const unsigned int tetr, const unsigned int cluster){
    if (tetr != display_tetrode_)
        return;
    
    // shift for the plot
    const int xsh = (5 * NBINS + 30) * cluster + 30;
    const int ysh = 0 + 200;
    
    ColorPalette palette_ = ColorPalette::BrewerPalette12;
    
    for (int b=0; b < NBINS; ++b) {
        int height = autocorrs_[tetr][cluster][b] * NBINS / total_counts_[tetr][cluster] * 100;
        
        SDL_Rect rect;
        rect.h = height;
        rect.w = 4;
        rect.x = xsh + b * 5;
        rect.y = ysh - height;
        
        SDL_SetRenderDrawColor(renderer_, palette_.getR(cluster), palette_.getG(cluster), palette_.getB(cluster), 255);
        SDL_RenderFillRect(renderer_, &rect);
    }
}