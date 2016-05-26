//
//  PlaceFieldProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "math.h"
#include "assert.h"
#include <fstream>
#include <memory>

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
			buf->config_->getBool("pf.load"),
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
: LFPProcessor(buf, processors_number)
, SDLControlInputProcessor(buf, processors_number)
, SDLSingleWindowDisplay("pf", buf->config_->getInt("pf.window.width"), buf->config_->getInt("pf.window.height"))
, sigma_(sigma)
, bin_size_(bin_size)
, nbinsx_(nbinsx)
, nbinsy_(nbinsy)
, spread_(spread)
, SAVE(save)
, LOAD(load)
, BASE_PATH(base_path)
, RREDICTION_FIRING_RATE_THRESHOLD(prediction_fr_thold)
, MIN_PKG_ID(min_pkg_id)
, USE_PRIOR(use_prior)
, display_prediction_(buf->config_->getBool("pf.display.prediction"))
, prediction_rate_(buf->config_->getInt("pf.prediction.rate"))
, POS_SAMPLING_RATE(buf->config_->getFloat("pos.sampling.rate", 512.0))
, MIN_OCCUPANCY(buf->config_->getFloat("pf.min.occupancy"))
, SPEED_THOLD(buf->config_->getFloat("pf.speed.threshold"))
, N_SESSIONS(buf->config_->pf_sessions_.size() + 1){
    const unsigned int tetrn = buf->tetr_info_->tetrodes_number();
    const unsigned int MAX_CLUST = 30;
    
    place_fields_.resize(tetrn);
    place_fields_smoothed_.resize(tetrn);

    occupancy_.resize(N_SESSIONS, PlaceField(sigma, bin_size, nbinsx, nbinsy, spread));
    occupancy_smoothed_.resize(N_SESSIONS, PlaceField(sigma, bin_size, nbinsx, nbinsy, spread));

    for (size_t t=0; t < tetrn; ++t) {
    	place_fields_[t].resize(MAX_CLUST);
    	place_fields_smoothed_[t].resize(MAX_CLUST);
        for (size_t c=0; c < MAX_CLUST; ++c) {
        	for (size_t s=0; s < N_SESSIONS; ++s) {
				place_fields_[t][c].push_back(PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
				place_fields_smoothed_[t][c].push_back(PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
				if (LOAD){
					place_fields_smoothed_[t][c][s].Load(BASE_PATH + Utils::Converter::int2str(t) + "_" + Utils::Converter::int2str(c) + "_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
				}
        	}
        }
    }
    
    palette_ = ColorPalette::MatlabJet256;

    // load smoothed occupancy
    if(LOAD){
    	for (size_t s=0; s < N_SESSIONS; ++s) {
    		occupancy_smoothed_[s].Load(BASE_PATH + "occ_" +  Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
    	}
    	// pos sampling rate is unknown in the beginning
    	//cachePDF();
    }

    Log("WARNING: processor assumes chronological order of spikes");
    Log("CONTROLS: g - save res / clu; s - smooth place fields (TBD before displaying); o - occupancy; RSHIFT + # - select session; # - select cluster; d - dump place fields	");

    clusters_in_tetrode_.resize(buf->tetr_info_->tetrodes_number());
    global_cluster_number_shfit_.resize(buf->tetr_info_->tetrodes_number());
}

void PlaceFieldProcessor::AddPos(float x, float y){
    unsigned int xb = (unsigned int)round(x / bin_size_ - 0.5);
    unsigned int yb = (unsigned int)round(y / bin_size_ - 0.5);
    
    // unknown coord
	if (Utils::Math::Isnan(x) || Utils::Math::Isnan(y)){
        return;
    }

    if (xb >= nbinsx_ || yb >= nbinsy_){
    	buffer->log_string_stream_ << "WARNING: overflow x/y: " << xb << ", " << yb;
    	buffer->Log();
    	return;
    }

    // TODO check correctness
    PlaceField& pf = occupancy_[current_session_];
    pf(yb, xb) += 1.f;
    
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
        
        unsigned int tetr = spike->tetrode_;
        int clust = spike->cluster_id_;

        if (clust > 0 &&  clust > (int)clusters_in_tetrode_[tetr]){
        	clusters_in_tetrode_[tetr] = clust;
        }

		if (spike->discarded_ || Utils::Math::Isnan(spike->x) || clust == -1){
        	buffer->spike_buf_pos_pf_++;
            continue;
        }
     
        // update current session number based on the spike id
        while (current_session_ <  N_SESSIONS - 1 && spike->pkg_id_ > buffer->config_->pf_sessions_[current_session_]){
        	current_session_ ++;
        	Log("Advance current session to ", current_session_);
        }

        if (spike->speed > SPEED_THOLD){
            bool spike_added = place_fields_[tetr][clust][current_session_].AddSpike(spike);
            if (!spike_added){
            	buffer->log_string_stream_ << "Spike with coordinates " << spike->x << ", " << spike->y << " not added.\n";
            	buffer->Log();
            }
        }

        buffer->spike_buf_pos_pf_++;
    }
    
    // TODO: configurable [in LFPProc ?]
    while(buffer->pos_buf_pos_ >= 8 && pos_buf_pos_ < buffer->pos_buf_pos_ - 8){
        if (buffer->positions_buf_[pos_buf_pos_].speed_ > SPEED_THOLD && buffer->positions_buf_[pos_buf_pos_].valid){
            AddPos(buffer->positions_buf_[pos_buf_pos_].x_pos(), buffer->positions_buf_[pos_buf_pos_].y_pos());
        }
        pos_buf_pos_ ++;
    }

    // if prediction display requested and at least prediction_rate_ time has passed since last prediction
    if (display_prediction_ && buffer->last_pkg_id - last_predicted_pkg_ > prediction_rate_){
    	arma::fmat pred = arma::exp(buffer->last_predictions_[processor_number_].t() / 50);
    	drawMat(pred);
    	last_predicted_pkg_ = buffer->last_pkg_id;
    }
}

//const arma::mat& PlaceFieldProcessor::GetSmoothedOccupancy() {
//	occupancy_smoothed_ = occupancy_.Smooth();
//	return occupancy_smoothed_.Mat();
//}

void PlaceFieldProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    display_tetrode_ = MIN(display_tetrode, buffer->tetr_info_->tetrodes_number() - 1);
    drawPlaceField();
}

template <class T>
void PlaceFieldProcessor::drawMat(const arma::Mat<T>& mat, const std::vector<std::string> text_output){
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

    RenderClear();

    for (unsigned int c = 0; c < mat.n_cols; ++c){
        for (unsigned int r = 0; r < mat.n_rows; ++r){
            unsigned int x = c * binw;
            unsigned int y = r * binh;

            unsigned int order = (unsigned int)MIN(mat(r, c) * palette_.NumColors() / max_val, palette_.NumColors() - 1);

            FillRect(x + binw / 2, y + binh / 2, order, binw, binh);
        }
    }

    // draw actual position tail
    unsigned int end = (unsigned int)MIN(buffer->last_preidction_window_ends_[processor_number_] / POS_SAMPLING_RATE, buffer->pos_buf_pos_);
    for (unsigned int pos = end - 100; pos < end; ++pos) {
    	FillRect(int(buffer->positions_buf_[pos].x_pos() * binw / bin_size_), int(buffer->positions_buf_[pos].y_pos() * binh / bin_size_), 0, 2, 2);
    }

    ResetTextStack();
    TextOut(Utils::Converter::Combine("Tetrode: ", (int)display_tetrode_), 0xFFFFFF, true);
    TextOut(Utils::Converter::Combine("Cluster: ", display_cluster_), 0xFFFFFF, true);
    TextOut(Utils::Converter::Combine("Cluster global: ", int(global_cluster_number_shfit_[display_tetrode_] + display_cluster_)), 0xFFFFFF, true);
    TextOut(Utils::Converter::Combine("Session: ", (int)selected_session_), 0xFFFFFF, true);
    TextOut(Utils::Converter::Combine("Peak firing rate (Hz): ", max_val * buffer->SAMPLING_RATE / POS_SAMPLING_RATE), 0xFFFFFF, false);
    TextOut(Utils::Converter::Combine(" / ", max_val * buffer->SAMPLING_RATE / POS_SAMPLING_RATE), 0x000000, true);

    for (auto const& line: text_output){
    	TextOut(line, 0xFFFFFF, true);
    }

    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

void PlaceFieldProcessor::drawOccupancy(){
    drawMat(occupancy_smoothed_[selected_session_].Mat());
}

void PlaceFieldProcessor::drawPlaceField(){
    const PlaceField& pf = place_fields_smoothed_[display_tetrode_][display_cluster_][selected_session_];
    arma::mat dv = pf.Mat() / occupancy_smoothed_[selected_session_].Mat();
    unsigned int mx = 0, my = 0;
    pf.Mat().max(mx, my);
    std::vector<std::string> lines = {Utils::Converter::Combine("# of spikes in max bin: ", place_fields_[display_tetrode_][display_cluster_][selected_session_](mx, my)), Utils::Converter::Combine("Total nuber of spikes accounted for: ", arma::sum(arma::sum(place_fields_[display_tetrode_][display_cluster_][selected_session_].Mat())))};
    drawMat(dv, lines);
}

void PlaceFieldProcessor::drawPrediction(){
    drawMat(reconstructed_position_);
}

void PlaceFieldProcessor::switchSession(const unsigned int& session){
	if (session < N_SESSIONS){
		selected_session_ = session;
		Log("Switched session to ", session);
	} else {
		Log("Requested session outside of range: ", session);
		Log("Number of available sessions: ", N_SESSIONS);
	}
}

void PlaceFieldProcessor::dumpCluAndRes(){
	calculateClusterNumberShifts();

	Log("START SAVING CLU/RES");
	Log("Global cluster number shifts: ", global_cluster_number_shfit_);

	std::ofstream *res_global, *clu_global;

	res_global = new std::ofstream(buffer->config_->getString("out.path.base") + "all.res");
	clu_global = new std::ofstream(buffer->config_->getString("out.path.base") + "all.clu");
	for (unsigned int i=0; i < buffer->spike_buf_pos; ++i){
		Spike *spike = buffer->spike_buffer_[i];
		if (spike->cluster_id_ > 0){
			*(res_global) << spike->pkg_id_ << "\n";
			*(clu_global) << global_cluster_number_shfit_[spike->tetrode_] + spike->cluster_id_ << "\n";
		}
	}

	clu_global->close();
	res_global->close();
	Log("FINISHED SAVING CLU/RES");
}

void PlaceFieldProcessor::dumpPlaceFields(){
	Log("dump place fields...");
    for (size_t t=0; t < place_fields_.size(); ++t) {
        for (size_t c = 1; c <= clusters_in_tetrode_[t]; ++c) {
        	for (size_t s = 0; s < N_SESSIONS; ++s) {
				arma::mat dv = place_fields_smoothed_[t][c][s].Mat() / occupancy_smoothed_[s].Mat();
				dv.save(BASE_PATH + Utils::Converter::int2str(c + global_cluster_number_shfit_[t]) + "_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
        	}
        }
    }
    Log("done dump place fields");
}

void PlaceFieldProcessor::process_SDL_control_input(const SDL_Event& e){
    SDL_Keymod kmod = SDL_GetModState();
    
    int shift = 0;
    if (kmod & KMOD_LSHIFT){
        shift = 10;
    }
    
    bool change_session = ((kmod & KMOD_RSHIFT) != 0);

    bool need_redraw = false;
    bool display_occupancy = false;
    
    if( e.type == SDL_KEYDOWN )
    {
        need_redraw = true;
        
        switch( e.key.keysym.sym )
        {
        	case SDLK_ESCAPE:
        		buffer->processing_over_ = true;
        		break;
            case SDLK_1:
            	if (change_session){
            		switchSession(0);
            	}
            	else
            		display_cluster_ = 1 + shift;
                break;
            case SDLK_2:
            	if (change_session)
            		switchSession(1);
            	else
            		display_cluster_ = 2 + shift;
                break;
            case SDLK_3:
            	if (change_session)
            		switchSession(2);
            	else
            		display_cluster_ = 3 + shift;
                break;
            case SDLK_4:
            	if (change_session)
            		switchSession(3);
            	else
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
            	if (change_session){
            		Log("Switch session to 0");
            		selected_session_ = 0;
            	}
            	else
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
            	// generate clu and res-files for all tetrodes
            case SDLK_g:
            	dumpCluAndRes();
    		    break;
            case SDLK_d:
            	// dump place fields
            	dumpPlaceFields();
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

void PlaceFieldProcessor::calculateClusterNumberShifts(){
    Log("Clusters per tetrodes:");
    unsigned int total_clusters = 0;
    for (unsigned int t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
    	std::stringstream ss;
    	ss << "Clusters in tetrode " << t << " : " << clusters_in_tetrode_[t];
    	Log(ss.str());

    	global_cluster_number_shfit_[t] = total_clusters;
    	total_clusters += clusters_in_tetrode_[t];
    }

    Log("Total clusters: ", total_clusters);
}

void PlaceFieldProcessor::smoothPlaceFields(){
	Log("Smooth place fields");
	Log("Minimal occupancy: ", MIN_OCCUPANCY);

	for (size_t s = 0; s < N_SESSIONS; ++s) {
		occupancy_smoothed_[s] = occupancy_[s].Smooth();
	}

    std::vector<unsigned int> less_than_min;
    less_than_min.resize(N_SESSIONS);
    for (unsigned int x=0; x < nbinsx_; ++x)
    	for (unsigned int y =0; y < nbinsy_; ++y)
    		for (size_t s = 0; s < N_SESSIONS; ++s) {
				if (occupancy_smoothed_[s](y, x) < MIN_OCCUPANCY){
					occupancy_smoothed_[s](y, x) = .0f;
					less_than_min[s] ++;
				}
    		}

    std::stringstream ss;
    for (size_t s = 0; s < N_SESSIONS; ++s) {
    	ss << "Bins with less than minimal occupancy in session " << s << ": " << less_than_min[s] << " out of " << nbinsx_ * nbinsy_ << "\n";
    }
    Log(ss.str());

    if (SAVE){
    	for (size_t s = 0; s < N_SESSIONS; ++s) {
    		occupancy_smoothed_[s].Mat().save(BASE_PATH + "occ_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
    	}
    }
    
    for (size_t t=0; t < place_fields_.size(); ++t) {
        for (size_t c = 0; c < place_fields_[t].size(); ++c) {
        	for (size_t s = 0; s < N_SESSIONS; ++s) {
				place_fields_smoothed_[t][c][s] = place_fields_[t][c][s].Smooth();

				if (SAVE){
					place_fields_smoothed_[t][c][s].Mat().save(BASE_PATH + Utils::Converter::int2str(t) + "_" + Utils::Converter::int2str(c) + "_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
				}
        	}
        }
    }

    Log("Done smoothing place fields");
    calculateClusterNumberShifts();
}

void PlaceFieldProcessor::cachePDF(){
	// !!! dividing spike counts by occupancy will give the FR for window 1/pos_sampling_rate (s)
	// we need FR for window POP_VEC_WIN_LEN (ms), thus factor is POP_VEC_WIN_LEN  * pos_sampling_rat / 1000


	// factor - number by which the

	// TODO THIS GOT BROKE after introducing session-wise place fields
	// TODO: !!! introduce counter (in case of buffer rewind)
//	float pos_sampling_rate = buffer->pos_buf_pos_ / (float)buffer->last_pkg_id * buffer->SAMPLING_RATE;
//	float factor = buffer->POP_VEC_WIN_LEN * pos_sampling_rate / 1000.0f;
//    for (size_t t=0; t < place_fields_.size(); ++t) {
//        for (size_t c = 0; c < place_fields_[t].size(); ++c) {
//        	for (size_t s = 0; s < buffer->config_->pf_sessions_.size(); ++c) {
//        		place_fields_smoothed_[t][c][s].CachePDF(PlaceField::PDFType::Poisson, occupancy_smoothed_[s], factor);
//        	}
//        }
//    }
}

void PlaceFieldProcessor::ReconstructPosition(std::vector<std::vector<unsigned int> > pop_vec){
    reconstructed_position_.resize(occupancy_smoothed_[current_session_].Height(), occupancy_smoothed_[current_session_].Width());
	reconstructed_position_.fill(0);
    assert(pop_vec.size() == buffer->tetr_info_->tetrodes_number());
    
    // TODO: build cache in LFP buffer, configurableize
    arma::mat firing_rates(buffer->tetr_info_->tetrodes_number(), 40);
    int fr_cnt = 0;
    for (size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
		for (size_t cl = 0; cl < pop_vec[t].size(); ++cl) {
			firing_rates(t, cl) = buffer->cluster_spike_counts_(t, cl) / buffer->last_pkg_id * buffer->SAMPLING_RATE;
			if (firing_rates(t, cl) > RREDICTION_FIRING_RATE_THRESHOLD){
				fr_cnt ++;
			}
		}
    }

    // DEBUG
    if (!(buffer->last_pkg_id % 20))
    	buffer->Log("Clusters with FR > thold: ", fr_cnt);

    // normalize by sum to have probabilities
    const double occ_sum = arma::sum(arma::sum(occupancy_smoothed_[current_session_].Mat()));

    unsigned int nclust = 0;
    for (size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
        nclust += pop_vec[t].size();
    }
    
    for (size_t r=0; r < reconstructed_position_.n_rows; ++r) {
        for (size_t c=0; c < reconstructed_position_.n_cols; ++c) {
            
        	// apply occupancy threshold
        	if (occupancy_smoothed_[current_session_](r, c)/occ_sum < 0.001){
        		reconstructed_position_(r, c) = -1000000000;
        		continue;
        	}

            // estimate log-prob of being in (r,c) - for all tetrodes / clusters (under independence assumption)
            for (size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
                for (size_t cl = 0; cl < pop_vec[t].size(); ++cl) {
                	if (firing_rates(t, cl) < RREDICTION_FIRING_RATE_THRESHOLD){
                		continue;
                	}

                	int spikes = MIN(PlaceField::MAX_SPIKES - 1, pop_vec[t][cl]);
                	// TODO: choose section properly if the method is to be used
                	double logprob = place_fields_smoothed_[t][cl][selected_session_].Prob(r, c, spikes);
                    reconstructed_position_(r, c) += logprob;
                }
            }
            
            // TODO: !!! ENABLE prior probabilities (disabled for sake of debugging simplification)
            if (USE_PRIOR){
            	reconstructed_position_(r, c) += occupancy_smoothed_[current_session_](r, c) > 0 ? (fr_cnt * log(occupancy_smoothed_[current_session_](r, c) / occ_sum)) : -100000.0;
            }
        }
    }
    
    pos_updated_ = true;
    
    // update ID of last predicted package to control the frequency of prediction (more important for models with memory, for memory-less just for performance)
    // TODO: use last clustered spike pkg_id ???
    last_predicted_pkg_ = buffer->last_pkg_id;

//    double lpmax = -reconstructed_position_.min();
    for (size_t r=0; r < reconstructed_position_.n_rows; ++r) {
            for (size_t c=0; c < reconstructed_position_.n_cols; ++c) {
            	reconstructed_position_(r, c) = exp(reconstructed_position_(r, c)/100);
//            	reconstructed_position_(r, c) = lpmax + reconstructed_position_(r, c);
            }
    }

    //  find the coordinates (and write to pos?)
    unsigned int rmax, cmax;
    reconstructed_position_.max(rmax, cmax);

	buffer->positions_buf_[buffer->pos_buf_pos_].Init(float((cmax + 0.5) * bin_size_ + (rand() - RAND_MAX / 2) * (bin_size_) / 2 / RAND_MAX),
    		float((rmax + 0.5) * bin_size_ + (rand() - RAND_MAX /2.f) * (bin_size_) / 2.f / RAND_MAX),
			float((cmax + 0.5) * bin_size_ + (rand() - RAND_MAX / 2.f) * (bin_size_) / 2.f / RAND_MAX),
			float((rmax + 0.5) * bin_size_ + (rand() - RAND_MAX / 2.f) * (bin_size_) / 2.f / RAND_MAX));
}
