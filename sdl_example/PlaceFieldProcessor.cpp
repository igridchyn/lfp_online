//
//  PlaceFieldProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "math.h"

#include "LFPProcessor.h"
#include "PlaceFieldProcessor.h"

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
    
    place_field_(yb, xb) += 1;
    
    // normalizer - sum of all values to be added
    // TODO: cache vals
    // -- SMOOTHING RAW MAPS BY REQUEST
//    double norm = .0f;
//    for(int xba = MAX(xb-spread_, 0); xba < MIN(place_field_.n_cols, xb+spread_); ++xba){
//        for (int yba = MAX(yb-spread_, 0); yba < MIN(place_field_.n_rows, yb+spread_); ++yba) {
//            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
//            norm += add;
//        }
//    }
//    
//    for(int xba = MAX(xb-spread_, 0); xba < MIN(place_field_.n_cols, xb+spread_); ++xba){
//        for (int yba = MAX(yb-spread_, 0); yba < MIN(place_field_.n_rows, yb+spread_); ++yba) {
//            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
//            place_field_(yba, xba) += add / norm;
//        }
//    }
}

PlaceField PlaceField::Smooth(){
    PlaceField spf(sigma_, bin_size_, place_field_.n_cols, spread_);
    int width = place_field_.n_cols;
    int height = place_field_.n_rows;

    // get Gaussian for smoothing, summing up to 1
    arma::mat gauss(spread_*2 + 1, spread_*2 + 1, arma::fill::zeros);
    double g_sum = .0f;
    for (int dx = -spread_; dx <= spread_; ++dx){
        for (int dy = -spread_; dy <= spread_; ++dy) {
            gauss(dy+spread_, dx+spread_) = Utils::Math::Gauss2D(sigma_ / bin_size_, dx, dy);
            g_sum += gauss(dy+spread_, dx+spread_);
        }
    }
    for (int dx = -spread_; dx <= spread_; ++dx){
        for (int dy = -spread_; dy <= spread_; ++dy) {
            gauss(dy+spread_, dx+spread_) /= g_sum;
//            std::cout << gauss(dy+spread_, dx+spread_) << " ";
        }
    }

    // compute normalizers for border / corner cases
    arma::mat border_normalizers(spread_ + 1, spread_ + 1, arma::fill::zeros);
    for (int bx=0; bx <= spread_; ++bx) {
        for (int by=0; by <= spread_; ++by) {
            for (int dx = -spread_ + bx; dx <= spread_; ++dx){
                for (int dy = -spread_ + by; dy <= spread_; ++dy) {
                    border_normalizers(by, bx) += gauss(dy+spread_, dx+spread_);
                }
            }
        }
    }

    // smooth place field
    // TODO: smoothing on the edge corners
    for (int x=spread_; x < place_field_.n_cols-spread_; ++x) {
        for (int y=spread_; y < place_field_.n_rows-spread_; ++y) {
            for (int dx=-spread_; dx <= spread_; ++dx) {
                for (int dy=-spread_; dy <= spread_; ++dy) {
                    spf(y, x) += place_field_(y + dy, x + dx) * gauss(dy+spread_, dx+spread_);
                }
            }
        }
    }

    // smooth on the edges
    // !!! TODO: account for different width / height of PF
    for (int b=0; b < spread_; ++b) {
        // on all four sides
        for (int x=spread_; x < place_field_.n_cols-spread_; ++x) {
            for (int dx=-spread_; dx <= spread_; ++dx) {
                for (int dy=-spread_ + b; dy <= spread_; ++dy) {
                    spf(b, x)        += place_field_(b, x + dx) * gauss(dy+spread_, dx+spread_);
                    spf(height - 1 - b, x) += place_field_(height - 1 - b, x + dx) * gauss(dy+spread_, dx+spread_);
                    spf(x, b)        += place_field_(x + dx, b) * gauss(dx+spread_, dy+spread_);
                    spf(x, height - 1 - b) += place_field_(x + dx, height - 1 - b) * gauss(dx+spread_, dy+spread_);
                }
            }

            spf(b, x)               /= border_normalizers(b, 0);
            spf(height - 1 - b, x)  /= border_normalizers(b, 0);
            spf(x, b)               /= border_normalizers(b, 0);
            spf(x, height - 1 - b)  /= border_normalizers(b, 0);
        }
    }

    // smooth in the corners
    // bx, by - distances of center from the border
    for (int bx=0; bx <= spread_; ++bx) {
        for (int by=0; by <= spread_; ++by) {
            // in all 4
            for (int dx = -bx; dx <= spread_; ++dx) {
                for (int dy = -by; dy <= spread_; ++dy) {
                    spf(by, bx) += place_field_(by + dy, bx + dx) * gauss(dy+spread_, dx+spread_);
                    spf(height - 1 - by, bx) += place_field_(height - 1 - by - dy, bx + dx) * gauss(dy+spread_, dx+spread_);
                    spf(by, width - 1 - bx) += place_field_(by + dy, width - 1 - bx - dx) * gauss(dy+spread_, dx+spread_);
                    spf(height - 1 - by, width - 1 -bx) += place_field_(height - 1 - by - dy, width - 1 - bx - dx) * gauss(dy+spread_, dx+spread_);
                }
            }
            
            spf(by, bx) /= border_normalizers(by, bx);
            spf(height - 1 - by, bx) /= border_normalizers(by, bx);
            spf(by, width - 1 - bx) /= border_normalizers(by, bx);
            spf(height - 1 - by, width - 1 -bx) /= border_normalizers(by, bx);
        }
    }

    // std::cout << spf.place_field_ << "\n\n\n";
    
    return spf;
}


// ============================================================================================================================================

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread)
: SDLControlInputProcessor(buf)
, SDLSingleWindowDisplay("Place Field", 400, 400)
, sigma_(sigma)
, bin_size_(bin_size)
, nbins_(nbins)
, spread_(spread)
, occupancy_(sigma, bin_size, nbins, spread)
, occupancy_smoothed_(sigma, bin_size, nbins, spread){
    const unsigned int& tetrn = buf->tetr_info_->tetrodes_number;
    const unsigned int MAX_CLUST = 30;
    
    place_fields_.resize(tetrn);
    place_fields_smoothed_.resize(tetrn);
    
    // TODO: ???
    for (size_t t=0; t < tetrn; ++t) {
        for (size_t c=0; c < MAX_CLUST; ++c) {
            place_fields_[t].push_back(PlaceField(sigma_, bin_size_, nbins_, spread_));
            place_fields_smoothed_[t].push_back(PlaceField(sigma_, bin_size_, nbins_, spread_));
        }
    }
    
    palette_ = ColorPalette::MatlabJet256;
    spike_buf_pos_ = buffer->SPIKE_BUF_HEAD_LEN;
}

void PlaceFieldProcessor::AddPos(int x, int y){
    int xb = (int)round(x / bin_size_);
    int yb = (int)round(y / bin_size_);
    
    // TODO: ? reconstruct ?? (in previous proc)
    // unknown coord
    if (x == 1023 || y == 1023){
        return;
    }
    
    // normalizer
    double norm = .0f;
    for(int xba = MAX(xb-spread_, 0); xba < MIN(occupancy_.Width(), xb+spread_); ++xba){
        for (int yba = MAX(yb-spread_, 0); yba < MIN(occupancy_.Height(), yb+spread_); ++yba) {
            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((x - bin_size_*(0.5 + xba)), 2) + pow((y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
            norm += add;
        }
    }
    
    for(int xba = MAX(xb-spread_, 0); xba < MIN(occupancy_.Width(), xb+spread_); ++xba){
        for (int yba = MAX(yb-spread_, 0); yba < MIN(occupancy_.Height(), yb+spread_); ++yba) {
            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((x - bin_size_*(0.5 + xba)), 2) + pow((y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
            occupancy_(yba, xba) += add / norm;
        }
    }
}

void PlaceFieldProcessor::process(){
    while (spike_buf_pos_ < buffer->spike_buf_pos_speed_){
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
     
        if (spike->speed > SPEED_THOLD){
            place_fields_[tetr][clust].AddSpike(spike);
        }
        
        spike_buf_pos_++;
    }
    
    // TODO: configurable [in LFPProc ?]
    while(buffer->pos_buf_pos_ >= 8 && pos_buf_pos_ < buffer->pos_buf_pos_ - 8){
        // TODO: use noth LEDs to compute coord (in upstream processor) + speed estimate
        if (buffer->positions_buf_[pos_buf_pos_][5] > SPEED_THOLD){
            AddPos(buffer->positions_buf_[pos_buf_pos_][0], buffer->positions_buf_[pos_buf_pos_][1]);
        }
        pos_buf_pos_ ++;
    }
}

void PlaceFieldProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = display_tetrode;
    drawPlaceField();
}

void PlaceFieldProcessor::drawOccupancy(){
    const unsigned int binw = window_width_ / nbins_;
    const unsigned int binh = window_height_ / nbins_;
    
    double max_val = 0;
    for (unsigned int c = 0; c < occupancy_smoothed_.Width(); ++c){
        for (unsigned int r = 0; r < occupancy_smoothed_.Height(); ++r){
            double val = occupancy_smoothed_(r, c);
            if (val > max_val)
                max_val = val;
        }
    }
    
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    
    for (unsigned int c = 0; c < occupancy_smoothed_.Width(); ++c){
        for (unsigned int r = 0; r < occupancy_smoothed_.Height(); ++r){
            unsigned int x = c * binw;
            unsigned int y = r * binh;
            
            unsigned int order = MIN(occupancy_smoothed_(r, c) * palette_.NumColors() / max_val, palette_.NumColors() - 1);
            
            FillRect(x, y, order, binw, binh);
        }
    }
    
    SDL_SetRenderTarget(renderer_, NULL);
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    SDL_RenderPresent(renderer_);
}

void PlaceFieldProcessor::drawPlaceField(){
    const PlaceField& pf = place_fields_smoothed_[display_tetrode_][display_cluster_];
    
    const unsigned int binw = window_width_ / nbins_;
    const unsigned int binh = window_height_ / nbins_;
    
    // normalize by max ...
    double max_val = 0;
    for (unsigned int c = 0; c < pf.Width(); ++c){
        for (unsigned int r = 0; r < pf.Height(); ++r){
            double val = occupancy_smoothed_(r, c) > EPS ? (pf(r, c) / occupancy_smoothed_(r, c)) : 0;
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
    
    bool need_redraw = false;
    bool display_occupancy = false;
    
    if( e.type == SDL_KEYDOWN )
    {
        need_redraw = true;
        
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
            case SDLK_o:
                display_occupancy = true;
                break;
            case SDLK_s:
                smoothPlaceFields();
                break;
            default:
                need_redraw = false;
                break;
        }
    }
    
    if (need_redraw){
        if (display_occupancy){
            drawOccupancy();
        }
        else{
            drawPlaceField();
        }
    }
}


void PlaceFieldProcessor::smoothPlaceFields(){
    occupancy_smoothed_ = occupancy_.Smooth();
    
    for (int t=0; t < place_fields_.size(); ++t) {
        for (int c = 0; c < place_fields_[t].size(); ++c) {
            place_fields_smoothed_[t][c] = place_fields_[t][c].Smooth();
        }
    }
}