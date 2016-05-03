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
	for (unsigned int c = 0; c < buffer->tetr_info_->channels_number(display_tetrode); ++c) {
        displayed_channels_.push_back(buffer->tetr_info_->tetrode_channels[display_tetrode][c]);
    }
}

SDLSignalDisplayProcessor::SDLSignalDisplayProcessor(LFPBuffer *buffer)
		: SDLSignalDisplayProcessor(buffer,
				buffer->config_->getString("lfpdisp.window.name"),
				buffer->config_->getInt("lfpdisp.window.width"),
				buffer->config_->getInt("lfpdisp.window.height"),
				buffer->config_->lfp_disp_channels_
				)
{}

std::string SDLSignalDisplayProcessor::name(){
	return "Signal Display";
}

SDLSignalDisplayProcessor::SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width,
		const unsigned int& window_height, std::vector<unsigned int> displayed_channels)
	: LFPProcessor(buffer)
	, SDLControlInputProcessor(buffer)
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
    , displayed_channels_(displayed_channels)
    , current_x(0)
    , last_disp_pos(0)
	, SCREEN_HEIGHT(window_height)
	, SCREEN_WIDTH(window_width)
	, plot_hor_scale(buffer->config_->getInt("lfpdisp.horizontal.scale", 10))
	, plot_scale(buffer->config_->getFloat("lfpdisp.vertical.scale", 40))
	, SHIFT(buffer->config_->getInt("lfpdisp.shift", 3000)){

	Log("Constructor start");

    SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, window_width, SHIFT/plot_scale);
    prev_vals_ = new int[buffer->CHANNEL_NUM];

    // check if lfp display channels is a subset of read channels
    if (!buffer->tetr_info_->ContainsChannels(displayed_channels_)){
    	buffer->Log("WARNING: requested channels are absent in channels config, displaying first tetrode channels...");
    	displayed_channels_.clear();
    	for (unsigned int c = 0; c < buffer->tetr_info_->channels_number(0); ++c) {
    		displayed_channels_.push_back(buffer->tetr_info_->tetrode_channels[0][c]);
		}
    }

    Log("Constructor done");
}

void SDLSignalDisplayProcessor::process(){
    // buffer has been reinitizliedz`

	//Log("process start");
	//Log(SDL_GetError());

    if ( buffer->zero_level > 0 )
    {
        last_disp_pos = buffer->BUF_HEAD_LEN - (buffer->BUF_HEAD_LEN - buffer->zero_level) % plot_hor_scale;
    }
    
    // whether to display
    
    for (unsigned int pos = last_disp_pos + plot_hor_scale; pos < buffer->buf_pos; pos += plot_hor_scale){
        
        for (size_t chani = 0; chani<displayed_channels_.size(); ++chani) {
            int channel = displayed_channels_[chani];

            int val = transform_to_y_coord(buffer->signal_buf[channel][pos]) + 40 * chani;
            
			//Log("SetRenderTarget and draw line, last error:");
			//Log(SDL_GetError());
            SDL_SetRenderTarget(renderer_, texture_);
			//Log(SDL_GetError());
            drawLine(renderer_, current_x, prev_vals_[channel], current_x + 1, val);
			//Log("done");

            if (current_x == SCREEN_WIDTH - 1 && chani == displayed_channels_.size() - 1){
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
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
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
            SDL_SetRenderTarget(renderer_, nullptr);
            SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
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

	//Log("process done");
}


int SDLSignalDisplayProcessor::transform_to_y_coord(int voltage){
    // scale for plotting
    int val = voltage + SHIFT;
    val = val > 0 ? val / plot_scale : 1;
    val = (unsigned int)val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;
    
    return val;
}

void SDLSignalDisplayProcessor::drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2){
    SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
    SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
	//SDL_RenderDrawPoint(renderer_, x1, y1);
}

void SDLSignalDisplayProcessor::process_SDL_control_input(const SDL_Event &e){
    //User requests quit
    if( e.type == SDL_KEYDOWN )
    {
		int shift = 0;
		SDL_Keymod kmod = SDL_GetModState();

		if (kmod & KMOD_LSHIFT){
			shift = 10;
		}

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
            	buffer->processing_over_ = true;
                break;

            case SDLK_0:
				display_tetrode_ = 0 + shift;
                break;
            case SDLK_1:
				display_tetrode_ = 1 + shift;
                break;
            case SDLK_2:
				display_tetrode_ = 2 + shift;
                break;
            case SDLK_3:
				display_tetrode_ = 3 + shift;
                break;
            case SDLK_4:
				display_tetrode_ = 4 + shift;
                break;
            case SDLK_5:
				display_tetrode_ = 5 + shift;
                break;
            case SDLK_6:
				display_tetrode_ = 6;
                break;
            case SDLK_7:
				display_tetrode_ = 7;
                break;
            case SDLK_8:
				display_tetrode_ = 8;
                break;
			case SDLK_9:
				display_tetrode_ = 9;
				break;
        }

		if (display_tetrode_ < buffer->tetr_info_->tetrodes_number()){
			displayed_channels_.clear();

			for (size_t c = 0; c < buffer->tetr_info_->channels_number(display_tetrode_); ++c){
				displayed_channels_.push_back(buffer->tetr_info_->tetrode_channels[display_tetrode_][c]);
			}

			buffer->Log("Switch (LFP) to tetrode ", (int)display_tetrode_);
		}
    }
}
