//
//  SDLWaveshapeDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 12/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"

int transform(int smpl, int chan){
    return 100 + smpl/20 + 200 * chan;
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
    SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
    
    while(buf_pointer_ < buffer->spike_buf_no_rec){
        Spike *spike = buffer->spike_buffer_[buf_pointer_];
        
        // !!! PLOTTING EVERY N-th spike
        if (spike->tetrode_ != targ_tetrode_ || spike->discarded_ || !(spike->pkg_id_ % 5)){
            buf_pointer_++;
            continue;
        }
        
        for (int chan=0; chan < 4; ++chan) {
            int prev_smpl = transform(spike->waveshape[chan][0], chan);
            
            for (int smpl=1; smpl < 128; ++smpl) {
                int tsmpl = transform(spike->waveshape[chan][smpl], chan);
                
                SDL_RenderDrawLine(renderer_, smpl*4, prev_smpl, (smpl + 1) * 4, tsmpl);
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