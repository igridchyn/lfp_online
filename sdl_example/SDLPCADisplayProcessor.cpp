//
//  SDLPCADisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "SDLPCADisplayProcessor.h"
#include "OnlineEstimator.h"
#include "OnlineEstimator.cpp"

SDLPCADisplayProcessor::SDLPCADisplayProcessor(LFPBuffer *buffer)
:SDLPCADisplayProcessor(buffer,
		buffer->config_->getString("pcadisp.window.name"),
		buffer->config_->getInt("pcadisp.window.width"),
		buffer->config_->getInt("pcadisp.window.height"),
		buffer->config_->getInt("pcadisp.tetrode"),
		buffer->config_->getBool("pcadisp.display.unclassified"),
		buffer->config_->getFloat("pcadisp.scale"),
		buffer->config_->getInt("pcadisp.shift.x", buffer->config_->getInt("pcadisp.shift")),
		buffer->config_->getInt("pcadisp.shift.y", buffer->config_->getInt("pcadisp.shift"))
		){}

SDLPCADisplayProcessor::SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width,
		const unsigned int window_height, int target_tetrode, bool display_unclassified, const float& scale, const int shift_x, const int shift_y)
: LFPProcessor(buffer)
, SDLControlInputProcessor(buffer)
, SDLSingleWindowDisplay(buffer, window_name, window_width, window_height)
// paired qualitative brewer palette
, palette_(ColorPalette::BrewerPalette24)
, target_tetrode_(target_tetrode)
, display_unclassified_(display_unclassified)
, scale_(scale)
, shift_x_(shift_x)
, shift_y_(shift_y)
, time_end_(buffer->SAMPLING_RATE * 60)
, rend_freq_(buffer->config_->getInt("pcadisp.rend.rate", 1))
, poly_save_(buffer->config_->getBool("pcadisp.poly.save", false))
, poly_load_(buffer->config_->getBool("pcadisp.poly.load", false))
, poly_path_(buffer->config_->getOutPath("pcadisp.poly.path", "poly.dat"))
, num_pc_(buffer->config_->getInt("pca.num.pc"))
, refractory_period_(buffer->config_->getInt("pcadisp.refrac"))
, power_thold_nstd_(buffer->config_->getInt("spike.detection.nstd"))
, MAX_CLUST(buffer->config_->getInt("pcadisp.max.clust", 201))
, gaussian_distance_threshold_(buffer->config_->getFloat("pcadisp.gaussian.thold"))
{
    nchan_ = buffer->tetr_info_->channels_number(target_tetrode);

    if(poly_load_){

    	if (Utils::FS::FileExists(poly_path_)){

			std::ifstream fpoly(poly_path_);

			// read version
			std::string version;
			std::getline(fpoly, version);
			// version 1 - projections only, first inclusive, then exclusive, before 27.10.2016; or version 0 - inclusive projections only
			if (version[0] != 'V'){
				std::istringstream ss(version);

				// n tetr / n polycs / polycs
				unsigned int ntetr = 0;
				ss >> ntetr;

				if (ntetr == buffer->tetr_info_->tetrodes_number()){
					for(unsigned int t=0; t < ntetr; ++t){
						unsigned int nclust = 0;
						fpoly >> nclust;
						for (unsigned int c=0; c < nclust; ++c){
							buffer->cells_[t].push_back(PutativeCell(PolygonCluster(fpoly, 1)));
							if (buffer->cells_[t][c].polygons_.projections_inclusive_.size() == 0 && c > 0){
								Log("WARNING: empty cluster number ", c);
								Log("	in tetrode", t);
							}
						}
					}
				}
				else{
					buffer->Log("Number of tetrodes in clustering file is different from current number of tetrodes");
				}
			}
    	}else{
    		buffer->Log("ERROR: Polygon clusters file not found!");
    	}
    }

    //points_ = new SDL_Point[1000000];
    points_.reserve(1000000);

    spikes_to_draw_.resize(MAX_CLUST);
    for (unsigned int c = 0; c < MAX_CLUST; ++c) {

    	spikes_to_draw_[c] = new SDL_Point[spikes_draw_freq_ * 100];
	}
    spikes_counts_.resize(MAX_CLUST);

    display_cluster_.resize(MAX_CLUST, true);

    Log("PCA DISPLAY CONTROLS:\n"
    		"<MOUSE WHEEL> : Zoom In / Out\n"
    		"<LEFT MOUSE BUTTON> : Add polygon vertex\n"
    		"<MOUSE WLEEL PRESS> : Close polygon\n"
    		"<LEFT/RIGHT/UP/DOWN> : move around\n"
    		"b : toggle background color\n"
    		"d : delete last polyogon vertex\n"
    		"ESC : exit\n\n"

    		"0-9 : choose first component\n"
    		"NUM 0-9 : choose second component\n"
    		"LSHIFT: +10 to component / tetrode number\n"
    		"RSHIFT: +20 to component / tetrode number\n"
    		"ALT + 0-9 :choose tetrode [LSHIFT = +10; RDHIFT = +20]\n\n"

    		"CLUSTER OPERATIONS:\n"
    		"s : create new cluster\n"
    		"m : merge selected cluster\n"
    		"r : remove spikes from RED-selected cluster\n"
    		"R : delete RED-selected cluster\n"
    		"e : extract spikes from RED-selected cluster\n"
    		"E : exgract spikes from all displayed clusters\n"
    		"CTRL+z : undo last operation \n"

    		"DISPLAY MODES:\n"
    		"h : highlight selected cluster(s)\n"
    		"t : show only selected clusters\n"
    		"T : show all clusters\n"
    		"u : display / hide unclustered spikes\n"
    		"c : show spikes in the refractory period of the RED-selected cluster\n"
    		"a : save polygons\n"
    		"q : proejctions preview\n"
    		"* / : display less / more spikes\n"
    		"<KP> / and * : display sparsity factor (for better resolution of high density areas)\n"

    		"FITTING OPERATIONS:\n"
    		"g: exclude outliers to separate cluster after Guassian fit\n"
    		"G: split into two clusters according to GMM\n"
    		"LTRCL + g: decrease outlier threshold\n"
    		"RCTRL + g: increase outlier threshold\n"
    );

    drawn_pixels_.resize(window_width_ * window_height_);
}

SDLPCADisplayProcessor::~SDLPCADisplayProcessor(){
	// delete[] points_;
    for (unsigned int c = 0; c < MAX_CLUST; ++c) {
    	delete []spikes_to_draw_[c];
	}
}

void SDLPCADisplayProcessor::process(){
    bool render = false;

    if (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_){
    	SDL_SetRenderTarget(renderer_, texture_);
    }

	while(user_context_.HasNewAction(last_proc_ua_id_)){
		// process new user action

		const UserAction *ua = user_context_.GetNextAction(last_proc_ua_id_);
		last_proc_ua_id_ = ua->id_;

		switch(ua->action_type_){

		case UA_SELECT_CLUSTER1:{
			// count number of selected
			unsigned int nsel = 0;
			for (int c = 0; c < (int)MAX_CLUST; ++c){
				if(display_cluster_[c])
					nsel ++;
			}

			if (nsel < 3 && !display_cluster_[0])
				for (int c = 0; c < (int)MAX_CLUST; ++c){
					if (c != user_context_.SelectedCluster1() && c != user_context_.SelectedCluster2()){
						display_cluster_[c] = false;
					}

				}

			if (user_context_.SelectedCluster1() > -1){
				display_cluster_[user_context_.SelectedCluster1()] = true;
				buffer->spike_buf_no_disp_pca = 0;
				RenderClear();
			} else if (user_context_.SelectedCluster1() == -1 && user_context_.SelectedCluster2() == -1){
				display_cluster_[0] = true;
			}

			break;
		}

		case UA_SELECT_CLUSTER2:{
			// count number of selected
			unsigned int nsel = 0;
			for (int c = 0; c < (int)MAX_CLUST; ++c){
				if(display_cluster_[c])
					nsel ++;
			}

			if (nsel < 3 && !display_cluster_[0])
				for (int c = 0; c < (int)MAX_CLUST; ++c){
					if (c != user_context_.SelectedCluster1() && c != user_context_.SelectedCluster2()){
						display_cluster_[c] = false;
					}
				}

			if (user_context_.SelectedCluster2() > -1){
				display_cluster_[user_context_.SelectedCluster2()] = true;
				buffer->spike_buf_no_disp_pca = 0;
				RenderClear();
			} else if (user_context_.SelectedCluster1() == -1 && user_context_.SelectedCluster2() == -1){
				display_cluster_[0] = true;
			}

			break;
		}

		default:
			break;

		}

	}

    while (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_disp_pca];
        // wait until cluster is assigned

        // TODO !!! take channel with the spike peak (save if not available)
//        double power_thold = buffer->powerEstimatorsMap_[buffer->tetr_info_->tetrode_channels[spike->tetrode_][0]]->get_std_estimate() * power_thold_nstd_ * power_threshold_factor_;

        // TODO : configurable factor
//        double power_thold = 1000 * power_threshold_factor_;
        // BUGGY -> DISABLED
//        if (abs(spike->power_) < power_thold){
//        	buffer->spike_buf_no_disp_pca++;
//        	continue;
//        }

        if (spike->tetrode_ != target_tetrode_){
        	buffer->spike_buf_no_disp_pca++;
        	continue;
        }

        if (spike->pc == nullptr || (spike->cluster_id_ == -1 && !display_unclassified_))
        {
            if (spike->discarded_){
                buffer->spike_buf_no_disp_pca++;
                continue;
            }
            else{
                break;
            }
        }

        // polygon cluster
        if (spike->cluster_id_ == -1 && need_clust_check_){
        	for (size_t i=0; i < buffer->cells_[spike->tetrode_].size(); ++i){
        		bool contains = buffer->cells_[spike->tetrode_][i].Contains(spike);
        		if (contains){
        			spike->cluster_id_ = i;
        		}
        	}
        }

        // skip if subsampling
        total_spikes_to_draw ++;
        if (total_spikes_to_draw % draw_subsample_factor){
        	buffer->spike_buf_no_disp_pca++;
        	continue;
        }

        int x;
        int y;
        getSpikeCoords(spike, x ,y);


        // TODO ??? don't display artifacts and unknown with the same color
        const unsigned int cid = spike->cluster_id_ > -1 ? spike->cluster_id_ : 0;

        if (! display_cluster_[cid]){
        	buffer->spike_buf_no_disp_pca++;
        	continue;
        }

        // if not online, collect and draw in batches
        if (buffer->pipeline_status_ != PIPELINE_STATUS_ONLINE){
        	if (preview_mode_){
        		unsigned int oc1 = comp1_, oc2 = comp2_;
        		unsigned int compn = buffer->tetr_info_->channels_number(target_tetrode_) * 2;
        		unsigned int cnt = 0;

        		const unsigned int NWX = 7;
        		const unsigned int NWY = 4;

        		unsigned int dx = window_width_ / NWX;
        		unsigned int dy = window_height_ / NWY;


        		for (unsigned int c1 = 0; c1 < compn; ++c1)
        			for (unsigned int c2 = 0; c2 < c1; ++c2){
        				comp1_ = c1;
        				comp2_ = c2;
        				getSpikeCoords(spike, x, y);

        				if (x < 0 || x > (int)window_width_ || y < 0 || y > (int)window_height_){
        					cnt ++;
        					continue;
        				}

        				x /= NWX;
        				y /= NWY;
        				x += dx * (cnt % NWX);
        				y += dy * (cnt / NWX);


        				spikes_to_draw_[cid][spikes_counts_[cid] ++] = {x, y};

        				cnt ++;
        			}

        		comp1_ = oc1;
        		comp2_ = oc2;
        	} else {
        		if (y < window_height_ && x < window_width_ && !drawn_pixels_[y * window_width_ + x]){
        			drawn_pixels_[y * window_width_ + x] = true;
        			spikes_to_draw_[cid][spikes_counts_[cid] ++] = {x, y};
        		}
        	}

        	if (highlight_current_cluster_ && (user_context_.SelectedCluster2() == (int)cid || user_context_.SelectedCluster1() == (int)cid)){
        		spikes_to_draw_[cid][spikes_counts_[cid] ++] = {x + 1, y + 1};
        		spikes_to_draw_[cid][spikes_counts_[cid] ++] = {x - 1, y + 1};
        	}
        }
        else{
        	SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid), 255);
        	SDL_RenderDrawPoint(renderer_, x, y);
        }

        // check if spike batch has to be drawn
        if (spikes_counts_[cid] >= spikes_draw_freq_)
        {
        	SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid), 255);
        	SDL_RenderDrawPoints(renderer_, &spikes_to_draw_[cid][0], spikes_counts_[cid]);
        	spikes_counts_[cid] = 0;
        }

        // display refractory spike
        if (refractory_display_cluster_ >= 0 && spike->cluster_id_ == refractory_display_cluster_){
        	if(spike->pkg_id_ - refractory_last_time_ < refractory_period_){
				SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
				// red = 5
				DrawCross(3, x, y, refractory_display_cluster_ + 1);
				DrawCross(3, refractory_last_x_, refractory_last_y_, refractory_display_cluster_ + 1);
        	}

        	refractory_last_time_ = spike->pkg_id_;
        	refractory_last_x_ = x;
        	refractory_last_y_ = y;
        }

        buffer->spike_buf_no_disp_pca++;

        int freqmult = buffer->pipeline_status_ == PIPELINE_STATUS_READ_FET ? 30000 : 1;
        if (!(buffer->spike_buf_no_disp_pca % (rend_freq_ * freqmult)))
            render = true;
    }

    if (render){
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);

        if (preview_mode_){
        	const unsigned int NWX = 7;
        	const unsigned int NWY = 4;

        	unsigned int dx = window_width_ / NWX;
        	for (unsigned int dxi = 1; dxi < NWX; ++dxi){
        		SDL_RenderDrawLine(renderer_, dx * dxi, 0, dx * dxi, window_height_);
        	}
        	unsigned int dy = window_height_ / NWY;
        	for (unsigned int dyi = 1; dyi < NWY; ++dyi){
        		SDL_RenderDrawLine(renderer_, 0, dy * dyi, window_width_, dy * dyi);
        	}
        } else {
        	SDL_RenderDrawLine(renderer_, 0, shift_y_, window_width_, shift_y_);
        	SDL_RenderDrawLine(renderer_, shift_x_, 0, shift_x_, window_height_);
        }

        // polygon vertices
        for(size_t i=0; i < polygon_x_.size(); ++i){
        	SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 255);
        	int x = int(polygon_x_[i] * scale_ + shift_x_);
        	int y = int(polygon_y_[i] * scale_ + shift_y_);
        	int w = 3;
        	SDL_RenderDrawLine(renderer_, x-w, y, x+w, y);
        	SDL_RenderDrawLine(renderer_, x, y-w, x, y+w);

        	if (i > 0){
				int px = int(polygon_x_[i - 1] * scale_ + shift_x_);
        		int py = int(polygon_y_[i-1] * scale_ + shift_y_);
        		SDL_RenderDrawLine(renderer_, px, py, x, y);
        	}
        }

        // TODO !!! don't redraw every time?
        // TODO ??? select unknown cluster
        if (user_context_.SelectedCluster1() > 0 && buffer->cells_[target_tetrode_].size() > (unsigned int)user_context_.SelectedCluster1()){
        	std::vector<PolygonClusterProjection> & princ = buffer->cells_[target_tetrode_][user_context_.SelectedCluster1()].polygons_.projections_inclusive_;
        	std::vector<PolygonClusterProjection> & prex = buffer->cells_[target_tetrode_][user_context_.SelectedCluster1()].polygons_.projections_exclusive_;

        	for(size_t p=0; p < princ.size() + prex.size(); ++p){
        		PolygonClusterProjection& proj = p < princ.size() ? princ[p] :prex[p - princ.size()];
        		if ((unsigned int)proj.dim1_ == comp1_ && (unsigned int)proj.dim2_ == comp2_){
        			SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 255);
        			for (size_t pt=1;pt < proj.coords1_.size(); ++pt){
        				int x = (int)scale_x(proj.coords1_[pt-1]);
						int y = (int)scale_y(proj.coords2_[pt - 1]);
						int x2 = (int)scale_x(proj.coords1_[pt]);
						int y2 = (int)scale_y(proj.coords2_[pt]);
        				SDL_RenderDrawLine(renderer_, x, y, x2, y2);
        			}
        		}
        		if ((unsigned int)proj.dim1_ == comp2_ && (unsigned int)proj.dim2_ == comp1_){
        			SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 255);
        			for (size_t pt=1;pt < proj.coords1_.size(); ++pt){
						int x = (int)scale_x(proj.coords2_[pt - 1]);
						int y = (int)scale_y(proj.coords1_[pt - 1]);
						int x2 = (int)scale_x(proj.coords2_[pt]);
						int y2 = (int)scale_y(proj.coords1_[pt]);
        				SDL_RenderDrawLine(renderer_, x, y, x2, y2);
        			}
        		}
        	}
        }
        if (user_context_.SelectedCluster2() >= 0 && buffer->cells_[target_tetrode_].size() > (unsigned int)user_context_.SelectedCluster2()){
			std::vector<PolygonClusterProjection> & princ = buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_.projections_inclusive_;
			std::vector<PolygonClusterProjection> & prex = buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_.projections_exclusive_;

			for(size_t p=0; p < princ.size() + prex.size(); ++p){
				PolygonClusterProjection& proj = p < princ.size() ? princ[p] :prex[p - princ.size()];
					if ((unsigned int)proj.dim1_ == comp1_ && (unsigned int)proj.dim2_ == comp2_){
						SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
						for (size_t pt=1;pt < proj.coords1_.size(); ++pt){
							int x = (int)scale_x(proj.coords1_[pt - 1]);
							int y = (int)scale_y(proj.coords2_[pt - 1]);
							int x2 = (int)scale_x(proj.coords1_[pt]);
							int y2 = (int)scale_y(proj.coords2_[pt]);
							SDL_RenderDrawLine(renderer_, x, y, x2, y2);
						}
					}
					if ((unsigned int)proj.dim1_ == comp2_ && (unsigned int)proj.dim2_ == comp1_){
						SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
						for (size_t pt=1;pt < proj.coords1_.size(); ++pt){
							int x = (int)scale_x(proj.coords2_[pt - 1]);
							int y = (int)scale_y(proj.coords1_[pt - 1]);
							int x2 = (int)scale_x(proj.coords2_[pt]);
							int y2 = (int)scale_y(proj.coords1_[pt]);
							SDL_RenderDrawLine(renderer_, x, y, x2, y2);
						}
					}
				}
			}

		if (polygon_closed_ && polygon_x_.size() > 0){
			SDL_SetRenderDrawColor(renderer_, 0, 255, 0,255);
			SDL_RenderDrawLine(renderer_, (int)scale_x(polygon_x_[0]), (int)scale_y(polygon_y_[0]), (int)scale_x(polygon_x_[polygon_x_.size() - 1]), (int)scale_y(polygon_y_[polygon_y_.size() - 1]));
        }

		// too slow, don't display during load
		if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER || buffer->pipeline_status_ == PIPELINE_STATUS_ONLINE){
			ResetTextStack();
			std::string text = std::string("Tetrode # ") + Utils::NUMBERS[target_tetrode_ + 1] + "( channels:";
			for (unsigned int c=0; c < buffer->tetr_info_->channels_number(target_tetrode_); ++c){
				text += std::string(" ") + Utils::Converter::int2str((int)buffer->tetr_info_->tetrode_channels[target_tetrode_][c]);
			}
			text += ") PC " + Utils::Converter::int2str(comp1_) + " / " + Utils::Converter::int2str(comp2_) + " CNS=" + Utils::Converter::int2str(buffer->global_cluster_number_shfit_[target_tetrode_]);
			TextOut(text);

			// cluster numbers
			number_panel_mapping_.push_back(0);
			for (unsigned int c=1; c < MAX_CLUST; ++c){
//				if (display_cluster_[c]){
					TextOut(Utils::Converter::int2str(c) + " ", palette_.getColor(c % palette_.NumColors()), false);
					number_panel_mapping_.push_back(number_panel_mapping_[number_panel_mapping_.size() - 1] + last_text_width_);
//				}
			}
			// whether displayed or not
			for (unsigned int c=1; c < MAX_CLUST; ++c){
				std::string dText = display_cluster_[c] ? "+" : "-";
				TextOut(dText, number_panel_mapping_[c - 1], 40, palette_.getColor(c % palette_.NumColors()), false);
			}
		}

		double power_thold = 100 * power_threshold_factor_;
		std::stringstream ss;
		ss << "Power threshold: " << power_thold;
		if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER){
			TextOut(ss.str());
		}

		// if were redrawing in an off-line mode, draw all collected spikes
		if (buffer->pipeline_status_ != PIPELINE_STATUS_ONLINE){
			for (size_t cid=0; cid <  buffer->cells_[target_tetrode_].size(); ++cid){
				SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid), 255);
				SDL_RenderDrawPoints(renderer_, &spikes_to_draw_[cid][0], spikes_counts_[cid]);
				spikes_counts_[cid] = 0;
			}
		}

    	Render();
    	std::fill(drawn_pixels_.begin(), drawn_pixels_.end(), false);
    }
}

void SDLPCADisplayProcessor::save_polygon_clusters() {
	if (poly_path_.length() == 0){
		buffer->Log("ERROR: Polygon cluster file path was not specified in the config");
		return;
	}

	std::ofstream fpoly(poly_path_);

	int ntetr = buffer->tetr_info_->tetrodes_number();
	fpoly << ntetr << "\n";

	for(int t=0; t < ntetr; ++t){
		fpoly << buffer->cells_[t].size() << "\n";
		for (size_t c=0; c < buffer->cells_[t].size(); ++c){
			buffer->cells_[t][c].polygons_.Serialize(fpoly);
		}
	}

	fpoly << "FORMAT:\n<tetrode number>\n[<clusters number>\n<inclusive projections size>\n[<dim1> <dim2> <npoints>\n[<xi yi]]]";

	fpoly.close();
}

void SDLPCADisplayProcessor::getSpikeCoords(const Spike *const spike, int& x, int& y) {
	 // time
	if (comp1_ == spike->num_pc_ * spike->num_channels_){
		x = int((spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_);
	}
	else{
		float rawx = spike->getFeature(comp1_); // spike->pc[comp1_ % nchan_][comp1_ / nchan_];
		x = int(rawx * scale_ + shift_x_);
	}
	if (comp2_ == spike->num_pc_ * spike->num_channels_){
		y = int((spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_);
	}
	else{
		float rawy = spike->getFeature(comp2_); //spike->pc[comp2_ % nchan_][comp2_ / nchan_];
		y = int(rawy * scale_ + shift_y_);
	}
}

void SDLPCADisplayProcessor::undoUserAction(){
	UserAction lastAction = user_context_.action_list_.back();
	if (lastAction.action_type_ == UA_ADD_EXCLUSIVE_PROJECTION){
		for (unsigned int i=0; i < lastAction.spike_ids_.size(); ++i){
			buffer->spike_buffer_[lastAction.spike_ids_[i]]->cluster_id_ = lastAction.cluster_number_1_;
		}
		user_context_.action_list_.pop_back();
		buffer->ResetAC(target_tetrode_, lastAction.cluster_number_1_);
		buffer->ResetAC(target_tetrode_, lastAction.cluster_number_2_);
	} else if (lastAction.action_type_ == UA_MERGE_CLUSTERS){
		unsigned int clun = 0;
		PolygonCluster new_clust_ = createNewCluster(clun);
		buffer->Log("Undo merge");

		for (unsigned int i=0; i < lastAction.spike_ids_.size(); ++i){
			buffer->spike_buffer_[lastAction.spike_ids_[i]]->cluster_id_ = clun;
		}

		buffer->ResetAC(target_tetrode_, clun);
		buffer->ResetPopulationWindow();
	} else {
		Log("CANNOT UNDO ACTION OF THIS TYPE");
	}
}

void SDLPCADisplayProcessor::process_SDL_control_input(const SDL_Event& e){
	SDL_Keymod kmod = SDL_GetModState();

	bool need_redraw = false;

	if( e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
		// update
		SDL_RenderPresent(renderer_);
		return;
	}

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.windowID == GetWindowID()){
		if (e.button.button == SDL_BUTTON_LEFT){

			if (preview_mode_){
				preview_mode_ = false;
				unsigned int wx = e.button.x * 7 / window_width_;
				unsigned int wy = e.button.y * 4 / window_height_;

				std::cout << "sub-window number " << wx << ", " << wy << "\n";

				// find out to which components this corresponds
				unsigned int compn = buffer->tetr_info_->channels_number(target_tetrode_) * 2;
				unsigned int cnt = 0;
				bool found = false;
				for (unsigned int c1 = 0; c1 < compn; ++c1){
					for (unsigned int c2 = 0; c2 < c1; ++c2){
						unsigned int nx = cnt % 7;
						unsigned int ny = cnt / 7;

						if (nx == wx && ny == wy){
							std::cout << "cnt = " << cnt << ", c1 = " << c1 << ", c2 = " << c2 << "\n";

							comp1_ = c1;
							comp2_ = c2;
							found = true;
							break;
						}

						cnt ++;
					}
					if (found)
						break;
				}

				need_redraw = true;
			}
			// select cluster 1
			else if (kmod & KMOD_LCTRL){
				float rawx = (e.button.x - shift_x_) / scale_;
				float rawy = (e.button.y - shift_y_) / scale_;

				for(size_t c=0; c < buffer->cells_[target_tetrode_].size(); ++c){
					if (buffer->cells_[target_tetrode_][c].polygons_.Contains(rawx, rawy)){
						if (user_context_.SelectedCluster1() >= 0){
							if ((unsigned int)user_context_.SelectedCluster1() == c){
								user_context_.SelectCluster1(-1);
							}
							else{
								user_context_.SelectCluster1(c);
							}
						}
						else{
							user_context_.SelectCluster1(c);
						}
						//need_redraw = true;
						break;
					}
				}
			}else if (kmod & KMOD_LSHIFT){ // select clustewr 2
				float rawx = (e.button.x - shift_x_) / scale_;
				float rawy = (e.button.y - shift_y_) / scale_;

				for(size_t c=0; c < buffer->cells_[target_tetrode_].size(); ++c){
					if (buffer->cells_[target_tetrode_][c].polygons_.Contains(rawx, rawy)){
						if (user_context_.SelectedCluster2() >= 0){
							if ((unsigned int)user_context_.SelectedCluster2() == c){
								user_context_.SelectCluster2(-1);
							}
							else{
								user_context_.SelectCluster2(c);
							}
						}
						else{
							user_context_.SelectCluster2(c);
						}
						//need_redraw = true;
						break;
					}
				}
			}
			else{ // create polygon
				// toggle cluster display
				if (e.button.y <= 60){
					unsigned int x = e.button.x;
					unsigned int c = 0;
					while (c < number_panel_mapping_.size() && x > number_panel_mapping_[c]) ++c;

					if (c >= number_panel_mapping_.size())
						return;

					display_cluster_[c] = ! display_cluster_[c];
					std::cout << "Toggle cluster " << c << "\n";
					need_redraw = true;
				} else if (!polygon_closed_){
					polygon_x_.push_back((e.button.x - shift_x_) / scale_);
					polygon_y_.push_back((e.button.y - shift_y_) / scale_);

					SDL_SetRenderTarget(renderer_, texture_);
					DrawCross(3, e.button.x, e.button.y, 4);
					if (polygon_x_.size() > 1){
						int last = polygon_x_.size() - 1;
						SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 0);
						SDL_RenderDrawLine(renderer_, (int)scale_x(polygon_x_[last]), (int)scale_y(polygon_y_[last]), (int)scale_x(polygon_x_[last - 1]), (int)scale_y(polygon_y_[last - 1]));
					}
					Render();
				}else{
					polygon_closed_ = false;
					polygon_x_.clear();
					polygon_y_.clear();
				}
			}
		}
		else if (e.button.button == SDL_BUTTON_MIDDLE && polygon_y_.size() > 0){
			polygon_closed_ = true;
			//reset_spike_pointer();
			SDL_SetRenderTarget(renderer_, texture_);
			if (polygon_x_.size() > 1){
				int last = polygon_x_.size() - 1;
				SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 0);
				SDL_RenderDrawLine(renderer_, (int)scale_x(polygon_x_[last]), (int)scale_y(polygon_y_[last]), (int)scale_x(polygon_x_[0]), (int)scale_y(polygon_y_[0]));
			}
			Render();
		}
	}

	if (e.type == SDL_MOUSEWHEEL){
		scale_ /= pow(1.1f, e.wheel.y);
		need_redraw = true;
		need_clust_check_ = false;
	}

	bool need_current_projection_reset = true;

    if( e.type == SDL_KEYDOWN )
    {
    	need_redraw = true;
    	need_clust_check_ = false;


        // select clusters from 10 to 39
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift += 10;
        }
		if (kmod & KMOD_RSHIFT){
			shift += 20;
		}

        unsigned int old_comp1 = comp1_;
        unsigned int old_comp2 = comp2_;

        bool kp_pressed_ = false;

        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
        	case SDLK_h:
        		highlight_current_cluster_ = ! highlight_current_cluster_;
        		need_redraw = true;
        		break;

        	// change power threshold + udpate ACs
//        	case SDLK_EQUALS:
//        		power_threshold_factor_ *= power_threshold_factor_step_;
//        		need_redraw = true;
//        		Log("Power threshold set to: ", power_threshold_factor_);
//        		buffer->ResetAC(target_tetrode_);
//        		break;

        	case SDLK_KP_MULTIPLY:
        		// draw subsampling factor control
        		draw_subsample_factor ++;
        		need_redraw = true;
        		Log("Increase draw sampling factor to ", draw_subsample_factor);
                break;

        	case SDLK_KP_DIVIDE:
        		if (draw_subsample_factor > 1){
        			draw_subsample_factor --;
        			need_redraw = true;
        			Log("Decrease draw sampling factor to ", draw_subsample_factor);
        		}
        		break;

//        	case SDLK_MINUS:
//				power_threshold_factor_ /= power_threshold_factor_step_;
//				need_redraw = true;
//				Log("Power threshold set to: ", power_threshold_factor_);
//				buffer->ResetAC(target_tetrode_);
//        	    break;

        	// merge clusters
        	case SDLK_m:
        		need_redraw = false;
        		if (user_context_.SelectedCluster1() < 0 || user_context_.SelectedCluster2() < 0)
        			break;
        		else{
        			mergeClusters();
        			need_redraw = true;
        		}

        		break;

        	// polygon operations
        	case SDLK_s:{
        		addCluster();
        		need_redraw = false;
        		break;
        	}

        	// show [T]wo selected clusters only : T - show all
        	case SDLK_t:{
        		for (unsigned int c=0; c < MAX_CLUST; ++c){
        			if (shift > 0){
        				display_cluster_[c] = true;
        			} else {
        				display_cluster_[c] = (c == (unsigned int)user_context_.SelectedCluster1() || c == (unsigned int)user_context_.SelectedCluster2());
        			}
        		}

        		// if none selected - show unclustered
        		if (user_context_.SelectedCluster1() == -1 && user_context_.SelectedCluster2() == -1){
        			display_cluster_[0] = true;
        		}

        		need_redraw = true;
        		break;
        	}

        	// e: extract from selected cluster
        	case SDLK_e:
        	{
        		if (kmod & KMOD_LSHIFT){
        			extractClusterFromMultiple();
        		} else {
        			extractCluster();
        		}
        		break;
        	}

        	// r: set cluster to unassigned in selected spikes
        	case SDLK_r:
        	{
        		if (kmod & KMOD_LSHIFT){
        			// DELETE  CLUSTER (selected cluster 2)
        			deleteCluster();
        		}
        		// add exclusive projection to selected cluster 2
        		else{
        			addExclusiveProjection();
        		}
        		break;
        	}

        	case SDLK_u:{
        		// toggle unclustered display
        		display_cluster_[0] = ! display_cluster_[0];
        		need_redraw = true;
        		break;
        	}

        	// d: delete last polygon point
        	case SDLK_d:
				if (polygon_x_.size() > 0){
					polygon_x_.erase(polygon_x_.end() - 1);
					polygon_y_.erase(polygon_y_.end() - 1);
				}
				// need_redraw = false;

        		break;

        	// c: clear cluster -> show spikes in the refractory period
        	case SDLK_c:
        		// RELOAD THE CLUSTER IDENTITIES
        		// !!! ASSUMING NO NEW CLUSTERED SPIKES, ONLY CHANGED IDENTITIES !!!
        		if (kmod & KMOD_LSHIFT){

        			buffer->clu_reset_ = true;

        			// 1-to-1 buffer -> clu files content
//        			unsigned int current_file = 0;
//        			unsigned int clu_changed = 0;
//        			unsigned bufind = 0;
//        			int clu = -1;
//        			std::ifstream clu_stream_;
//
//        			while (current_file < buffer->config_->spike_files_.size()){
//						std::string filepath = buffer->config_->spike_files_[current_file] + "clu";
//						clu_stream_.open(filepath);
//
//						while (!clu_stream_.eof()){
//							clu_stream_ >> clu;
//							// find next clustered spike and update clu + need to account for cluster number shifts
//							while (buffer->spike_buffer_[bufind]->cluster_id_ <= 0 || buffer->spike_buffer_[bufind]->discarded_)
//								bufind ++;
//
//							if (buffer->spike_buffer_[bufind]->cluster_id_ != clu - (int)buffer->global_cluster_number_shfit_[buffer->spike_buffer_[bufind]->tetrode_])
//								clu_changed ++;
//
//							// buffer->spike_buffer_[bufind]->cluster_id_ = clu - buffer->global_cluster_number_shfit_[buffer->spike_buffer_[bufind]->tetrode_];
//							bufind ++;
//						}
//						current_file ++;
//						clu_stream_.close();
//        			}
//        			Log("Number of cluster identities changed:", clu_changed);
        		}

        		// cancel refractory spikes display
        		if (refractory_display_cluster_ >= 0){
        			refractory_display_cluster_ = -1;
        			break;
        		}

        		if (user_context_.SelectedCluster2() == -1){
        			break;
        		}

        		refractory_display_cluster_ = user_context_.SelectedCluster2();

        		break;

        	case SDLK_a:
        		if (kmod & KMOD_LSHIFT){
        			save_polygon_clusters();
        		}
        		break;

        	case SDLK_RIGHTBRACKET:
        		if (user_context_.SelectedCluster2() > 0 && !memory_less_clustering_){
        			need_current_projection_reset = false;
        			PolygonCluster & poly = buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_;
        			current_projection_ = (current_projection_ + 1) % (poly.projections_exclusive_.size() + poly.projections_inclusive_.size());
        			if ((unsigned int)current_projection_ < poly.projections_inclusive_.size()){
        				comp1_ = poly.projections_inclusive_[current_projection_].dim1_;
        				comp2_ = poly.projections_inclusive_[current_projection_].dim2_;
        			} else {
        				comp1_ = poly.projections_exclusive_[current_projection_ - poly.projections_exclusive_.size()].dim1_;
        				comp2_ = poly.projections_exclusive_[current_projection_ - poly.projections_exclusive_.size()].dim2_;
        			}
        			need_redraw = true;
        		}
				break;

        	case SDLK_LEFTBRACKET:
        		if (user_context_.SelectedCluster2() > 0 && !memory_less_clustering_){
        			need_current_projection_reset = false;
        			PolygonCluster & poly = buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_;
        			current_projection_ = (current_projection_ - 1) % (poly.projections_exclusive_.size() + poly.projections_inclusive_.size());
        			if ((unsigned int)current_projection_ < poly.projections_inclusive_.size()){
        				comp1_ = poly.projections_inclusive_[current_projection_].dim1_;
        				comp2_ = poly.projections_inclusive_[current_projection_].dim2_;
        			} else {
        				comp1_ = poly.projections_exclusive_[current_projection_ - poly.projections_exclusive_.size()].dim1_;
        				comp2_ = poly.projections_exclusive_[current_projection_ - poly.projections_exclusive_.size()].dim2_;
        			}
        			need_redraw = true;
        		}
				break;

			// toggle background color
        	case SDLK_b:{
        		whiteBG_ = ! whiteBG_;
        		break;
        	}

			// disply histogram of Gaussian distances
        	case SDLK_g:
        		if (kmod & KMOD_LCTRL){
        			gaussian_distance_threshold_ /= 2.0;
        			Log("Decreased gaussian distance threshold: ", gaussian_distance_threshold_);
        			break;
        		}

        		if (kmod & KMOD_RCTRL){
					gaussian_distance_threshold_ *= 2.0;
					Log("Increased gaussian distance threshold: ", gaussian_distance_threshold_);
					break;
				}

        		if (kmod & KMOD_LSHIFT){
        			splitIntoGuassians();
        		} else {
        			displayChiHistogramm();
        		}
        		need_redraw = true;
        		break;

        	case SDLK_p:
        		if (kmod & KMOD_LSHIFT){
        			// cluster by proximity - DISABLED
        			//clusterNNwithThreshold();
        			Log("NN CLUSTERING TEMPORARILY DISABLED FOR PERFORMANCE REASONS");
        			need_redraw = true;
        		}
        		break;

        	// CTRL+Z : UNDO
        	case SDLK_z:
        		if (kmod & KMOD_LCTRL){
        			undoUserAction();
        			need_redraw = true;
        		}
        		break;

        	case SDLK_q:
        		preview_mode_ = ! preview_mode_;
        		need_redraw = true;
        		break;

            case SDLK_ESCAPE:
                buffer->processing_over_ = true;
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
                comp1_ = 6 + shift;
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
                kp_pressed_ = true;
                break;
            case SDLK_KP_2:
                comp2_ = 2 + shift;
                kp_pressed_ = true;
                break;
            case SDLK_KP_3:
                comp2_ = 3 + shift;
                kp_pressed_ = true;
                break;
            case SDLK_KP_4:
                comp2_ = 4 + shift;
                kp_pressed_ = true;
                break;
            case SDLK_KP_5:
                comp2_ = 5 + shift;
                kp_pressed_ = true;
                break;
            case SDLK_KP_6:
                comp2_ = 6 + shift;
                kp_pressed_ = true;
                break;
            case SDLK_KP_7:
                comp2_ = 7;
                kp_pressed_ = true;
                break;
            case SDLK_KP_8:
                comp2_ = 8;
                kp_pressed_ = true;
                break;
            case SDLK_KP_9:
                comp2_ = 9;
                kp_pressed_ = true;
                break;
            case SDLK_KP_0:
                comp2_ = 0 + shift;
                kp_pressed_ = true;
                break;
            case SDLK_KP_MINUS:
            	scale_ /= 1.1f;
            	break;
            case SDLK_KP_PLUS:
            	scale_ *= 1.1f;
            	break;
            case SDLK_RIGHT:
            	shift_x_ -= 50;
            	break;
            case SDLK_LEFT:
            	shift_x_ += 50;
            	break;
            case SDLK_UP:
            	shift_y_ += 50;
            	break;
            case SDLK_DOWN:
            	shift_y_ -= 50;
            	break;
            default:
                need_redraw = false;

        }

        if (kp_pressed_ && (kmod & KMOD_RALT)){
        	display_cluster_[comp2_] = ! display_cluster_[comp2_];
        	comp2_ = old_comp2;
        	need_redraw = true;
        }

        if (comp1_ != old_comp1 || comp2_ != old_comp2){
        	if (comp1_ >= num_pc_ * nchan_ + 4){
        		comp1_ = old_comp1;
        	}

        	if (comp2_ >= num_pc_ * nchan_ + 4){
        		comp2_ = old_comp2;
        	}

        	if (need_current_projection_reset){
        		current_projection_ = -1;
        	}

        	for (unsigned int i=0; i < MAX_CLUST; ++i){
        		spikes_counts_[i] = 0;
        	}
        }
    }

    if (need_redraw){
		// TODO: case-wise
		buffer->spike_buf_no_disp_pca = 0;

		time_start_ = buffer->spike_buffer_[buffer->SPIKE_BUF_HEAD_LEN] == nullptr ? 0 : buffer->spike_buffer_[buffer->SPIKE_BUF_HEAD_LEN]->pkg_id_;
		if (buffer->spike_buf_pos_unproc_ > 1 && buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1] != nullptr)
		time_end_ = buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_;

		RenderClear(whiteBG_);
	}
}

void SDLPCADisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode) {
	target_tetrode_ = display_tetrode;
	RenderClear();

	comp1_ = 0;
	comp2_ = 2;

	polygon_x_.clear();
	polygon_y_.clear();

	nchan_ = buffer->tetr_info_->channels_number(target_tetrode_);

	user_context_.active_tetrode_ = display_tetrode;

	user_context_.SelectCluster1(-1);
	user_context_.SelectCluster2(-1);

	buffer->spike_buf_no_disp_pca = 0;
	time_start_ = buffer->spike_buffer_[buffer->SPIKE_BUF_HEAD_LEN] == nullptr ? 0 : buffer->spike_buffer_[buffer->SPIKE_BUF_HEAD_LEN]->pkg_id_;
	if (buffer->spike_buf_pos_unproc_ > 1 && buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1] != nullptr)
	time_end_ = buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_;
}

void SDLPCADisplayProcessor::deleteCluster() {
	const int c2 = user_context_.SelectedCluster2();

	if (c2 >= 0){
		// update spikes
		for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
			Spike *spike = buffer->spike_buffer_[sind];
			if (spike->tetrode_ != target_tetrode_)
				continue;

			if (spike -> cluster_id_ == c2){
				spike->cluster_id_ = -1;
			}
			else if (spike->cluster_id_ > c2){
				spike->cluster_id_ --;
			}
		}

		user_context_.DelleteCluster(buffer->cells_[target_tetrode_][c2].polygons_);

		// move polygon clusters up
		for(unsigned int i = c2; i < buffer->cells_[target_tetrode_].size() - 1; ++i){
			buffer->cells_[target_tetrode_][i] = buffer->cells_[target_tetrode_][i + 1];
		}
		buffer->cells_[target_tetrode_].erase(buffer->cells_[target_tetrode_].begin() + buffer->cells_[target_tetrode_].size() - 1);

		buffer->ResetPopulationWindow();
	}

	buffer->dumpCluAndRes(true);
}

void SDLPCADisplayProcessor::clearPolygon() {
	polygon_closed_ = false;
	polygon_x_.clear();
	polygon_y_.clear();
}

void SDLPCADisplayProcessor::extractClusterFromMultiple() {
	if (user_context_.SelectedCluster2() == -1){
		Log("No cluster selected for extraction");
	}

	unsigned int clun = 0;
	PolygonCluster new_clust_ = createNewCluster(clun);

	buffer->Log("Created new polygon cluster, total clusters = ", (int)buffer->cells_[target_tetrode_].size());
	save_polygon_clusters();

	clearPolygon();
	buffer->ResetAC(target_tetrode_, clun);

	buffer->ResetPopulationWindow();

	// not to redraw: iterate through spikes and redraw
//	SDL_SetRenderTarget(renderer_, nullptr);
//	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_SetRenderTarget(renderer_, texture_);
	SetDrawColor(clun);
	for (unsigned int s=0; s < buffer->spike_buf_pos_unproc_; ++s){
		Spike *spike = buffer->spike_buffer_[s];
		if (spike->discarded_ || spike->tetrode_ != target_tetrode_ || spike->cluster_id_ == -1 || !display_cluster_[spike->cluster_id_]){
			continue;
		}

		if (new_clust_.Contains(spike)){
			int x, y;
			getSpikeCoords(spike, x, y);
			spike->cluster_id_ = clun;
			//SDL_RenderDrawPoint(renderer_, x, y);
			points_.push_back({x, y});
		}
	}
	SDL_RenderDrawPoints(renderer_, &points_[0], points_.size());

	Render();

	buffer->dumpCluAndRes(true);
}

void SDLPCADisplayProcessor::extractCluster() {
	if (user_context_.SelectedCluster2() == -1){
		Log("No cluster selected for extraction");
		return;
	}

	unsigned int clun = 0;
	PolygonCluster new_clust_ = createNewCluster(clun);

	buffer->Log("Created new polygon cluster, total clusters = ", (int)buffer->cells_[target_tetrode_].size());
	save_polygon_clusters();

	clearPolygon();
//	buffer->ResetAC(target_tetrode_, clun);

	user_context_.CreateClsuter((int)clun, new_clust_.projections_inclusive_[0]);

	buffer->ResetPopulationWindow();

	// not to redraw: iterate through spikes and redraw
//	SDL_SetRenderTarget(renderer_, nullptr);
//	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_SetRenderTarget(renderer_, texture_);
	SetDrawColor(clun);

	std::vector<unsigned int> affected_clusters_;

	for (unsigned int s=0; s < buffer->spike_buf_pos_unproc_; ++s){
		Spike *spike = buffer->spike_buffer_[s];
		if (spike->discarded_ || spike->tetrode_ != target_tetrode_ || (spike->cluster_id_ != user_context_.SelectedCluster2())){
			continue;
		}

		if (new_clust_.Contains(spike)){
			int x, y;
			getSpikeCoords(spike, x, y);
			spike->cluster_id_ = clun;
			//SDL_RenderDrawPoint(renderer_, x, y);
			points_.push_back({x, y});
			affected_clusters_.push_back(s);
		}
	}
	SDL_RenderDrawPoints(renderer_, &points_[0], points_.size());

	user_context_.AddExclusiveProjection(new_clust_.projections_inclusive_[0], affected_clusters_);

	Render();

	buffer->dumpCluAndRes(true);
}

void SDLPCADisplayProcessor::addExclusiveProjection() {
	PolygonClusterProjection tmpproj(polygon_x_, polygon_y_, comp1_, comp2_);

	if (user_context_.SelectedCluster2() > -1){
		std::vector<unsigned int> affected_ids_;

		// TODO either implement polygon intersection or projections logical operations
		for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
			Spike *spike = buffer->spike_buffer_[sind];
			if (spike->tetrode_ != target_tetrode_)
				continue;



			if (spike -> cluster_id_ == user_context_.SelectedCluster2()){
				float rawx = spike->getFeature(comp1_); //spike->pc[comp1_ % nchan_][comp1_ / nchan_];
				float rawy = spike->getFeature(comp2_);; //spike->pc[comp2_ % nchan_][comp2_ / nchan_];

				if (tmpproj.Contains(rawx, rawy)){
					spike->cluster_id_ = -1;
					affected_ids_.push_back(sind);
				}
			}
		}

		polygon_closed_ = false;
		polygon_x_.clear();
		polygon_y_.clear();

		if (!memory_less_clustering_)
			buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_.projections_exclusive_.push_back(tmpproj);

		user_context_.AddExclusiveProjection(tmpproj, affected_ids_);

		// reset AC
		//buffer->ResetAC(target_tetrode_, user_context_.SelectedCluster2());

		// needed here?
		buffer->ResetPopulationWindow();

		buffer->dumpCluAndRes(false);
	}
}

PolygonCluster SDLPCADisplayProcessor::createNewCluster(unsigned int & clun){
	PolygonCluster new_clust_ = PolygonCluster(PolygonClusterProjection(polygon_x_, polygon_y_, comp1_, comp2_));
	clun = (unsigned int)user_context_.CreateClsuter(buffer->cells_[target_tetrode_].size(), new_clust_.projections_inclusive_[0]);

	// push back new or replace cluster invalidated before
	if (clun == buffer->cells_[target_tetrode_].size()){
		if (memory_less_clustering_){
			// dummy empty clusters
			buffer->cells_[target_tetrode_].push_back(PolygonCluster());
		} else {
			buffer->cells_[target_tetrode_].push_back(new_clust_);
		}
	}
	else{
		if (memory_less_clustering_){
			// dummy empty clusters
			buffer->cells_[target_tetrode_][clun] = PutativeCell();
		} else {
			buffer->cells_[target_tetrode_][clun] = PutativeCell(new_clust_);
		}
	}

	return new_clust_;
}

void SDLPCADisplayProcessor::addCluster() {
	unsigned int clun = 0;
	PolygonCluster new_clust_ = createNewCluster(clun);

	buffer->Log("Created new polygon cluster, total clusters = ", (int)buffer->cells_[target_tetrode_].size());
	save_polygon_clusters();

	clearPolygon();

	buffer->ResetAC(target_tetrode_, clun);

	buffer->ResetPopulationWindow();

	// not to redraw: iterate through spikes and redraw
//	SDL_SetRenderTarget(renderer_, nullptr);
//	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_SetRenderTarget(renderer_, texture_);
	SetDrawColor(clun);
	for (unsigned int s=0; s < buffer->spike_buf_pos_unproc_; ++s){
		Spike *spike = buffer->spike_buffer_[s];
		if (spike->discarded_ || spike->tetrode_ != target_tetrode_ || spike->cluster_id_ >= 0){
			continue;
		}

		if (new_clust_.Contains(spike)){
			int x, y;
			getSpikeCoords(spike, x, y);
			spike->cluster_id_ = clun;
			//SDL_RenderDrawPoint(renderer_, x, y);
			points_.push_back({x, y});
		}
	}
	SDL_RenderDrawPoints(renderer_, &points_[0], points_.size());

	Render();

	buffer->dumpCluAndRes(true);
}

void SDLPCADisplayProcessor::mergeClusters() {
	// keep the cluster with the lowers numbers
	if (user_context_.SelectedCluster1() > user_context_.SelectedCluster2()){
		int swap = user_context_.SelectedCluster1();
		user_context_.SelectCluster1(user_context_.SelectedCluster2());
		user_context_.SelectCluster2(swap);
	}

	const int c1 = user_context_.SelectedCluster1();
	const int c2 = user_context_.SelectedCluster2();

	// copy projections
	// TODO ??? exclusive as well ? (can affect other clusters)
	buffer->cells_[target_tetrode_][user_context_.SelectedCluster1()].polygons_.projections_inclusive_.insert(buffer->cells_[target_tetrode_][user_context_.SelectedCluster1()].polygons_.projections_inclusive_.end(),
			buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_.projections_inclusive_.begin(), buffer->cells_[target_tetrode_][user_context_.SelectedCluster2()].polygons_.projections_inclusive_.end());
	buffer->cells_[target_tetrode_][user_context_.SelectedCluster1()].waveshape_cuts_.clear();

	SDL_SetRenderTarget(renderer_, texture_);
	SetDrawColor(c1);
	std::vector<unsigned int> affected_ids;
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		Spike *spike = buffer->spike_buffer_[sind];
		if (spike->tetrode_ != target_tetrode_)
			continue;

		if (spike->cluster_id_ == c2){
			affected_ids.push_back(sind);
			spike->cluster_id_ = c1;
			int x,y;
			getSpikeCoords(spike, x, y);
			points_.push_back({x, y});
		} else {
			// shift cluster numbers up
			if (spike->cluster_id_ > c2){
				spike->cluster_id_ --;
			}
		}
	}

	// remove cluster from list of tetrode poly clusters
	user_context_.MergeClusters(buffer->cells_[target_tetrode_][c1].polygons_, buffer->cells_[target_tetrode_][c2].polygons_, affected_ids);
	buffer->ResetPopulationWindow();

	// move polygon clusters up
	for(unsigned int i = c2; i < buffer->cells_[target_tetrode_].size() - 1; ++i){
		buffer->cells_[target_tetrode_][i] = buffer->cells_[target_tetrode_][i + 1];
	}
	buffer->cells_[target_tetrode_].erase(buffer->cells_[target_tetrode_].begin() + buffer->cells_[target_tetrode_].size() - 1);

	SDL_RenderDrawPoints(renderer_, &points_[0], points_.size());
	Render();

	buffer->dumpCluAndRes(true);
}

void SDLPCADisplayProcessor::displayChiHistogramm(){
	if (user_context_.SelectedCluster1() <= 0){
		Log("No cluster selected");
		return;
	}

	// collect points for selected cluster
	arma::fmat clumat(1, 8);
	// index of spike in the matrix
	unsigned int si = 0;

	int clu = user_context_.SelectedCluster1();
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
			Spike *spike = buffer->spike_buffer_[sind];
			if (spike->tetrode_ != target_tetrode_ || spike->cluster_id_ != clu){
				continue;
			}

			// add spike to observations matrix
			for (unsigned int pi = 0; pi < 8; ++pi){
				clumat(si, pi) = spike->pc[pi];
			}

			si ++;
			if (si >= clumat.n_rows){
				clumat.resize(clumat.n_rows * 2, clumat.n_cols);
			}
	}

	// calculate mean / covariance
	clumat.resize(si, clumat.n_cols);
	arma::fmat cov = arma::cov(clumat);
	arma::fmat covi = cov.i();
	arma::fmat mean = arma::mean(clumat);

	// calculate gaussian distances
	std::vector<float> distances;
	for (unsigned int s = 0; s < si; ++s){
		arma::fmat diff = clumat.row(s) - mean;
		arma::fmat dist = -0.5 * diff * covi * diff.t();
		float distance = /*pow(2*M_PI, -8/2) * 1.0 / sqrt(arma::det(cov)) */ exp(dist(0,0));
		distances.push_back(distance);
		std::cout << distance << " ";
	}

	// display histogramm with feedback
//	arma::uvec hist = arma::hist(arma::fmat(distances), 100);
//	arma::uvec hist = arma::hist(arma::fmat(distances), arma::linspace<arma::vec>(0, 1, 20), 0);
	unsigned int NBIN = 20;
	std::vector<unsigned int> counts(NBIN);
	double step = 1.0 / NBIN;
	for (unsigned int di =0; di < distances.size(); ++di){
		unsigned int c = std::max<unsigned int>(0, std::min<unsigned int>((unsigned int)(distances[di] / step), NBIN - 1));
		counts[c] ++;
	}

	// reassign points (split cluster)
//	std::cout << hist;
	std::cout << "\n";
	for (unsigned int i=0; i < NBIN; ++i){
		std::cout << counts[i] << " ";
	}
	std::cout << "\n";

	// second pass: separate spikes with high distance ...
	// TODO: do few more iteration of gaussian fitting of only closest ones ...
	unsigned int clun = 0;
	createNewCluster(clun);
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
			Spike *spike = buffer->spike_buffer_[sind];
			if (spike->tetrode_ != target_tetrode_ || spike->cluster_id_ != clu){
				continue;
			}

			// add spike to observations matrix
			arma::fmat spikevec(1, 8);
			for (unsigned int pi = 0; pi < 8; ++pi){
				spikevec(0, pi) = spike->pc[pi];
			}

			arma::fmat diff = spikevec - mean;
			arma::fmat dist = -0.5 * diff * covi * diff.t();
			float distance = /*pow(2*M_PI, -8/2) * 1.0 / sqrt(arma::det(cov)) */ exp(dist(0,0));
			if (distance < 0.0005){
				spike->cluster_id_ = clun;
			}
	}

	buffer->dumpCluAndRes(true);
}

void SDLPCADisplayProcessor::splitIntoGuassians(){
	// TODO: extract and share with oulier separation
	if (user_context_.SelectedCluster1() <= 0){
		Log("No cluster selected");
		return;
	}

	// collect points for selected cluster
	arma::mat clumat(1, 8);
	// index of spike in the matrix
	unsigned int si = 0;

	int clu = user_context_.SelectedCluster1();
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		Spike *spike = buffer->spike_buffer_[sind];
		if (spike->tetrode_ != target_tetrode_ || spike->cluster_id_ != clu){
			continue;
		}

		// add spike to observations matrix
		for (unsigned int pi = 0; pi < 8; ++pi){
			clumat(si, pi) = spike->pc[pi];
		}

		si ++;
		if (si >= clumat.n_rows){
			clumat.resize(clumat.n_rows * 2, clumat.n_cols);
		}
	}

	mlpack::gmm::GMM gmm2(2, 8);
	//gmm2.Estimate(clumat.t());
	arma::Col<size_t> labels_;
	//	gmm2.Classify(clumat.t(), labels_);

	// second pass: split into 2
	// TODO: keep reference to spikes for faster access
	unsigned int clun = 0;
	unsigned int si2 = 0;
	createNewCluster(clun);
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		Spike *spike = buffer->spike_buffer_[sind];
		if (spike->tetrode_ != target_tetrode_ || spike->cluster_id_ != clu){
			continue;
		}

		if (labels_[si2] > 0){
			spike->cluster_id_ = clun;
		}

		si2 ++;
	}

	buffer->dumpCluAndRes(true);
}


void SDLPCADisplayProcessor::clusterNNwithThreshold(){
	// 0. Count number of clustered spikes per every tetrode
	const unsigned int NTETR = buffer->tetr_info_->tetrodes_number();
	std::vector<unsigned int> nclustered;
	nclustered.resize(NTETR);
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		if (buffer->spike_buffer_[sind]->discarded_){
			continue;
		}

		unsigned int t = buffer->spike_buffer_[sind]->tetrode_;
		unsigned int c = buffer->spike_buffer_[sind]->cluster_id_;

		if (c > 0){
			nclustered[t] ++;
		}
	}

	// 1. Build KD tree containing all clustered spikes + create cluster map
	std::vector<ANNkd_tree*> kdtrees_;
	std::vector<ANNpointArray> ann_points_;
	std::vector<unsigned int> current_points;
	std::vector<std::vector<unsigned int> > treePointClusters;

	kdtrees_.resize(NTETR);
	ann_points_.resize(NTETR);
	current_points.resize(NTETR);
	treePointClusters.resize(NTETR);

	for (unsigned int t=0; t < NTETR; ++t){
		ann_points_[t] = annAllocPts(nclustered[t], buffer->feature_space_dims_[t]);
		treePointClusters[t].resize(nclustered[t]);
	}

	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		Spike *spike = buffer->spike_buffer_[sind];
		if (spike->discarded_){
			continue;
		}

		int t = spike->tetrode_;
		int c = spike->cluster_id_;

		if (c < 1){
			continue;
		}

		for (unsigned int fet = 0; fet < buffer->feature_space_dims_[t]; ++fet) {
			ann_points_[t][current_points[t]][fet] = spike->pc[fet];
		}
		treePointClusters[t][current_points[t]] = c;
		current_points[t] ++;
	}

	for (unsigned int t=0; t < NTETR; ++t){
		kdtrees_[t] = new ANNkd_tree(ann_points_[t], current_points[t], buffer->feature_space_dims_[t]);
		Log("Points in tree per teterode: ", current_points[t]);
	}

	Log("Trees built, start clustering");

	const unsigned int NEIGHB_NUM = 20;
	double *pnt_ = annAllocPt(12);
	std::vector<int> neighbour_inds_;
	std::vector<double> neighbour_dists_;
	neighbour_dists_.resize(NEIGHB_NUM);
	neighbour_inds_.resize(NEIGHB_NUM);

	// DEBUG
	std::vector<std::ofstream*> fdists;
		for (unsigned int t=0; t < NTETR; ++t){
		fdists.push_back(new std::ofstream(std::string("clu_dists_") + Utils::Converter::int2str(t)));
	}

	// 2. Cluster all spikes
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		//kdtrees_[stetr]->annkSearch(pnt_, neighb_num_, &neighbour_inds_[0], &neighbour_dists_[0], NN_EPS);
		Spike *spike = buffer->spike_buffer_[sind];

		if (spike->discarded_ || spike->cluster_id_ > 0 || current_points[spike->tetrode_] < 10 ||
				spike->pkg_id_ < buffer->session_shifts_[2] || spike->pkg_id_ > buffer->session_shifts_[3]){
			continue;
		}

		std::map<int, int> vote_clust_;

		int t = spike->tetrode_;
		for (unsigned int fet = 0; fet < buffer->feature_space_dims_[t]; ++fet) {
			pnt_[fet] = spike->pc[fet];
		}
		kdtrees_[t]->annkSearch(pnt_, NEIGHB_NUM, &neighbour_inds_[0], &neighbour_dists_[0], 0.1);
//		std::cout << neighbour_dists_[0] << " ";

		// vote with nearest neighbours
		for (unsigned int nn=0; nn < NEIGHB_NUM; ++nn){
			if (neighbour_dists_[nn] < 100 && treePointClusters[t][neighbour_inds_[nn]] > 0){
				vote_clust_[treePointClusters[t][neighbour_inds_[nn]]] += 1;
			}
		}

		int currentMax = 0;
		int arg_max = -1;
		for(auto it = vote_clust_.cbegin(); it != vote_clust_.cend(); ++it ) {
		    if (it ->second > currentMax) {
		        arg_max = it->first;
		        currentMax = it->second;
		    }
		}

		if (arg_max > -1){
			spike->cluster_id_ = arg_max;
		}

		*(fdists[t]) << neighbour_dists_[0] << "\n";
	}

	Log("\nDone NN clustering");
}
