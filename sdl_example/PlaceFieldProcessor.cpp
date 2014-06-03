//
//  PlaceFieldProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "math.h"

PlaceField::PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread)
: sigma_(sigma)
, bin_size_(bin_size)
, spread_(spread){
    place_field_ = arma::mat(nbins, nbins, arma::fill::zeros);
}

void PlaceField::AddSpike(Spike *spike){
    int xb = (int)round(spike->x / bin_size_);
    int yb = (int)round(spike->y / bin_size_);
    
    // TODO: ? reconstruct ?? (in previous proc)
    // unknown coord
    if (spike->x == 1023 || spike->y == 1023){
        return;
    }
    
    for(int xba = MAX(xb-spread_, 0); xba < MIN(place_field_.n_cols, xb+spread_); ++xba){
        for (int yba = MAX(yb-spread_, 0); yba < MIN(place_field_.n_rows, yb+spread_); ++yba) {
            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
            place_field_(yba, xba) += add;
        }
    }
}

// ============================================================================================================================================

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread)
: SDLControlInputProcessor(buf)
, SDLSingleWindowDisplay("Place Field", 400, 400)
, sigma_(sigma)
, bin_size_(bin_size)
, nbins_(nbins)
, spread_(spread) {
    const unsigned int& tetrn = buf->tetr_info_->tetrodes_number;
    const unsigned int MAX_CLUST = 30;
    
    place_fields_.resize(tetrn);
    // TODO: ???
    for (size_t t=0; t < tetrn; ++t) {
        for (size_t c=0; c < MAX_CLUST; ++c) {
            place_fields_[t].push_back(PlaceField(sigma_, bin_size_, nbins_, spread_));
        }
    }
    
    palette_ = ColorPalette::MatlabJet256;
    spike_buf_pos_ = buffer->SPIKE_BUF_HEAD_LEN;
    
    occupancy_ = arma::mat(nbins, nbins, arma::fill::zeros);
}

void PlaceFieldProcessor::AddPos(int x, int y){
    int xb = (int)round(x / bin_size_);
    int yb = (int)round(y / bin_size_);
    
    // TODO: ? reconstruct ?? (in previous proc)
    // unknown coord
    if (x == 1023 || y == 1023){
        return;
    }
    
    for(int xba = MAX(xb-spread_, 0); xba < MIN(occupancy_.n_cols, xb+spread_); ++xba){
        for (int yba = MAX(yb-spread_, 0); yba < MIN(occupancy_.n_rows, yb+spread_); ++yba) {
            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((x - bin_size_*(0.5 + xba)), 2) + pow((y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
            occupancy_(yba, xba) += add;
        }
    }
}

void PlaceFieldProcessor::process(){
    while (spike_buf_pos_ < buffer->spike_buf_pos_clust_){
        Spike *spike = buffer->spike_buffer_[spike_buf_pos_];
        
        if(spike->discarded_){
            spike_buf_pos_++;
            continue;
        }
        
        if (spike->cluster_id_ == -1){
            break;
        }
        
        unsigned int tetr = spike->tetrode_;
        unsigned int clust = spike->cluster_id_;
     
        place_fields_[tetr][clust].AddSpike(spike);
        
        spike_buf_pos_++;
    }
    
    while(pos_buf_pos_ < buffer->pos_buf_pos_){
        // TODO: use noth LEDs to compute coord (in upstream processor) + speed estimate
        AddPos(buffer->positions_buf_[pos_buf_pos_][0], buffer->positions_buf_[pos_buf_pos_][1]);
        pos_buf_pos_ ++;
    }
}

void PlaceFieldProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = display_tetrode;
    drawPlaceField();
}

void PlaceFieldProcessor::drawPlaceField(){
    const PlaceField& pf = place_fields_[display_tetrode_][display_cluster_];
    
    const unsigned int binw = window_width_ / nbins_;
    const unsigned int binh = window_height_ / nbins_;
    
    // normalize by max ...
    double max_val = 0;
    for (unsigned int c = 0; c < pf.Width(); ++c){
        for (unsigned int r = 0; r < pf.Height(); ++r){
            double val = pf(r, c) / occupancy_(r, c);
            if (val > max_val)
                max_val = val;
        }
    }
    
    std::cout << "Peak firing rate (cluster " << display_cluster_ << ") = " << max_val << "\n";
    
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    
    for (unsigned int c = 0; c < pf.Width(); ++c){
        for (unsigned int r = 0; r < pf.Height(); ++r){
            unsigned int x = c * binw;
            unsigned int y = r * binh;
            
            unsigned int order = MIN(pf(r, c) / occupancy_(r, c) * palette_.NumColors() / max_val, palette_.NumColors() - 1);
            
            FillRect(x, y, order, binw, binh);
        }
    }
    
    SDL_SetRenderTarget(renderer_, NULL);
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    SDL_RenderPresent(renderer_);
}

void PlaceFieldProcessor::process_SDL_control_input(const SDL_Event& e){
    // TODO: implement, abstract ClusterInfoDisplay
    
    SDL_Keymod kmod = SDL_GetModState();
    
    int shift = 0;
    if (kmod & KMOD_LSHIFT){
        shift = 10;
    }
    
    bool need_redraw_ = false;
    
    if( e.type == SDL_KEYDOWN )
    {
        need_redraw_ = true;
        
        switch( e.key.keysym.sym )
        {
            case SDLK_1:
                display_cluster_ = 1 + shift;
                break;
            case SDLK_2:
                display_cluster_ = 2 + shift;
                break;
            case SDLK_3:
                display_cluster_ = 3 + shift;
                break;
            case SDLK_4:
                display_cluster_ = 4 + shift;
                break;
            case SDLK_5:
                display_cluster_ = 5 + shift;
                break;
            case SDLK_6:
                display_cluster_ = 6;
                break;
            case SDLK_7:
                display_cluster_ = 7;
                break;
            case SDLK_8:
                display_cluster_ = 8;
                break;
            case SDLK_9:
                display_cluster_ = 9;
                break;
            case SDLK_0:
                display_cluster_ = 0 + shift;
                break;
            default:
                need_redraw_ = false;
                break;
        }
    }
    
    if (need_redraw_){
        
        drawPlaceField();
    }
}