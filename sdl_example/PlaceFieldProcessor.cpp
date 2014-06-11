//
//  PlaceFieldProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "math.h"
#include "assert.h"

#include "LFPProcessor.h"
#include "PlaceFieldProcessor.h"


// ============================================================================================================================================

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread)
: SDLControlInputProcessor(buf)
, SDLSingleWindowDisplay("Place Field", 420, 420)
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
 
    occupancy_(yb, xb) += 1.f;
    
    // !!! smoothed later
    // normalizer
//    double norm = .0f;
//    for(int xba = MAX(xb-spread_, 0); xba < MIN(occupancy_.Width(), xb+spread_); ++xba){
//        for (int yba = MAX(yb-spread_, 0); yba < MIN(occupancy_.Height(), yb+spread_); ++yba) {
//            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((x - bin_size_*(0.5 + xba)), 2) + pow((y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
//            norm += add;
//        }
//    }
//    
//    for(int xba = MAX(xb-spread_, 0); xba < MIN(occupancy_.Width(), xb+spread_); ++xba){
//        for (int yba = MAX(yb-spread_, 0); yba < MIN(occupancy_.Height(), yb+spread_); ++yba) {
//            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((x - bin_size_*(0.5 + xba)), 2) + pow((y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
//            occupancy_(yba, xba) += add / norm;
//        }
//    }
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

    // if prediction display requested and at least prediction_rate_ time has passed since last prediction
    if (display_prediction_ && buffer->spike_buf_pos_clust_ - last_predicted_pkg_ > prediction_rate_){
        ReconstructPosition(buffer->population_vector_window_);
        drawPrediction();
    }
}

void PlaceFieldProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = MIN(display_tetrode, buffer->tetr_info_->tetrodes_number - 1);
    drawPlaceField();
}

void PlaceFieldProcessor::drawMat(const arma::mat& mat){
    const unsigned int binw = window_width_ / nbins_;
    const unsigned int binh = window_height_ / nbins_;
    
    double max_val = 0;
    for (unsigned int c = 0; c < mat.n_cols; ++c){
        for (unsigned int r = 0; r < mat.n_rows; ++r){
            double val = mat(r, c);
            if (val > max_val && arma::is_finite(val))
                max_val = val;
        }
    }
    
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    
    for (unsigned int c = 0; c < mat.n_cols; ++c){
        for (unsigned int r = 0; r < mat.n_rows; ++r){
            unsigned int x = c * binw;
            unsigned int y = r * binh;
            
            unsigned int order = MIN(mat(r, c) * palette_.NumColors() / max_val, palette_.NumColors() - 1);
            
            FillRect(x + binw / 2, y + binh / 2, order, binw, binh);
        }
    }
    
    SDL_SetRenderTarget(renderer_, NULL);
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    SDL_RenderPresent(renderer_);
}

void PlaceFieldProcessor::drawOccupancy(){
    drawMat(occupancy_smoothed_.Mat());
}

void PlaceFieldProcessor::drawPlaceField(){
    const PlaceField& pf = place_fields_smoothed_[display_tetrode_][display_cluster_];
    arma::mat dv = pf.Mat() / occupancy_smoothed_.Mat();
    drawMat(dv);
}

void PlaceFieldProcessor::drawPrediction(){
    drawMat(reconstructed_position_);
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
            	// TODO: remove workaround
            	display_prediction_ = false;
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
            case SDLK_c:
                smoothPlaceFields();
                cachePDF();
                break;
            case SDLK_p:
                //smoothPlaceFields();
                //cachePDF();
                display_prediction_ = true;
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

void PlaceFieldProcessor::cachePDF(){
    for (int t=0; t < place_fields_.size(); ++t) {
        for (int c = 0; c < place_fields_[t].size(); ++c) {
            // TODO: configurableize occupancy factor
            place_fields_smoothed_[t][c].CachePDF(PlaceField::PDFType::Poisson, occupancy_smoothed_, 10);
        }
    }
}

void PlaceFieldProcessor::ReconstructPosition(std::vector<std::vector<unsigned int> > pop_vec){
    reconstructed_position_.resize(occupancy_smoothed_.Height(), occupancy_smoothed_.Width());
	reconstructed_position_.fill(0);
    assert(pop_vec.size() == buffer->tetr_info_->tetrodes_number);
    
    // normalize by sum to have probabilities
    const double occ_sum = arma::sum(arma::sum(occupancy_smoothed_.Mat()));

    unsigned int nclust = 0;
    for (int t=0; t < buffer->tetr_info_->tetrodes_number; ++t) {
        nclust += pop_vec[t].size();
    }
    
    for (int r=0; r < reconstructed_position_.n_rows; ++r) {
        for (int c=0; c < reconstructed_position_.n_cols; ++c) {
            
            // estimate log-prob of being in (r,c) - for all tetrodes / clusters (under independence assumption)
            for (int t=0; t < buffer->tetr_info_->tetrodes_number; ++t) {
                for (int cl = 0; cl < pop_vec[t].size(); ++cl) {
                    reconstructed_position_(r, c) += place_fields_smoothed_[t][cl].Prob(r, c, MIN(PlaceField::MAX_SPIKES - 1, pop_vec[t][cl]));
                }
            }
            
            // TODO: log / 0
            reconstructed_position_(r, c) += nclust * log(occupancy_smoothed_(r, c) / occ_sum);
        }
    }
    
    pos_updated_ = true;
    
    // update ID of last predicted package to control the frequency of prediction (more important for models with memory, for memory-less just for performance)
    last_predicted_pkg_ = buffer->spike_buf_pos_clust_;

    double lpmax = -reconstructed_position_.min();
    for (int r=0; r < reconstructed_position_.n_rows; ++r) {
            for (int c=0; c < reconstructed_position_.n_cols; ++c) {
            	//reconstructed_position_(r, c) = exp(reconstructed_position_(r, c));
            	reconstructed_position_(r, c) = lpmax + reconstructed_position_(r, c);
            }
    }
}
