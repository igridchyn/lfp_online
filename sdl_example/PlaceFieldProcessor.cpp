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

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf, const unsigned int& processors_number)
	: PlaceFieldProcessor(buf,
			buf->config_->getFloat("pf.sigma"),
			buf->config_->getFloat("bin.size"),
			buf->config_->getInt("nbinsx"),
			buf->config_->getInt("nbinsy"),
			buf->config_->getInt("pf.spread"),
			!buf->config_->getBool("pf.save"),
			buf->config_->getBool("pf.save"),
			buf->config_->getOutPath("pf.base.path"),
			buf->config_->getFloat("pf.prediction.firing.rate.threshold"),
			buf->config_->getInt("pf.min.pkg.id"),
			buf->config_->getBool("pf.use.prior"),
			processors_number)
{
}

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbinsx, const unsigned int& nbinsy,
		const unsigned int& spread, const bool& load, const bool& save, const std::string& base_path,
		const float& prediction_fr_thold, const unsigned int& min_pkg_id, const bool& use_prior, const unsigned int& processors_number)
: SDLControlInputProcessor(buf, processors_number)
, SDLSingleWindowDisplay("pf", buf->config_->getInt("pf.window.width"), buf->config_->getInt("pf.window.height"))
, LFPProcessor(buf, processors_number)
, sigma_(sigma)
, bin_size_(bin_size)
, nbinsx_(nbinsx)
, nbinsy_(nbinsy)
, spread_(spread)
, occupancy_(sigma, bin_size, nbinsx, nbinsy, spread)
, occupancy_smoothed_(sigma, bin_size, nbinsx, nbinsy, spread)
, SAVE(save)
, LOAD(load)
, BASE_PATH(base_path)
, RREDICTION_FIRING_RATE_THRESHOLD(prediction_fr_thold)
, MIN_PKG_ID(min_pkg_id)
, USE_PRIOR(use_prior)
, display_prediction_(buf->config_->getBool("pf.display.prediction"))
, prediction_rate_(buf->config_->getInt("pf.prediction.rate")){
    const unsigned int& tetrn = buf->tetr_info_->tetrodes_number;
    const unsigned int MAX_CLUST = 30;
    
    place_fields_.resize(tetrn);
    place_fields_smoothed_.resize(tetrn);
    
    // TODO: ???
    for (size_t t=0; t < tetrn; ++t) {
        for (size_t c=0; c < MAX_CLUST; ++c) {
            place_fields_[t].push_back(PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
			place_fields_smoothed_[t].push_back(PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
			if (LOAD){
				place_fields_smoothed_[t][c].Load(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::NUMBERS[c] + ".mat", arma::raw_ascii);
			}
        }
    }
    
    palette_ = ColorPalette::MatlabJet256;

    // load smoothed occupancy
    if(LOAD){
    	occupancy_smoothed_.Load(BASE_PATH + "occ.mat", arma::raw_ascii);
    	// pos sampling rate is unknown in the beginning
    	//cachePDF();
    }
}

void PlaceFieldProcessor::AddPos(int x, int y){
    int xb = (int)round(x / bin_size_);
    int yb = (int)round(y / bin_size_);
    
    // TODO: ? reconstruct ?? (in previous proc)
    // unknown coord
    if (x == 1023 || y == 1023){
        return;
    }
 
    // WORKAROUND
    // TODO handle x/y overflow
    if (xb >= nbinsx_ || yb >= nbinsy_){
    	std::cout << "WARNING: overflow x/y: " << xb << ", " << yb << "\n";
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
	if (buffer->last_pkg_id < MIN_PKG_ID){
		buffer->spike_buf_pos_pf_ = buffer->spike_buf_pos_speed_;
		pos_buf_pos_ = buffer->pos_buf_pos_ - 8;
		return;
	}

    while (buffer->spike_buf_pos_pf_ < buffer->spike_buf_pos_speed_){
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_pf_];
        
        if(spike->discarded_){
        	buffer->spike_buf_pos_pf_++;
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
        
        buffer->spike_buf_pos_pf_++;
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
    if (display_prediction_ && buffer->last_pkg_id - last_predicted_pkg_ > prediction_rate_){
        // ReconstructPosition(buffer->population_vector_window_);
//        drawPrediction();

//    	if (!(buffer->last_preidction_window_end_ % 10000)){
    	drawMat(buffer->last_predictions_[processor_number_]);
    	//    	}

    	last_predicted_pkg_ = buffer->last_pkg_id;
    }
}

const arma::mat& PlaceFieldProcessor::GetSmoothedOccupancy() {
	occupancy_smoothed_ = occupancy_.Smooth();
	return occupancy_smoothed_.Mat();
}

void PlaceFieldProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = MIN(display_tetrode, buffer->tetr_info_->tetrodes_number - 1);
    drawPlaceField();
}

void PlaceFieldProcessor::drawMat(const arma::mat& mat){
    const unsigned int binw = window_width_ / nbinsx_;
    const unsigned int binh = window_height_ / nbinsy_;
    
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
    
    // draw actual position tail
    // TODO parametrize
    unsigned int end = MIN(buffer->last_preidction_window_ends_[processor_number_] / 512, buffer->pos_buf_pos_);
    for (int pos = end - 100; pos < end; ++pos) {
    	FillRect(buffer->positions_buf_[pos][0] * binw / bin_size_, buffer->positions_buf_[pos][1] * binh / bin_size_, 0, 2, 2);
    }

    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
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
        	case SDLK_ESCAPE:
        		exit(0);
        		break;
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
                display_cluster_ = 6 + shift;
                break;
            case SDLK_7:
                display_cluster_ = 7 + shift;
                break;
            case SDLK_8:
                display_cluster_ = 8 + shift;
                break;
            case SDLK_9:
                display_cluster_ = 9 + shift;
                break;
            case SDLK_0:
                display_cluster_ = 0 + shift;
                break;
            case SDLK_o:
                display_occupancy = true;
                break;
            case SDLK_s:
                smoothPlaceFields();
                cachePDF();
                break;
            case SDLK_c:
                cachePDF();
                break;
            case SDLK_p:
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
    if (SAVE){
    	occupancy_smoothed_.Mat().save(BASE_PATH + "occ.mat", arma::raw_ascii);
//    	occupancy_.Mat().save(BASE_PATH + "occ.mat", arma::raw_ascii);
    }
    
    for (int t=0; t < place_fields_.size(); ++t) {
        for (int c = 0; c < place_fields_[t].size(); ++c) {
            place_fields_smoothed_[t][c] = place_fields_[t][c].Smooth();

            if (SAVE){
            	place_fields_smoothed_[t][c].Mat().save(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::NUMBERS[c] + ".mat", arma::raw_ascii);
//            	place_fields_[t][c].Mat().save(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::NUMBERS[c] + ".mat", arma::raw_ascii);
            }
        }
    }
}

void PlaceFieldProcessor::cachePDF(){
	// TODO: !!! introduce counter (in case of buffer rewind)
	float pos_sampling_rate = buffer->pos_buf_pos_ / (float)buffer->last_pkg_id * buffer->SAMPLING_RATE;
	// !!! dividing spike counts by occupancy will give the FR for window 1/pos_sampling_rate (s)
	// we need FR for window POP_VEC_WIN_LEN (ms), thus factor is POP_VEC_WIN_LEN  * pos_sampling_rat / 1000
	float factor = buffer->POP_VEC_WIN_LEN * pos_sampling_rate / 1000.0f;

	// factor - number by which the

    for (int t=0; t < place_fields_.size(); ++t) {
        for (int c = 0; c < place_fields_[t].size(); ++c) {
            // TODO: configurableize occupancy factor
            place_fields_smoothed_[t][c].CachePDF(PlaceField::PDFType::Poisson, occupancy_smoothed_, factor);
        }
    }
}

void PlaceFieldProcessor::ReconstructPosition(std::vector<std::vector<unsigned int> > pop_vec){
    reconstructed_position_.resize(occupancy_smoothed_.Height(), occupancy_smoothed_.Width());
	reconstructed_position_.fill(0);
    assert(pop_vec.size() == buffer->tetr_info_->tetrodes_number);
    
    // TODO: build cache in LFP buffer, configurableize
    arma::mat firing_rates(buffer->tetr_info_->tetrodes_number, 40);
    int fr_cnt = 0;
    for (int t=0; t < buffer->tetr_info_->tetrodes_number; ++t) {
		for (int cl = 0; cl < pop_vec[t].size(); ++cl) {
			// TODO: configurableize sampling rate
			firing_rates(t, cl) = buffer->cluster_spike_counts_(t, cl) / buffer->last_pkg_id * buffer->SAMPLING_RATE;
			if (firing_rates(t, cl) > RREDICTION_FIRING_RATE_THRESHOLD){
				fr_cnt ++;
			}
		}
    }

    // DEBUG
    if (!(buffer->last_pkg_id % 20))
    	std::cout << fr_cnt << " clsuters with FR > thold\n";

    // normalize by sum to have probabilities
    const double occ_sum = arma::sum(arma::sum(occupancy_smoothed_.Mat()));

    unsigned int nclust = 0;
    for (int t=0; t < buffer->tetr_info_->tetrodes_number; ++t) {
        nclust += pop_vec[t].size();
    }
    
    for (int r=0; r < reconstructed_position_.n_rows; ++r) {
        for (int c=0; c < reconstructed_position_.n_cols; ++c) {
            
        	// apply occupancy threshold
        	if (occupancy_smoothed_(r, c)/occ_sum < 0.001){
        		reconstructed_position_(r, c) = -1000000000;
        		continue;
        	}

            // estimate log-prob of being in (r,c) - for all tetrodes / clusters (under independence assumption)
            for (int t=0; t < buffer->tetr_info_->tetrodes_number; ++t) {
                for (int cl = 0; cl < pop_vec[t].size(); ++cl) {
                	if (firing_rates(t, cl) < RREDICTION_FIRING_RATE_THRESHOLD){
                		continue;
                	}

                	int spikes = MIN(PlaceField::MAX_SPIKES - 1, pop_vec[t][cl]);
                	double logprob = place_fields_smoothed_[t][cl].Prob(r, c, spikes);
                    reconstructed_position_(r, c) += logprob;
                }
            }
            
            // TODO: log / 0
            // TODO: !!! ENABLE prior probabilities (disabled for sake of debugging simplification)
            if (USE_PRIOR){
            	reconstructed_position_(r, c) += occupancy_smoothed_(r, c) > 0 ? (fr_cnt * log(occupancy_smoothed_(r, c) / occ_sum)) : -100000.0;
            }
        }
    }
    
    pos_updated_ = true;
    
    // update ID of last predicted package to control the frequency of prediction (more important for models with memory, for memory-less just for performance)
    // TODO: use last clustered spike pkg_id ???
    last_predicted_pkg_ = buffer->last_pkg_id;

    double lpmax = -reconstructed_position_.min();
    for (int r=0; r < reconstructed_position_.n_rows; ++r) {
            for (int c=0; c < reconstructed_position_.n_cols; ++c) {
            	reconstructed_position_(r, c) = exp(reconstructed_position_(r, c)/100);
//            	reconstructed_position_(r, c) = lpmax + reconstructed_position_(r, c);
            }
    }

    //  find the coordinates (and write to pos?)
    unsigned int rmax, cmax;
    double pmax = reconstructed_position_.max(rmax, cmax);
    buffer->positions_buf_[buffer->pos_buf_pos_][2] = (cmax + 0.5) * bin_size_ + (rand() - RAND_MAX /2) * (bin_size_) / 2 / RAND_MAX;
    buffer->positions_buf_[buffer->pos_buf_pos_][3] = (rmax + 0.5) * bin_size_ + (rand() - RAND_MAX /2) * (bin_size_) / 2 / RAND_MAX;
}
