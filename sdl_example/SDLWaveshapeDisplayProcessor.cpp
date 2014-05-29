//
//  SDLWaveshapeDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 12/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"

int transform(int smpl, int chan){
    return 100 + smpl/25 + 200 * chan;
}

void SDLWaveshapeDisplayProcessor::process() {
    // 4 channel waveshape, shifted; colour by cluster, if available (use palette)
    
    // POINTER ??? : after rewind preserve last valid position -> will allow autonomous pointer rewind (restarting from the beginning)ßß
    //  depend on object properties (don't exceed main pointer), not on the previous pointer !!!
    // TODO: implement idea above; workaround: rewind if target pointer less than
    
    if (buf_pointer_ > buffer->spike_buf_no_rec){
        buf_pointer_ = LFPBuffer::BUF_HEAD_LEN;
    }
    
    int last_pkg_id;
    
    SDL_SetRenderTarget(renderer_, texture_);
    const ColorPalette& colpal = ColorPalette::BrewerPalette12;
    SDL_SetRenderDrawColor(renderer_, colpal.getR(disp_cluster_) ,colpal.getG(disp_cluster_), colpal.getB(disp_cluster_),255);
    
    while(buf_pointer_ < buffer->spike_buf_no_rec){
        Spike *spike = buffer->spike_buffer_[buf_pointer_];
        
        // !!! PLOTTING EVERY N-th spike
        // TODO: plot only one cluster [switch !!!]
        if (spike->tetrode_ != targ_tetrode_ || spike->cluster_id_ != disp_cluster_ || spike->discarded_ || !(tetrode_total_spikes_ % 50)){
            buf_pointer_++;
            tetrode_total_spikes_ ++;
            continue;
        }
        
        for (int chan=0; chan < 4; ++chan) {
            int prev_smpl = transform(spike->waveshape[chan][0], chan);
            
            for (int smpl=1; smpl < 128; ++smpl) {
                int tsmpl = transform(spike->waveshape[chan][smpl], chan);
                
                SDL_RenderDrawLine(renderer_, smpl*4 - 3, prev_smpl, smpl * 4 + 1, tsmpl);
                prev_smpl = tsmpl;
            }
        }
        
        last_pkg_id = spike->pkg_id_;
        buf_pointer_++;
    }
    
    if (last_pkg_id - last_disp_pkg_id_ > DISPLAY_RATE){
        last_disp_pkg_id_ = last_pkg_id;
        
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }
}

void SDLWaveshapeDisplayProcessor::process_SDL_control_input(const SDL_Event& e){
    if( e.type == SDL_KEYDOWN )
    {
        bool need_redraw = true;
        
        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
            case SDLK_ESCAPE:
                exit(0);
                break;
                
            // select tetrode
            case SDLK_0:
                targ_tetrode_ = 0;
                break;
            case SDLK_1:
                targ_tetrode_ = 1;
                break;
            case SDLK_2:
                targ_tetrode_ = 2;
                break;
            case SDLK_3:
                targ_tetrode_ = 3;
                break;
                
            // select cluster
            case SDLK_KP_0:
                disp_cluster_ = 0;
                break;
            case SDLK_KP_1:
                disp_cluster_ = 1;
                break;
            case SDLK_KP_2:
                disp_cluster_ = 2;
                break;
            case SDLK_KP_3:
                disp_cluster_ = 3;
                break;
            case SDLK_KP_4:
                disp_cluster_ = 4;
                break;
            case SDLK_KP_5:
                disp_cluster_ = 5;
                break;
            case SDLK_KP_6:
                disp_cluster_ = 6;
                break;
            case SDLK_KP_7:
                disp_cluster_ = 7;
                break;
            case SDLK_KP_8:
                disp_cluster_ = 8;
                break;
            case SDLK_KP_9:
                disp_cluster_ = 9;
                break;

            default:
                need_redraw = false;
        }
        
        if (need_redraw){
            SDL_SetRenderTarget(renderer_, texture_);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_RenderPresent(renderer_);
        }
    }
}

void SDLWaveshapeDisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    // duplicates functionality in process_SDL_control_input, but is supposed to be called in all displays simultaneously
    targ_tetrode_ = display_tetrode;
    ReinitScreen();
}