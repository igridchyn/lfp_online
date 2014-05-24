//
//  PositionDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PositionDisplayProcessor.h"

PositionDisplayProcessor::PositionDisplayProcessor(LFPBuffer *buf, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, const unsigned int& target_tetrode)
    : LFPProcessor(buf)
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
    , target_tetrode_(target_tetrode)
{
    
//    SDL_SetRenderTarget(renderer_, NULL);
//    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
//    SDL_RenderPresent(renderer_);
}

void PositionDisplayProcessor::process(){
    const int rend_freq = 5;
    bool render = false;
    
    while (buffer->pos_buf_disp_pos_ < buffer->pos_buf_pos_) {
        
        int x = buffer->positions_buf_[buffer->pos_buf_disp_pos_][0];
        int y = buffer->positions_buf_[buffer->pos_buf_disp_pos_][1];
        
        SDL_SetRenderTarget(renderer_, texture_);
        SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
        SDL_RenderDrawPoint(renderer_, x, y);
        
        buffer->pos_buf_disp_pos_++;
        
        if (!(buffer->pos_buf_disp_pos_ % rend_freq))
            render = true;
        
        // display spikes on a target tetrode
        while (buffer->spike_buf_pos_draw_xy < buffer->spike_buf_pos_unproc_) {
            Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_draw_xy];
            // wait until cluster is assigned
            
            if (spike->tetrode_ != target_tetrode_){
                buffer->spike_buf_pos_draw_xy++;
                continue;
            }
            
            if (spike->pc == NULL || (spike->cluster_id_ == -1)) // && !display_unclassified_))
            {
                if (spike->discarded_){
                    buffer->spike_buf_pos_draw_xy++;
                    continue;
                }
                else{
                    break;
                }
            }
            
            // TODO: use spike position for display
            FillRect(spike->x, spike->y, spike->cluster_id_);
            buffer->spike_buf_pos_draw_xy++;
        }

    }
    
    if (render){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }
}