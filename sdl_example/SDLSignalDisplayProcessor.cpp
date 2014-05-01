//
//  SDLSignalDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 16/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"

SDLSignalDisplayProcessor::SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, int target_channel)
    : LFPProcessor(buffer)
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
    , target_channel_(target_channel)
    , current_x(0)
    , last_disp_pos(0){
    SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, window_width, SHIFT/plot_scale);
}

void SDLSignalDisplayProcessor::process(){
    // buffer has been reinitizlied
    if ( buffer->zero_level > 0 )
    {
        last_disp_pos = buffer->BUF_HEAD_LEN - (buffer->BUF_HEAD_LEN - buffer->zero_level) % plot_hor_scale;
    }
    
    // whether to display
    for (int pos = last_disp_pos + plot_hor_scale; pos < buffer->buf_pos; pos += plot_hor_scale){
        int val = transform_to_y_coord(buffer->signal_buf[target_channel_][pos]);
        
        SDL_SetRenderTarget(renderer_, texture_);
        drawLine(renderer_, current_x, val_prev, current_x + 1, val);
        
        current_x++;
        
        if (current_x == SCREEN_WIDTH - 1){
            current_x = 1;
            
            // reset screen
            SDL_SetRenderTarget(renderer_, texture_);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
            SDL_RenderPresent(renderer_);
        }
        
        last_disp_pos = pos;
        val_prev = val;
    }
    
    // whether to render
    if (unrendered > DISP_FREQ * plot_hor_scale){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
        
        unrendered = 0;
    }
    else{
        unrendered += buffer->buf_pos - last_disp_pos;
    }
    
    return;
    
    if (!(buffer->buf_pos % plot_hor_scale)){
        
        // whether to render
        if (!(buffer->buf_pos % DISP_FREQ * plot_hor_scale)){
            SDL_SetRenderTarget(renderer_, NULL);
            SDL_RenderCopy(renderer_, texture_, NULL, NULL);
            SDL_RenderPresent(renderer_);
        }
        
        if (current_x == SCREEN_WIDTH - 1){
            current_x = 1;
            
            // reset screen
            SDL_SetRenderTarget(renderer_, texture_);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
            SDL_RenderPresent(renderer_);
        }
    }
}


int SDLSignalDisplayProcessor::transform_to_y_coord(int voltage){
    // scale for plotting
    int val = voltage + SHIFT;
    val = val > 0 ? val / plot_scale : 1;
    val = val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;
    
    return val;
}

void SDLSignalDisplayProcessor::drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2){
    SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
    SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
}

void SDLSignalDisplayProcessor::process_SDL_control_input(const SDL_Event &e){
    //User requests quit
    if( e.type == SDL_KEYDOWN )
    {
        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
            case SDLK_UP:
                plot_scale += 5;
                break;
            case SDLK_DOWN:
                plot_scale = plot_scale > 5  ? plot_scale - 5 : 5;
                break;
                
            case SDLK_RIGHT:
                plot_hor_scale += 2;
                break;
            case SDLK_LEFT:
                plot_hor_scale = plot_hor_scale > 2  ? plot_hor_scale - 2 : 2;
                break;
                
            case SDLK_ESCAPE:
                exit(0);
                break;
            case SDLK_1:
                target_channel_ = 1;
                break;
            case SDLK_2:
                target_channel_ = 5;
                break;
            case SDLK_3:
                target_channel_ = 9;
                break;
            case SDLK_4:
                target_channel_ = 13;
                break;
            case SDLK_5:
                target_channel_ = 17;
                break;
        }
    }
}
