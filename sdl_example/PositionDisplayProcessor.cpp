//
//  PositionDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PositionDisplayProcessor.h"

PositionDisplayProcessor::PositionDisplayProcessor(LFPBuffer *buf, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, const unsigned int& target_tetrode)
    : SDLControlInputProcessor(buf)
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
    , target_tetrode_(target_tetrode)
{
    
//    SDL_SetRenderTarget(renderer_, NULL);
//    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
//    SDL_RenderPresent(renderer_);
    display_cluster_.resize(20, false);
    
}

void PositionDisplayProcessor::process(){
    const int rend_freq = 5;
    bool render = false;
    
    while (buffer->pos_buf_disp_pos_ < buffer->pos_buf_pos_) {
        
        int x = buffer->positions_buf_[buffer->pos_buf_disp_pos_][0];
        int y = buffer->positions_buf_[buffer->pos_buf_disp_pos_][1];
        
        SDL_SetRenderTarget(renderer_, texture_);
        SDL_SetRenderDrawColor(renderer_, 100,100,100,255);
        SDL_RenderDrawPoint(renderer_, x, y);
        
        buffer->pos_buf_disp_pos_++;
        
        if (!(buffer->pos_buf_disp_pos_ % rend_freq))
            render = true;
    }
    
    // display spikes on a target tetrode
    while (buffer->spike_buf_pos_draw_xy < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_draw_xy];
        // wait until cluster is assigned
        
        if (spike->tetrode_ != target_tetrode_){
            buffer->spike_buf_pos_draw_xy++;
            continue;
        }
        
        if (spike->pc == NULL || (spike->cluster_id_ == -1) || !display_cluster_[spike->cluster_id_]) // && !display_unclassified_))
        {
            if (spike->discarded_ || (spike->cluster_id_ > -1) && !display_cluster_[spike->cluster_id_]){
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
    
    if (render){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }
}

void PositionDisplayProcessor::process_SDL_control_input(const SDL_Event& e){
    if( e.type == SDL_KEYDOWN )
    {
        bool need_reset = true;
        
        SDL_Keymod kmod = SDL_GetModState();
        
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift = 10;
        }

        switch( e.key.keysym.sym )
        {
            case SDLK_ESCAPE:
                exit(0);
                break;
            case SDLK_0:
                display_cluster_[0+shift] = !display_cluster_[0+shift];
                break;
            case SDLK_1:
                display_cluster_[1+shift] = !display_cluster_[1+shift];
                break;
            case SDLK_2:
                display_cluster_[2+shift] = !display_cluster_[2+shift];
                break;
            case SDLK_3:
                display_cluster_[3+shift] = !display_cluster_[3+shift];
                break;
            case SDLK_4:
                display_cluster_[4+shift] = !display_cluster_[4+shift];
                break;
            case SDLK_5:
                display_cluster_[5+shift] = !display_cluster_[5+shift];
                break;
            case SDLK_6:
                display_cluster_[6+shift] = !display_cluster_[6+shift];
                break;
            case SDLK_7:
                display_cluster_[7+shift] = !display_cluster_[7+shift];
                break;
            case SDLK_8:
                display_cluster_[8+shift] = !display_cluster_[8+shift];
                break;
            case SDLK_9:
                display_cluster_[9+shift] = !display_cluster_[9+shift];
                break;

            default:
                need_reset = false;
                break;
        }
        
        if (need_reset){
            buffer->spike_buf_pos_draw_xy = buffer->SPIKE_BUF_HEAD_LEN;
            buffer->pos_buf_disp_pos_ = 0;
            ReinitScreen();
        }
    }
    
}

void PositionDisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    target_tetrode_ = display_tetrode;
    buffer->spike_buf_pos_draw_xy = buffer->SPIKE_BUF_HEAD_LEN;
}