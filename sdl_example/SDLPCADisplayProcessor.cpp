//
//  SDLPCADisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "SDLPCADisplayProcessor.h"

SDLPCADisplayProcessor::SDLPCADisplayProcessor(LFPBuffer *buffer)
:SDLPCADisplayProcessor(buffer,
		buffer->config_->getString("pcadisp.window.name"),
		buffer->config_->getInt("pcadisp.window.width"),
		buffer->config_->getInt("pcadisp.window.height"),
		buffer->config_->getInt("pcadisp.tetrode"),
		buffer->config_->getBool("pcadisp.display.unclassified"),
		buffer->config_->getFloat("pcadisp.scale"),
		buffer->config_->getInt("pcadisp.shift")
		){}

SDLPCADisplayProcessor::SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width,
		const unsigned int window_height, int target_tetrode, bool display_unclassified, const float& scale, const int shift)
: SDLControlInputProcessor(buffer)
, SDLSingleWindowDisplay(window_name, window_width, window_height)
// paired qualitative brewer palette
, palette_(12, new int[12]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928})
, target_tetrode_(target_tetrode)
, display_unclassified_(display_unclassified)
, scale_(scale)
, shift_(shift)
, time_end_(buffer->SAMPLING_RATE * 60)
{
    nchan_ = buffer->tetr_info_->channels_numbers[target_tetrode];
}

void SDLPCADisplayProcessor::process(){
    // TODO: parametrize displayed channels and pc numbers
    
    const int rend_freq = 5;
    bool render = false;
    
    while (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_disp_pca];
        // wait until cluster is assigned
        
        if (spike->tetrode_ != target_tetrode_){
            buffer->spike_buf_no_disp_pca++;
            continue;
        }
        
        if (spike->pc == NULL || (spike->cluster_id_ == -1 && !display_unclassified_))
        {
            if (spike->discarded_){
                buffer->spike_buf_no_disp_pca++;
                continue;
            }
            else{
                break;
            }
        }

        int x;
        int y;

        x = spike->pc[comp1_ % nchan_][comp1_ / nchan_]/scale_ + shift_;
        y = spike->pc[comp2_ % nchan_][comp2_ / nchan_]/scale_ + shift_;

        // time
        // TODO: check numeration
        if (comp1_ == 15){
        	x = (spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_;
        }
        if (comp2_ == 15){
        	y = (spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_;
        }

        const unsigned int cid = spike->cluster_id_ > -1 ? spike->cluster_id_ : 0;
        
        SDL_SetRenderTarget(renderer_, texture_);
		std::string sdlerror(SDL_GetError());
        //SDL_SetRenderDrawColor(renderer_, 255,255,255*((int)spike->cluster_id_/2),255);
        SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid),255);
        SDL_RenderDrawPoint(renderer_, x, y);
        
        buffer->spike_buf_no_disp_pca++;
        
        if (!(buffer->spike_buf_no_disp_pca % rend_freq))
            render = true;
    }
    
    if (render){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }
}

void SDLPCADisplayProcessor::process_SDL_control_input(const SDL_Event& e){
    if( e.type == SDL_KEYDOWN )
    {
        bool need_redraw = true;
        
        SDL_Keymod kmod = SDL_GetModState();
        
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift = 10;
        }
        
        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
            case SDLK_ESCAPE:
                exit(0);
                break;
            case SDLK_1:
                comp1_ = 1 + shift;
                break;
            case SDLK_2:
                comp1_ = 2 + shift;
                break;
            case SDLK_3:
                comp1_ = 3 + shift;
                break;
            case SDLK_4:
                comp1_ = 4 + shift;
                break;
            case SDLK_5:
                comp1_ = 5 + shift;
                break;
            case SDLK_6:
                comp1_ = 6;
                break;
            case SDLK_7:
                comp1_ = 7;
                break;
            case SDLK_8:
                comp1_ = 8;
                break;
            case SDLK_9:
                comp1_ = 9;
                break;
            case SDLK_0:
                comp1_ = 0 + shift;
                break;
            case SDLK_KP_1:
                comp2_ = 1 + shift;
                break;
            case SDLK_KP_2:
                comp2_ = 2 + shift;
                break;
            case SDLK_KP_3:
                comp2_ = 3 + shift;
                break;
            case SDLK_KP_4:
                comp2_ = 4 + shift;
                break;
            case SDLK_KP_5:
                comp2_ = 5 + shift;
                break;
            case SDLK_KP_6:
                comp2_ = 6;
                break;
            case SDLK_KP_7:
                comp2_ = 7;
                break;
            case SDLK_KP_8:
                comp2_ = 8;
                break;
            case SDLK_KP_9:
                comp2_ = 9;
                break;
            case SDLK_KP_0:
                comp2_ = 0 + shift;
                break;
            default:
                need_redraw = false;
                
        }
        
        if (need_redraw){
            // TODO: case-wise
            buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN;
            
            time_start_ = buffer->spike_buffer_[buffer->SPIKE_BUF_HEAD_LEN]->pkg_id_;
            if (buffer->spike_buf_pos_unproc_ > 1 && buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1] != NULL)
            time_end_ = buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_;

            // TODO: EXTRACT
            SDL_SetRenderTarget(renderer_, texture_);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_RenderPresent(renderer_);
        }
    }
}
