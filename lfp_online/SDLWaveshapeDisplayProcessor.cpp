//
//  SDLWaveshapeDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 12/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "SDLWaveshapeDisplayProcessor.h"

SDLWaveshapeDisplayProcessor::SDLWaveshapeDisplayProcessor(LFPBuffer *buf)
:SDLWaveshapeDisplayProcessor(buf,
		buf->config_->getString("waveshapedisp.window.name"),
		buf->config_->getInt("waveshapedisp.window.width"),
		buf->config_->getInt("waveshapedisp.window.height")
		){
	buf_pointer_ = buf->SPIKE_BUF_HEAD_LEN;
}

SDLWaveshapeDisplayProcessor::SDLWaveshapeDisplayProcessor(LFPBuffer *buf, const std::string& window_name,
		const unsigned int& window_width,
    		const unsigned int& window_height)
	: LFPProcessor(buf)
	, SDLControlInputProcessor(buf)
    , SDLSingleWindowDisplay(buf, window_name, window_width, window_height)
	, scale_((float)buf->config_->getInt("waveshapedisp.scale", 25))
	, spike_plot_rate_(buf->config_->getInt("waveshapedisp.spike.plot.rate", 10))
	, DISPLAY_SIZE(buf->config_->getInt("waveshapedisp.display.size", 1))
	, display_final_(buf->config_->getBool("waveshapedisp.final", false))
	, buf_pointer_(buf->spike_buf_pos_ws_disp_)
	, cuts_file_path_(buf->config_->getOutPath("waveshapedisp.cuts.path"))
	, cuts_save_(buf->config_->getBool("waveshapedisp.cuts.save", false))
	, cuts_load_(buf->config_->getBool("waveshapedisp.cuts.load", false))
	, on_demand_(!buf->config_->getBool("spike.reader.spk.read", false))
{
	// LOAD CUTS FROM FILE
	if (cuts_load_){
		if (!Utils::FS::FileExists(cuts_file_path_)){
			Log(std::string("WARNING: Cuts file not found: ") + cuts_file_path_);
			return;
		}

		std::ifstream cuts_file_(cuts_file_path_);
		int cn, x1, x2, y1, y2;
		unsigned int chan, t;
		int waveshapeCutType;
		while (!cuts_file_.eof()){
			cuts_file_ >> t >> cn >> x1 >> y1 >> x2 >> y2 >> chan >> waveshapeCutType;
			buffer->AddWaveshapeCut(t, cn, WaveshapeCut(x1, y1, x2, y2, chan, (WaveshapeType)waveshapeCutType));
		}
		cuts_file_.close();
	}
}

void SDLWaveshapeDisplayProcessor::saveCuts(){
	if (cuts_save_){
		std::ofstream cuts_file_(cuts_file_path_);

		for (unsigned int t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
			for (unsigned int c=0; c < buffer->cells_[t].size(); ++c){
				for (unsigned int cut=0; cut < buffer->cells_[t][c].waveshape_cuts_.size(); ++cut){
					WaveshapeCut & wcut = buffer->cells_[t][c].waveshape_cuts_[cut];
					cuts_file_ << t << " " << c << " " << wcut.x1_ << " " << wcut.y1_ << " " << wcut.x2_ << " " << wcut.y2_ << " " << wcut.channel_ << " " << (int)wcut.waveshapeType_ << "\n";
				}
			}
		}
	}
}

float SDLWaveshapeDisplayProcessor::XToWaveshapeSampleNumber(int x) {
	if (display_final_){
		return x / (float)x_mult_final_;
	}
	else
	{
		return x / (float)x_mult_reconstructed_;
	}
}

float SDLWaveshapeDisplayProcessor::YToPower(int chan, int y) {
	return -(y - 100 - (float)y_mult_ * chan) * scale_;
}

void SDLWaveshapeDisplayProcessor::displayClusterCuts(const int & cluster_id, int highlight_number) {
	if (cluster_id < 1 || (int)buffer->cells_[targ_tetrode_].size() <= cluster_id)
		return;

	for (size_t cu = 0; cu < buffer->cells_[targ_tetrode_][cluster_id].waveshape_cuts_.size(); ++cu) {
		WaveshapeCut& cut = buffer->cells_[targ_tetrode_][cluster_id].waveshape_cuts_[cu];

		float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

		x1 = cut.x1_ * ((cut.waveshapeType_ == WaveshapeTypeOriginal) ? x_mult_final_ : x_mult_reconstructed_);
		x2 = cut.x2_ * ((cut.waveshapeType_ == WaveshapeTypeOriginal) ? x_mult_final_ : x_mult_reconstructed_);
		y1 = (- cut.y1_ / scale_) + 100 + (float)y_mult_ * cut.channel_;
		y2 = (- cut.y2_ / scale_) + 100 + (float)y_mult_ * cut.channel_;

		DrawCross(2, x1, y1);
		DrawCross(2, x2, y2);

		if (highlight_number == (int)cu){
			SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
		}
		SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
	}
}

void SDLWaveshapeDisplayProcessor::reinit() {
	RenderClear();
	buf_pointer_ = 0;

	// calculate sampling rate
	unsigned int n_clus_1_ = 0;
	unsigned int n_clus_2_ = 0;
    const int& disp_cluster_1_ = user_context_.SelectedCluster1();
    const int& disp_cluster_2_ = user_context_.SelectedCluster2();

    // DEBUG
    //Log("Reinit, display cluster 2 = ", disp_cluster_2_);

    // nodisp_pca can be reset => not useful for display rate calculation
	for (unsigned int si=0; si < buffer->spike_buf_pos; ++si){
		Spike* s = buffer->spike_buffer_[si];

        if (s->discarded_ || s->tetrode_ != (int)targ_tetrode_){
			continue;
        }

        if (s->cluster_id_ == disp_cluster_1_){
        	n_clus_1_ ++;
        }
        if (s->cluster_id_ == disp_cluster_2_){
            n_clus_2_ ++;
        }
	}


	display_rate_1_ = n_clus_1_ > 0 ? n_clus_1_ / DISPLAY_SIZE + 1 : 1;
	display_rate_2_ = n_clus_2_ > 0 ? n_clus_2_ / DISPLAY_SIZE + 1 : 1;

	c1_total_ = 0;
	c2_total_ = 0;
	c1_total_ = c1_prev_;
	c2_total_ = c2_prev_;

	// DEBUG
	//Log("Clu 1 display rate: ", display_rate_1_);
	//Log("Clu 2 display rate: ", display_rate_2_);
}

float SDLWaveshapeDisplayProcessor::transform(float smpl, int chan){
    return 100 + -smpl/scale_ + y_mult_ * chan;
}

void SDLWaveshapeDisplayProcessor::process() {
	//quick and dirty: wait for nullptr in buffer->chunk
	if (buffer->pipeline_status_ == PIPELINE_STATUS_READ_FET){
		return;
	}

    // 4 channel waveshape, shifted; colour by cluster, if available (use palette)
    
    int last_pkg_id = 0;
    
    SDL_SetRenderTarget(renderer_, texture_);

    const int& disp_cluster_1_ = user_context_.SelectedCluster1();
    const int& disp_cluster_2_ = user_context_.SelectedCluster2();
    
    while(user_context_.HasNewAction(last_ua_id_)){
    	const UserAction *ua = user_context_.GetNextAction(last_ua_id_);
    	last_ua_id_ ++;

    	switch(ua->action_type_){
    		case UA_SELECT_CLUSTER1:
    			reinit();
    			break;

    		case UA_SELECT_CLUSTER2:
    			reinit();
    			break;

    		case UA_NONE:
    			break;

    		case UA_CREATE_CLUSTER:
    			break;

    		case UA_MERGE_CLUSTERS:
    			break;

    		case UA_CUT_SPIKES:
    			break;

    		case UA_DELETE_CLUSTER:
    			break;

    		case UA_ADD_INCLUSIVE_PROJECTION:
    			break;

    		case UA_ADD_EXCLUSIVE_PROJECTION:
    			break;

    		case UA_REMOVE_PROJECTION:
    			break;
    	}
    }

    while(buf_pointer_ < buffer->spike_buf_no_disp_pca){
        Spike *spike = buffer->spike_buffer_[buf_pointer_];
        
        if (spike->waveshape == nullptr){
        	buf_pointer_++;
			continue;
        }

		if ((unsigned int)spike->tetrode_ != targ_tetrode_ || spike->cluster_id_<=0 ||
				(spike->cluster_id_ != disp_cluster_1_ && spike->cluster_id_ != disp_cluster_2_) ||
				spike->discarded_){
            buf_pointer_++;
            continue;
        }

		if (spike->cluster_id_ == disp_cluster_1_){
			c1_total_ ++;
			if (c1_total_ % display_rate_1_){
	            buf_pointer_++;
				continue;
			}
		}
		if (spike->cluster_id_ == disp_cluster_2_){
			c2_total_ ++;
			if (c2_total_ % display_rate_2_){
	            buf_pointer_++;
				continue;
			}
		}

		if (display_final_){
			drawWS(spike, spike->waveshape_final);
		}
		else{
			drawWS(spike, spike->waveshape);
		}
        
        last_pkg_id = spike->pkg_id_;
        buf_pointer_++;
    }
    
    if (last_pkg_id - last_disp_pkg_id_ > DISPLAY_SIZE){
        last_disp_pkg_id_ = last_pkg_id;
        
        if (x1_ > 0){
        	DrawCross(3, x1_, y1_);
        }

        if (x2_ > 0){
        	DrawCross(3, x2_, y2_);
        	SDL_RenderDrawLine(renderer_, x1_, y1_, x2_, y2_);
        }

        DrawRect(0, window_height_* 12/13 / buffer->tetr_info_->channels_number(targ_tetrode_) * selected_channel_, window_width_, window_height_*12/13 / buffer->tetr_info_->channels_number(targ_tetrode_), 5);

        // draw cuts
        // TODO 2 colors cuts for 2 clusters
        displayClusterCuts(disp_cluster_1_, -1);
        displayClusterCuts(disp_cluster_2_, current_cut_);

        Render();
    }
}

template<class T>
void SDLWaveshapeDisplayProcessor::drawWSClu(int clu, T ws){
	int x_scale = display_final_ ? x_mult_final_ : x_mult_reconstructed_; // for final wave shapes
	for (int chan=0; chan < ws.size(); ++chan) {
		int prev_smpl = (int)transform((float)ws[chan][0], chan);
		for (int smpl = 1; smpl < 128; ++smpl) {
			int tsmpl = (int)transform((float)ws[chan][smpl], chan);
			SDL_SetRenderDrawColor(renderer_, colpal.getR(clu) ,colpal.getG(clu), colpal.getB(clu),255);
			SDL_RenderDrawLine(renderer_, smpl * x_scale - (x_scale - 1), prev_smpl, smpl * x_scale + 1, tsmpl);
			prev_smpl = tsmpl;
		}
	}
}

template<class T>
void SDLWaveshapeDisplayProcessor::drawWS(Spike* spike, T ws){
	int x_scale = display_final_ ? x_mult_final_ : x_mult_reconstructed_; // for final wave shapes
	for (int chan=0; chan < spike->num_channels_; ++chan) {
		int prev_smpl = (int)transform((float)ws[chan][0], chan);
		for (int smpl = 1; smpl < 128; ++smpl) {
			int tsmpl = (int)transform((float)ws[chan][smpl], chan);
			SDL_SetRenderDrawColor(renderer_, colpal.getR(spike->cluster_id_) ,colpal.getG(spike->cluster_id_), colpal.getB(spike->cluster_id_),255);
			SDL_RenderDrawLine(renderer_, smpl * x_scale - (x_scale - 1), prev_smpl, smpl * x_scale + 1, tsmpl);
			prev_smpl = tsmpl;
		}
	}
}

void SDLWaveshapeDisplayProcessor::displayFromFiles(){
    int disp_cluster_1 = user_context_.SelectedCluster1();
    int disp_cluster_2 = user_context_.SelectedCluster2();

    std::ifstream fws;
    ws_type **ws_tmp;
	unsigned int sp_tetr = 0;

	unsigned int current_session = 0;
	unsigned int spike_shift = 0;

	SDL_SetRenderTarget(renderer_, texture_);

	ws_tmp = new ws_type*[4];
	for (unsigned int c=0; c < 4; ++c){
		ws_tmp[c] = new ws_type[128];
	}

	unsigned int summed1=0, summed2=0;
	std::vector<std::vector<long int> > sum_c1, sum_c2;
	unsigned int channel_number = buffer->tetr_info_->channels_number(targ_tetrode_);
	sum_c1.resize(channel_number);
	sum_c2.resize(channel_number);
	for (unsigned int c = 0; c < channel_number; ++c){
		sum_c1[c].resize(128);
		sum_c2[c].resize(128);
	}

	// open first session streams for given tetrode
	fws.open(buffer->config_->spike_files_[0] + "spkb." + Utils::NUMBERS[buffer->config_->tetrodes[0]]);
	// iterate over all spikes, seek and read if need to draw

	Spike* spike = NULL;
	for (unsigned int s=0; s < buffer->spike_buf_pos; ++s){
		spike = buffer->spike_buffer_[s];
		if (spike->tetrode_ != (int)targ_tetrode_){
			continue;
		}

		// check if need to open new file - first spike in the given tetrode from the new session !
		if (current_session < buffer->session_shifts_.size() - 1 && spike->pkg_id_ > buffer->session_shifts_[current_session + 1]){
			spike_shift = sp_tetr;
			current_session ++;
			fws.close();
			fws.open(buffer->config_->spike_files_[current_session] + "spkb." + Utils::NUMBERS[buffer->config_->tetrodes[0]]);
		}

		if (spike->cluster_id_<=0 ||
				(spike->cluster_id_ != disp_cluster_1 && spike->cluster_id_ != disp_cluster_2) ||
				spike->discarded_){
			sp_tetr ++;
			continue;
		}

		if (spike->cluster_id_ == disp_cluster_1){
			c1_total_ ++;
			if (c1_total_ % display_rate_1_){
				sp_tetr ++;
				continue;
			}
		}
		if (spike->cluster_id_ == disp_cluster_2){
			c2_total_ ++;
			if (c2_total_ % display_rate_2_){
				sp_tetr ++;
				continue;
			}
		}

		// now need to read and draw current
		fws.seekg(256 * buffer->tetr_info_->channels_number(targ_tetrode_) * (sp_tetr - spike_shift), fws.beg);
		for (unsigned int c=0; c < buffer->tetr_info_->channels_number(targ_tetrode_); ++c){
			fws.read((char*)ws_tmp[c], 128 * sizeof(ws_type));
		}

		if(spike->cluster_id_ == disp_cluster_1){
			for(unsigned int ipc = 0; ipc<channel_number; ++ipc){
				for (unsigned int iws = 0; iws < 128; ++iws){
					sum_c1[ipc][iws] += ws_tmp[ipc][iws];
				}
			}
			summed1 ++;
		}else{
			for(unsigned int ipc = 0; ipc<channel_number; ++ipc){
				for (unsigned int iws = 0; iws < 128; ++iws){
					sum_c2[ipc][iws] += ws_tmp[ipc][iws];
				}
			}
			summed2 ++;
		}

		sp_tetr ++;
		if (!mean_mode_)
			drawWS(spike, ws_tmp);
	}

	for(unsigned int ipc = 0; ipc<channel_number; ++ipc){
		// std::cout << "\nChannel " << ipc << "\n";
		for (unsigned int iws = 0; iws < 128; ++iws){
			if (summed1 > 0)
				sum_c1[ipc][iws] /= summed1;
			if (summed2 > 0)
				sum_c2[ipc][iws] /= summed2;

			// std::cout << sum_c1[ipc][iws] << " " << sum_c2[ipc][iws] << " ";
		}
	}

	drawWSClu(disp_cluster_1, sum_c1);
	drawWSClu(disp_cluster_2, sum_c2);

	Render();

	for (unsigned int c=0; c < 4; ++c){
		delete ws_tmp[c];
	}
	delete ws_tmp;
}

void SDLWaveshapeDisplayProcessor::process_SDL_control_input(const SDL_Event& e){
	SDL_Keymod kmod = SDL_GetModState();
	bool need_redraw = false;

	if( e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
		// update
		SDL_RenderPresent(renderer_);
		return;
	}

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.windowID == GetWindowID()){
			if (e.button.button == SDL_BUTTON_LEFT){
				// select cluster
				if (kmod & KMOD_LCTRL){
					if (x1_ == -1){
						x1_ = e.button.x;
						y1_ = e.button.y;
					}
					else{
						x2_ = e.button.x;
						y2_ = e.button.y;
					}

					need_redraw = true;
				}
			}
	}

	if( e.type == SDL_KEYDOWN )
    {
		need_redraw = true;
        SDL_Keymod kmod = SDL_GetModState();
        
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift = 10;
        }

        int disp_cluster_1 = user_context_.SelectedCluster1();
        int disp_cluster_2 = user_context_.SelectedCluster2();

        switch( e.key.keysym.sym )
        {
        	// on-demand waveshape display
        	case SDLK_w: {
        		if (on_demand_)
        			displayFromFiles();
        		need_redraw = false;
        		break;
        	}

        	// delete points
        	case SDLK_d:
        		x1_ = -1;
        		y1_ = -1;
        		x2_ = -1;
        		y2_ = -1;
        		break;

        	case SDLK_r:
        		//remove cut
        		if (current_cut_ >= 0){
        			buffer->DeleteWaveshapeCut(targ_tetrode_, user_context_.SelectedCluster2(), current_cut_);
        			need_redraw = true;
        			current_cut_ = -1;
        			buffer->spike_buf_no_disp_pca = 0;
        		}
        		break;

        	// clear cluster - remove spikes whose waveshape crosses line (x1,y1) -> (x2, y2)
        	case SDLK_c:
        		if (x2_ > 0)
        		{
        			// iterate through all spikes in buffer and assign 0 (artefact) clusters

        				if (x1_ < x2_){
        					int swap = x1_;
        					x1_ = x2_;
        					x2_ = swap;
        				}

        				float xw1 = XToWaveshapeSampleNumber(x1_);
        				float yw1 = YToPower(selected_channel_, y1_);
        				float xw2 = XToWaveshapeSampleNumber(x2_);
        				float yw2 = YToPower(selected_channel_, y2_);

        				// add to cuts list to cut coming spikes
        				if (user_context_.SelectedCluster2() > 0)
          					buffer->AddWaveshapeCut(targ_tetrode_, user_context_.SelectedCluster2(), WaveshapeCut(xw1, yw1, xw2, yw2, selected_channel_, (WaveshapeType)display_final_));

        				saveCuts();

        				// reset AC / CC
        				buffer->ResetAC(targ_tetrode_, user_context_.SelectedCluster1());

        			buf_pointer_ = 0;

        			x1_ = x2_ = y1_ = y2_ = -1;
        		}

        		break;

            case SDLK_ESCAPE:
            	buffer->processing_over_ = true;
                break;
                
            // change scale
            case SDLK_KP_PLUS:
            	scale_ /= 1.1f;
            	buf_pointer_ = 0;
            	if (on_demand_)
            		displayFromFiles();
            	break;
            case SDLK_KP_MINUS:
            	scale_ *= 1.1f;
            	buf_pointer_ = 0;
            	if (on_demand_)
            	    displayFromFiles();
            	break;

            // select cluster 1
            case SDLK_KP_0:
                disp_cluster_1 = 0 + shift;
                break;
            case SDLK_KP_1:
                disp_cluster_1 = 1 + shift;
                break;
            case SDLK_KP_2:
                disp_cluster_1 = 2 + shift;
                break;
            case SDLK_KP_3:
                disp_cluster_1 = 3 + shift;
                break;
            case SDLK_KP_4:
                disp_cluster_1 = 4 + shift;
                break;
            case SDLK_KP_5:
                disp_cluster_1 = 5 + shift;
                break;
            case SDLK_KP_6:
                disp_cluster_1 = 6 + shift;
                break;
            case SDLK_KP_7:
                disp_cluster_1 = 7 + shift;
                break;
            case SDLK_KP_8:
                disp_cluster_1 = 8 + shift;
                break;
            case SDLK_KP_9:
                disp_cluster_1 = 9 + shift;
                break;

                // select cluster 2
            case SDLK_0:
            	disp_cluster_2 = 0 + shift;
            	break;
            case SDLK_1:
            	if (kmod && KMOD_LCTRL != KMOD_NONE)
            		selected_channel_ = 0;
            	else
            		disp_cluster_2 = 1 + shift;
            	break;
            case SDLK_2:
            	if (kmod && KMOD_LCTRL != KMOD_NONE)
            		selected_channel_ = 1;
            	else
            		disp_cluster_2 = 2 + shift;
            	break;
            case SDLK_3:
            	if (kmod && KMOD_LCTRL != KMOD_NONE)
            		selected_channel_ = 2;
            	else
            		disp_cluster_2 = 3 + shift;
            	break;
            case SDLK_4:
            	if (kmod && KMOD_LCTRL != KMOD_NONE)
            	    selected_channel_ = 3;
            	else
            		disp_cluster_2 = 4 + shift;
            	break;
            case SDLK_5:
            	disp_cluster_2 = 5 + shift;
            	break;
            case SDLK_6:
            	disp_cluster_2 = 6 + shift;
            	break;
            case SDLK_7:
            	disp_cluster_2 = 7 + shift;
            	break;
            case SDLK_8:
            	disp_cluster_2 = 8 + shift;
            	break;
            case SDLK_9:
            	disp_cluster_2 = 9 + shift;
            	break;

            case SDLK_RIGHTBRACKET:
            	if (disp_cluster_2 < (int)buffer->cells_[targ_tetrode_].size() && buffer->cells_[targ_tetrode_][disp_cluster_2].waveshape_cuts_.size() > 0){
            		current_cut_ = (current_cut_ + 1) % buffer->cells_[targ_tetrode_][disp_cluster_2].waveshape_cuts_.size();
            		need_redraw = true;
            	}
            	break;

            case SDLK_n:
            	reinit();
            	++ c1_prev_;
            	++ c2_prev_;

            	if (on_demand_)
            		displayFromFiles();

            	break;

            case SDLK_m:
            	DISPLAY_SIZE *= 1.5;
            	Log("New display size: ", DISPLAY_SIZE);

            	if (on_demand_)
            		displayFromFiles();
            	else
            		need_redraw = true;

            	break;

            case SDLK_l:
            	DISPLAY_SIZE /= 1.5;
            	Log("New display size: ", DISPLAY_SIZE);

            	if (on_demand_)
            		displayFromFiles();
            	else
            		need_redraw = true;

            	break;

            case SDLK_z:
            	mean_mode_ = !mean_mode_;
            	if (on_demand_)
            		displayFromFiles();
            	break;

            default:
                need_redraw = false;
        }

        if (disp_cluster_1 != user_context_.SelectedCluster1()){
        	user_context_.SelectCluster1(disp_cluster_1);
        }
        if (disp_cluster_2 != user_context_.SelectedCluster2()){
        	user_context_.SelectCluster1(disp_cluster_2);
        	current_cut_ = -1;
        }
    }

    if (need_redraw){
        reinit();
    }
}

void SDLWaveshapeDisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    // duplicates functionality in process_SDL_control_input, but is supposed to be called in all displays simultaneously
    targ_tetrode_ = display_tetrode;
    current_cut_ = -1;
    RenderClear();
}

void SDLWaveshapeDisplayProcessor::Resize() {
	SDLSingleWindowDisplay::Resize();
	x_mult_final_ = (unsigned int) round(window_width_ / 16.25);
	x_mult_reconstructed_ = (unsigned int) round(window_width_ / 130);;
	// TODO properly adjust cluster-cut related usage (more space for last channel)
	y_mult_ = window_height_ * 3 / 13;
}
