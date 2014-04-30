//
//  PositionDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PositionDisplayProcessor.h"

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
    }
    
    if (render){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }
}