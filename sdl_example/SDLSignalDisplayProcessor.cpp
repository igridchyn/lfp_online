//
//  SDLSignalDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 16/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "SDLSignalDisplayProcessor.h"

void SDLSignalDisplayProcessor::SetDisplayTetrode(const unsigned  int& display_tetrode){
    // TODO: configurableize
    
    displayed_channels_number_ = buffer->tetr_info_->channels_numbers[display_tetrode];
    
    delete[] displayed_channels_;
    displayed_channels_ = new unsigned int[4];
    
    for (int c=0; c < displayed_channels_number_; ++c) {
        displayed_channels_[c] = buffer->tetr_info_->tetrode_channels[display_tetrode][c];
    }
}

SDLSignalDisplayProcessor::SDLSignalDisplayProcessor(LFPBuffer *buffer)
		: SDLSignalDisplayProcessor(buffer,
				buffer->config_->getString("lfpdisp.window.name"),
				buffer->config_->getInt("lfpdisp.window.width"),
				buffer->config_->getInt("lfpdisp.window.height"),
				buffer->config_->getInt("lfpdisp.channels.number"),
				buffer->config_->lfp_disp_channels_
				){}

SDLSignalDisplayProcessor::SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width,
		const unsigned int& window_height, unsigned int displayed_channels_number, unsigned int *displayed_channels)
    : SDLControlInputProcessor(buffer)
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
    , displayed_channels_number_(displayed_channels_number)
    , displayed_channels_(displayed_channels)
    , current_x(0)
    , last_disp_pos(0)
	, SCREEN_HEIGHT(window_height)
	, SCREEN_WIDTH(window_width){
        
    SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, window_width, SHIFT/plot_scale);
    prev_vals_ = new int[displayed_channels_number_];
}

void SDLSignalDisplayProcessor::process(){
    // buffer has been reinitizlied
    if ( buffer->zero_level > 0 )
    {
        last_disp_pos = buffer->BUF_HEAD_LEN - (buffer->BUF_HEAD_LEN - buffer->zero_level) % plot_hor_scale;
    }
    
    // whether to display
    
    for (int pos = last_disp_pos + plot_hor_scale; pos < buffer->buf_pos; pos += plot_hor_scale){
        
        for (int chani = 0; chani<displayed_channels_number_; ++chani) {
            int channel = displayed_channels_[chani];

            int val = transform_to_y_coord(buffer->signal_buf[channel][pos]) + 40 * chani;
            
            SDL_SetRenderTarget(renderer_, texture_);
            drawLine(renderer_, current_x, prev_vals_[channel], current_x + 1, val);

            if (current_x == SCREEN_WIDTH - 1 && chani == displayed_channels_number_ - 1){
                current_x = 1;
                
                // reset screen
                SDL_SetRenderTarget(renderer_, texture_);
                SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
                SDL_RenderClear(renderer_);
                //SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
                //SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
                SDL_RenderPresent(renderer_);
            }
            
            last_disp_pos = pos;
            prev_vals_[channel] = val;
        }
        
        current_x++;
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
            //SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
            //SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
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
                // TODO: check whether channels are available in the TetrodeInfo
            case SDLK_1:
                displayed_channels_ = new unsigned int[4]{0,1,2,3};
                break;
            case SDLK_2:
                displayed_channels_ = new unsigned int[4]{4,5,6,7};
                break;
            case SDLK_3:
                displayed_channels_ = new unsigned int[4]{8,9,10,11};
                break;
            case SDLK_4:
                displayed_channels_ = new unsigned int[4]{12,13,14,15};
                break;
            case SDLK_5:
                displayed_channels_ = new unsigned int[4]{16,17,18,19};
                break;
            case SDLK_6:
                displayed_channels_ = new unsigned int[4]{20,21,22,23};
                break;
            case SDLK_7:
                displayed_channels_ = new unsigned int[4]{24,25,26,27};
                break;
            case SDLK_8:
                displayed_channels_ = new unsigned int[4]{28,29,30,31};
                break;
            case SDLK_9:
                displayed_channels_ = new unsigned int[4]{32,33,34,35};
                break;
            case SDLK_0:
                displayed_channels_ = new unsigned int[4]{36,37,38,39};
                break;
        }
    }
}
