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

void PlaceFieldProcessor::initArrays(){
	N_SESSIONS = buffer->config_->pf_sessions_.size() + 1;

	const unsigned int tetrn = buffer->tetr_info_->tetrodes_number();
	const unsigned int MAX_CLUST = 100;

	occupancy_.resize(N_SESSIONS, PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
	occupancy_smoothed_.resize(N_SESSIONS, PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));

	for (size_t t=0; t < tetrn; ++t) {
		place_fields_[t].resize(MAX_CLUST);
		place_fields_smoothed_[t].resize(MAX_CLUST);
		// TODO !! CLUTSERS IN TETRODE
		for (size_t c=0; c < MAX_CLUST; ++c) {
			for (size_t s=0; s < N_SESSIONS; ++s) {
				place_fields_[t][c].push_back(PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
				place_fields_smoothed_[t][c].push_back(PlaceField(sigma_, bin_size_, nbinsx_, nbinsy_, spread_));
				if (LOAD){
					place_fields_smoothed_[t][c][s].Load(BASE_PATH + Utils::Converter::int2str(c + buffer->global_cluster_number_shfit_[t]) + "_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
				}
			}
		}
	}

	// load smoothed occupancy
	if(LOAD){
		for (size_t s=0; s < N_SESSIONS; ++s) {
			occupancy_smoothed_[s].Load(BASE_PATH + "occ_" +  Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
		}
		// pos sampling rate is unknown in the beginning
		//cachePDF();
	}
}

PlaceFieldProcessor::PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbinsx, const unsigned int& nbinsy,
		const unsigned int& spread, const bool& load, const bool& save, const std::string& base_path,
		const float& prediction_fr_thold, const unsigned int& min_pkg_id, const bool& use_prior, const unsigned int& processors_number)
: LFPProcessor(buf, processors_number)
, SDLControlInputProcessor(buf, processors_number)
, SDLSingleWindowDisplay(buf, "Place Fields", buf->config_->getInt("pf.window.width"), buf->config_->getInt("pf.window.height"))
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
, N_SESSIONS(buf->config_->pf_sessions_.size() + 1)
, DISPLAY_SCALE(buf->config_->getFloat("pf.display.scale"))
{
    const unsigned int tetrn = buf->tetr_info_->tetrodes_number();

    place_fields_.resize(tetrn);
    place_fields_smoothed_.resize(tetrn);

    // WORKAROUND
    // SPECIAL CASES: 0 : opened files are 9f/2 + 9l/4 + 14post/2 + 16l/4
//    if (buffer->config_->pf_sessions_.size() == 1 && buffer->config_->pf_sessions_[0] == 0){
    wait_file_read_ = true;
//    } else {
//    	initArrays();
//    }

    palette_ = ColorPalette::MatlabJet256;
    Log("WARNING: processor assumes chronological order of spikes");
    Log("CONTROLS: "
    		"g - save res / clu"
    		"s - smooth plfshiftsace fields (TBD before displaying)"
    		"o - occupancy"
    		"RSHIFT + <NUMBER> - select session"
    		"<NUMBER> - select cluster"
    		"d - dump place fields"
    		"t - toggle text colour (black/white)"
    		"r - reset (required before bulding place fields for newly created clusters)");

    if (buffer->config_->pf_groups_.size() > 0)
    	current_session_ = buffer->config_->pf_groups_[0];
}

void PlaceFieldProcessor::AddPos(float x, float y, unsigned int time){
    unsigned int xb = (unsigned int)round(x / bin_size_ - 0.5);
    unsigned int yb = (unsigned int)round(y / bin_size_ - 0.5);
    
    // unknown coord
	if (Utils::Math::Isnan(x) || Utils::Math::Isnan(y)){
        return;
    }

    if (xb >= nbinsx_ || yb >= nbinsy_){
    	buffer->log_string_stream_ << "WARNING: overflow x/y: " << xb << ", " << yb << "\n";
    	buffer->Log();
    	return;
    }

    unsigned int session = 0;
    while (session < buffer->config_->pf_sessions_.size() && time > buffer->config_->pf_sessions_[session]){
    	session ++;
    }

    if (buffer->config_->pf_groups_.size() > 0)
    	session = buffer->config_->pf_groups_[session];

    PlaceField& pf = occupancy_[session];
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
	if (wait_file_read_){
		if(buffer->pipeline_status_ != PIPELINE_STATUS_INPUT_OVER)
			return;

		wait_file_read_ = false;

    	for (unsigned int s=0; s < buffer->config_->pf_sessions_.size(); ++s){
    		if (buffer->config_->pf_sessions_[s] < 100){
    			buffer->config_->pf_sessions_[s] = buffer->all_sessions_[buffer->config_->pf_sessions_[s]];
    		}
    	}

    	// DEBUG
    	for (unsigned int i=0; i < buffer->all_sessions_.size(); ++i){
    		std::cout << "session " << i << " " << buffer->all_sessions_[i] << "\n";
    	}
    	std::cout << "PF SESSIONS:\n";
    	for (unsigned int i=0; i < buffer->config_->pf_sessions_.size(); ++i){
    		std::cout << "PF session " << i << " ending at " << buffer->config_->pf_sessions_[i] << " belongs to group " << buffer->config_->pf_groups_[i] << "\n";
    	}

    	initArrays();
	}

	if (buffer->last_pkg_id < MIN_PKG_ID){
		buffer->spike_buf_pos_pf_ = buffer->spike_buf_pos_speed_;
		pos_buf_pos_ = buffer->pos_buf_pos_ - 8;
		return;
	}

    while (buffer->spike_buf_pos_pf_ < buffer->spike_buf_pos_speed_){
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_pf_];
        
        unsigned int tetr = spike->tetrode_;
        int clust = spike->cluster_id_;

        if (clust > 0 &&  clust > (int)buffer->clusters_in_tetrode_[tetr]){
        	buffer->clusters_in_tetrode_[tetr] = clust;
        	// DEBUG
        	Log("New max clust in tetrode ", tetr);
        	Log("With number ", clust);
        }

		if (spike->discarded_ || Utils::Math::Isnan(spike->x) || clust == -1){
        	buffer->spike_buf_pos_pf_++;
            continue;
        }
     
        // update current session number based on the spike id
        while (session_group_ <  N_SESSIONS - 1 && spike->pkg_id_ > buffer->config_->pf_sessions_[session_group_]){
        	session_group_ ++;
        	if (buffer->config_->pf_groups_.size() > 0){
        		current_session_ = buffer->config_->pf_groups_[session_group_];
        	} else {
        		current_session_ = session_group_;
        	}
        	Log("Advance current session to ", session_group_);
        	Log("Switch current group to ", current_session_);`
        	Log("\twith spike at ", spike->pkg_id_);
        }

        if (spike->speed > SPEED_THOLD){
        	if (spike->y < 0 || spike->x < 0){
        		Log("WARNING: negative x or y coordinates of spikes, not added");
        	}

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
            AddPos(buffer->positions_buf_[pos_buf_pos_].x_pos(), buffer->positions_buf_[pos_buf_pos_].y_pos(), buffer->positions_buf_[pos_buf_pos_].pkg_id_);
        }
        pos_buf_pos_ ++;
    }

    // if prediction display requested and at least prediction_rate_ time has passed since last prediction
    if (display_prediction_ && buffer->last_pkg_id - last_predicted_pkg_ > prediction_rate_){
    	arma::fmat pred = arma::exp(buffer->last_predictions_[processor_number_].t() * DISPLAY_SCALE);
    	drawMat(pred);
    	last_predicted_pkg_ = buffer->last_pkg_id;
    }


    // TMP - for PFS generation
    smoothPlaceFields();
    dumpPlaceFields();
    exit(0);
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
    const float binw = window_width_ / float(nbinsx_);
    const float binh = window_height_ / float(nbinsy_);

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
            int x = (int)round(c * binw);
            int y = (int)round(r * binh);

            int cbinw = (int)round((c+1) * binw) - x;
            int cbinh = (int)round((r+1) * binh) - y;

            unsigned int order = (unsigned int)MIN(mat(r, c) * palette_.NumColors() / max_val, palette_.NumColors() - 1);

            FillRect(x, y, order, cbinw, cbinh);
        }
    }

    // draw actual position tail
    unsigned int end = (unsigned int)MIN(buffer->last_preidction_window_ends_[processor_number_] / POS_SAMPLING_RATE, buffer->pos_buf_pos_);
    for (unsigned int pos = end - 100; pos < end; ++pos) {
    	FillRect(int(buffer->positions_buf_[pos].x_pos() * binw / bin_size_), int(buffer->positions_buf_[pos].y_pos() * binh / bin_size_), 0, 2, 2);
    }

    if (!display_prediction_){
		ResetTextStack();
		int textCol = text_color_black_ ? 0xFFFFFF : 0x000000;
		TextOut(Utils::Converter::Combine("Tetrode: ", (int)display_tetrode_), textCol, true);
		TextOut(Utils::Converter::Combine("Cluster: ", display_cluster_), textCol, true);
		TextOut(Utils::Converter::Combine("Cluster global: ", int(buffer->global_cluster_number_shfit_[display_tetrode_] + display_cluster_)), textCol, true);
		TextOut(Utils::Converter::Combine("Session: ", (int)selected_session_), textCol, true);
		TextOut(Utils::Converter::Combine("Peak firing rate (Hz): ", max_val * buffer->SAMPLING_RATE / POS_SAMPLING_RATE), textCol, false);

		for (auto const& line: text_output){
			TextOut(line, textCol, true);
		}
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
    std::vector<std::string> lines = {Utils::Converter::Combine("# of spikes in max bin: ", int(place_fields_[display_tetrode_][display_cluster_][selected_session_](mx, my))), Utils::Converter::Combine("Total nuber of spikes accounted for: ", int(arma::sum(arma::sum(place_fields_[display_tetrode_][display_cluster_][selected_session_].Mat()))))};
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

void PlaceFieldProcessor::dumpPlaceFields(){
	Log("dump place fields...");
    for (size_t t=0; t < place_fields_.size(); ++t) {
        for (size_t c = 1; c <= buffer->clusters_in_tetrode_[t]; ++c) {
        	for (size_t s = 0; s < N_SESSIONS; ++s) {
				arma::mat dv = place_fields_smoothed_[t][c][s].Mat() / occupancy_smoothed_[s].Mat();
				arma::mat dvnonsm = place_fields_[t][c][s].Mat() / occupancy_[s].Mat();
				dv.save(BASE_PATH + Utils::Converter::int2str(c + buffer->global_cluster_number_shfit_[t]) + "_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
				dvnonsm.save(BASE_PATH + Utils::Converter::int2str(c + buffer->global_cluster_number_shfit_[t]) + "_" + Utils::Converter::int2str(s) + "_nosm.mat", arma::raw_ascii);
        	}
        }
    }

	for (size_t s = 0; s < N_SESSIONS; ++s) {
		occupancy_smoothed_[s].Mat().save(BASE_PATH + "occ_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
		occupancy_[s].Mat().save(BASE_PATH + "occ_" + Utils::Converter::int2str(s) + "_nosm.mat", arma::raw_ascii);
	}

    // write parameters to file pf_params.txt
    std::ofstream fpf_params(BASE_PATH + "params.txt");
    fpf_params << "MIN_OCCUPANCY " << MIN_OCCUPANCY;
    fpf_params << "\nSPEED_THRESHOLD " << SPEED_THOLD;
    fpf_params << "\nSIGMA " << sigma_;
    fpf_params << "\nBIN_SIZE " << bin_size_;
    fpf_params << "\nNBINSX " << nbinsx_;
    fpf_params << "\nNBINSY " << nbinsy_;
    fpf_params << "\nSPREAD " << spread_;
    fpf_params << "\nPF_SESSION ";
    for (unsigned int i=0; i < N_SESSIONS; ++i){
    	fpf_params << buffer->config_->pf_sessions_[i] << " ";
    }
    fpf_params << "\nPF_GROUPS ";
	for (unsigned int i=0; i < buffer->config_->pf_groups_.size(); ++i){
		fpf_params << buffer->config_->pf_groups_[i] << " ";
	}
	fpf_params << "\nSPIKE_FILES ";
	for (unsigned int i=0; i < buffer->config_->spike_files_.size(); ++i){
			fpf_params << buffer->config_->spike_files_[i] << " ";
		}
	fpf_params << "\n";

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
            	if (change_session)
					switchSession(4);
				else
					display_cluster_ = 5 + shift;
				break;
            case SDLK_6:
            	if (change_session)
            		switchSession(5);
            	else
            		display_cluster_ = 6 + shift;
                break;
            case SDLK_7:
            	if (change_session)
            		switchSession(6);
            	else
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
            	resetFieldsAndPointer();
                smoothPlaceFields();

                // TMP
                dumpPlaceFields();
                exit(0);

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
            	buffer->dumpCluAndRes(true);
    		    break;
            case SDLK_d:
            	// dump place fields
            	dumpPlaceFields();
    		    break;
            case SDLK_e:
            	if(kmod & KMOD_LSHIFT){
            		Log("Recalculate cluster number shifts - including empty cluster removal");
            		buffer->calculateClusterNumberShifts();
            	}
            	break;
            case SDLK_t:
            	text_color_black_ = ! text_color_black_;
            	break;
            // reset to re-build place fields
            case SDLK_r:
            	if(kmod & KMOD_LSHIFT){
            		resetFieldsAndPointer();
            	}

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
        		double sum = arma::accu(place_fields_[t][c][s].Mat());

        		if (sum > 1)
        			place_fields_smoothed_[t][c][s] = place_fields_[t][c][s].Smooth();

				if (SAVE){
					place_fields_smoothed_[t][c][s].Mat().save(BASE_PATH + Utils::Converter::int2str(t) + "_" + Utils::Converter::int2str(c) + "_" + Utils::Converter::int2str(s) + ".mat", arma::raw_ascii);
				}
        	}
        }
    }

    Log("Done smoothing place fields");
    // now calculated on-demand only, loaded from first session
    // buffer->calculateClusterNumberShifts();
}

void PlaceFieldProcessor::cachePDF(){
	// !!! dividing spike counts by occupancy will give the FR for window 1/pos_sampling_rate (s)
	// we need FR for window POP_VEC_WIN_LEN (ms), thus factor is POP_VEC_WIN_LEN  * pos_sampling_rat / 1000


	// factor - number by which the

	// TODO:FIX TO USE SESSION-WISE PFS !!! introduce counter (in case of buffer rewind)
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
            
            if (USE_PRIOR){
            	reconstructed_position_(r, c) += occupancy_smoothed_[current_session_](r, c) > 0 ? (fr_cnt * log(occupancy_smoothed_[current_session_](r, c) / occ_sum)) : -100000.0;
            }
        }
    }
    
    pos_updated_ = true;
    
    // update ID of last predicted package to control the frequency of prediction (more important for models with memory, for memory-less just for performance)
    last_predicted_pkg_ = buffer->population_vector_stack_.back()->pkg_id_;

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

void PlaceFieldProcessor::resetFieldsAndPointer() {
	Log("Reset place fields...");

	session_group_ = 0;
	current_session_ = 0;
    if (buffer->config_->pf_groups_.size() > 0)
    	current_session_ = buffer->config_->pf_groups_[0];

	buffer->spike_buf_pos_pf_ = 0;
	const unsigned int tetrn = buffer->tetr_info_->tetrodes_number();
	const unsigned int MAX_CLUST = 100;

	for (size_t t=0; t < tetrn; ++t) {
		place_fields_[t].resize(MAX_CLUST);
		place_fields_smoothed_[t].resize(MAX_CLUST);
		for (size_t c=0; c < MAX_CLUST; ++c) {
			for (size_t s=0; s < N_SESSIONS; ++s) {
				place_fields_[t][c][s].Zero();
				place_fields_smoothed_[t][c][s].Zero();
			}
		}
	}

	// force place field caclulation
	process();
}

void PlaceFieldProcessor::Resize() {
	SDLSingleWindowDisplay::Resize();
	drawPlaceField();
}
