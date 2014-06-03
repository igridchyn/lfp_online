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
    place_field_ = arma::mat(nbins, nbins);
}

void PlaceField::AddSpike(Spike *spike){
    int xb = (int)round(spike->x / bin_size_);
    int yb = (int)round(spike->y / bin_size_);
    
    for(int xba = MAX(xb-spread_, 0); xba < MIN(place_field_.n_cols, xb+spread_); ++xba){
        for (int yba = MAX(yb-spread_, 0); yba < MIN(place_field_.n_rows, yb+spread_); ++yba) {
            place_field_(yba, xba) += 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
        }
    }
}

// ============================================================================================================================================

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf)
: SDLControlInputProcessor(buf)
, SDLSingleWindowDisplay("Place Field", 800, 800){
    const unsigned int& tetrn = buf->tetr_info_->tetrodes_number;
    const unsigned int MAX_CLUST = 30;
    
    place_fields_.resize(tetrn);
    // TODO: ???
    for (size_t t=0; t < tetrn; ++t) {
        for (size_t c=0; c < MAX_CLUST; ++c) {
            place_fields_[t].push_back(PlaceField(sigma_, bin_size_, nbins_, spread_));
        }
    }
}

void PlaceFieldProcessor::process(){
    while (spike_buf_pos_ < buffer->spike_buf_pos_clust_){
        Spike *spike = buffer->spike_buffer_[spike_buf_pos_];
        
        unsigned int tetr = spike->tetrode_;
        unsigned int clust = spike->cluster_id_;
     
        place_fields_[tetr][clust].AddSpike(spike);
    }
}

void PlaceFieldProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = display_tetrode;
}

void PlaceFieldProcessor::drawPlaceField(){
    const PlaceField& pf = place_fields_[display_tetrode_][display_cluster_];
    
    // normalize by max ...
    
    for (size_t x = 0; x < pf.Width(); ++x){
        for (size_t y = 0; y < pf.Height(); ++y){
            // TODO: draw bin
        }
    }
}

void PlaceFieldProcessor::process_SDL_control_input(const SDL_Event& e){
    // TODO: implement, abstract ClusterInfoDisplay
    
    SDL_Keymod kmod = SDL_GetModState();
    
    int shift = 0;
    if (kmod & KMOD_LSHIFT){
        shift = 10;
    }
    
    if( e.type == SDL_KEYDOWN )
    {
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
        }
    }
}