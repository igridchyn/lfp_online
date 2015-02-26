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
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
	, scale_(buf->config_->getInt("waveshapedisp.scale", 25))
	, spike_plot_rate_(buf->config_->getInt("waveshapedisp.spike.plot.rate", 10))
	, DISPLAY_RATE(buf->config_->getInt("waveshapedisp.display.rate", 1))
	, display_final_(buf->config_->getBool("waveshapedisp.final", false))
	, buf_pointer_(buf->spike_buf_pos_ws_disp_)
{
	for (size_t t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
		cluster_cuts_.push_back(std::vector<std::vector<WaveshapeCut> >());
		cluster_cuts_[t].resize(MAX_CLUST);
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

void SDLWaveshapeDisplayProcessor::displayClusterCuts(const int cluster_id) {
	if (cluster_id < 1)
		return;

	for (size_t cu = 0; cu < cluster_cuts_[targ_tetrode_][cluster_id].size(); ++cu) {
		WaveshapeCut& cut = cluster_cuts_[targ_tetrode_][cluster_id][cu];
		DrawCross(2, cut.x1_, cut.y1_);
		DrawCross(2, cut.x2_, cut.y2_);
		SDL_RenderDrawLine(renderer_, cut.x1_, cut.y1_, cut.x2_, cut.y2_);
	}
}

void SDLWaveshapeDisplayProcessor::reinit() {
	SDL_SetRenderTarget(renderer_, texture_);
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);
	buf_pointer_ = 0;
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
    
    // POINTER ??? : after rewind preserve last valid position -> will allow autonomous pointer rewind (restarting from the beginning)
    //  depend on object properties (don't exceed main pointer), not on the previous pointer !!!
    // TODO: implement idea above; workaround: rewind if target pointer less than
    
    int last_pkg_id = 0;
    
    SDL_SetRenderTarget(renderer_, texture_);
    const ColorPalette& colpal = ColorPalette::BrewerPalette12;

    const unsigned int& disp_cluster_1_ = user_context_.SelectedCluster1();
    const unsigned int& disp_cluster_2_ = user_context_.SelectedCluster2();
    
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

    			// TODO: show in which dimensions cluster has projections
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
        
        // TODO warn ?
        if (spike == nullptr || spike->waveshape == nullptr){
        	buf_pointer_++;
			continue;
        }

        // !!! PLOTTING EVERY N-th spike
        // TODO: plot only one cluster [switch !!!]
		if ((unsigned int)spike->tetrode_ != targ_tetrode_ || spike->cluster_id_<=0 || ((unsigned int)spike->cluster_id_ != disp_cluster_1_ && (unsigned int)spike->cluster_id_ != disp_cluster_2_) || spike->discarded_ || (tetrode_total_spikes_ % spike_plot_rate_)){
            buf_pointer_++;
            tetrode_total_spikes_ ++;
            continue;
        }
        
		// check if falls under any cluster cuts
		bool is_cut = false;
		for(size_t c= 0; c < cluster_cuts_[targ_tetrode_][spike->cluster_id_].size(); ++c){
			// TODO save cut with channel and transform

			WaveshapeCut& cut = cluster_cuts_[targ_tetrode_][spike->cluster_id_][c];


			float xw1 = XToWaveshapeSampleNumber(cut.x1_);
			float yw1 = YToPower(cut.channel_, cut.y1_);
			float xw2 = XToWaveshapeSampleNumber(cut.x2_);
			float yw2 = YToPower(cut.channel_, cut.y2_);

			if (display_final_){
				if (spike->crossesWaveShapeFinal(cut.channel_, xw1, yw1, xw2, yw2)){
					spike->cluster_id_ = 0;
					is_cut = true;
					break;
				}
			}
			else{
				if (spike->crossesWaveShapeReconstructed(cut.channel_, xw1, yw1, xw2, yw2)){
					spike->cluster_id_ = 0;
					is_cut = true;
					break;
				}
			}

			if (is_cut)
				break;
		}
		if (is_cut){
			 buf_pointer_++;
			 tetrode_total_spikes_ ++;
			 continue;
		}

		int x_scale = display_final_ ? (8 * 4) : 4; // for final wave shapes
        for (int chan=0; chan < 4; ++chan) {
            int prev_smpl = transform(spike->waveshape[chan][0], chan);

			if (display_final_){
				for (int smpl = 1; smpl < 16; ++smpl) {
					int tsmpl = transform(spike->waveshape_final[chan][smpl], chan);
					SDL_SetRenderDrawColor(renderer_, colpal.getR(spike->cluster_id_) ,colpal.getG(spike->cluster_id_), colpal.getB(spike->cluster_id_),255);
					SDL_RenderDrawLine(renderer_, smpl * x_scale - (x_scale - 1), prev_smpl, smpl * x_scale + 1, tsmpl);
					prev_smpl = tsmpl;
				}
			}
			else{
				for (int smpl = 1; smpl < 128; ++smpl) {
					int tsmpl = transform(spike->waveshape[chan][smpl], chan);
					SDL_SetRenderDrawColor(renderer_, colpal.getR(spike->cluster_id_) ,colpal.getG(spike->cluster_id_), colpal.getB(spike->cluster_id_),255);
					SDL_RenderDrawLine(renderer_, smpl * x_scale - (x_scale - 1), prev_smpl, smpl * x_scale + 1, tsmpl);
					prev_smpl = tsmpl;
				}
			}
        }
        
        last_pkg_id = spike->pkg_id_;
        buf_pointer_++;
    }
    
    if (last_pkg_id - last_disp_pkg_id_ > DISPLAY_RATE){
        last_disp_pkg_id_ = last_pkg_id;
        
        if (x1_ > 0){
        	DrawCross(3, x1_, y1_);
        }

        if (x2_ > 0){
        	DrawCross(3, x2_, y2_);
        	SDL_RenderDrawLine(renderer_, x1_, y1_, x2_, y2_);
        }

        DrawRect(0, window_height_ / buffer->tetr_info_->channels_number(targ_tetrode_) * selected_channel_, window_width_, window_height_ / buffer->tetr_info_->channels_number(targ_tetrode_), 5);

        // draw cuts
        // TODO 2 colors cuts for 2 clusters
        displayClusterCuts(disp_cluster_1_);
        displayClusterCuts(disp_cluster_2_);

        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }
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
        if (kmod && KMOD_LSHIFT){
            shift = 10;
        }

        int disp_cluster_1 = user_context_.SelectedCluster1();
        int disp_cluster_2 = user_context_.SelectedCluster2();

        switch( e.key.keysym.sym )
        {
        	// delete points
        	case SDLK_d:
        		x1_ = -1;
        		y1_ = -1;
        		x2_ = -1;
        		y2_ = -1;
        		break;

        	// clear cluster - remove spikes whose waveshape crosses line (x1,y1) -> (x2, y2)
        	case SDLK_c:
        		if (x2_ > 0)
        		{
        			// iterate through all spikes in buffer and assign 0 (artefact) clusters

        				if (x2_ < x2_){
        					int swap = x1_;
        					x1_ = x2_;
        					x2_ = swap;
        				}

        				float xw1 = XToWaveshapeSampleNumber(x1_);
        				float yw1 = YToPower(selected_channel_, y1_);
        				float xw2 = XToWaveshapeSampleNumber(x2_);
        				float yw2 = YToPower(selected_channel_, y2_);

        				// add to cuts list to cut coming spikes
        				// TODO display cuts
        				if (user_context_.SelectedCluster1() > 0)
        					cluster_cuts_[targ_tetrode_][user_context_.SelectedCluster1()].push_back(WaveshapeCut(x1_, y1_, x2_, y2_, selected_channel_));
        				if (user_context_.SelectedCluster2() > 0)
        					cluster_cuts_[targ_tetrode_][user_context_.SelectedCluster2()].push_back(WaveshapeCut(x1_, y1_, x2_, y2_, selected_channel_));

        				for (unsigned int s = 0; s < buffer->spike_buf_no_disp_pca; ++s) {
        					Spike *spike = buffer->spike_buffer_[s];

        					if (spike == nullptr)
        						continue;

        					if ((unsigned int)spike->tetrode_ == targ_tetrode_ && user_context_.IsSelected(spike)){
        						if (display_final_){
        							if (spike->crossesWaveShapeFinal(selected_channel_, xw1, yw1, xw2, yw2)){
        								spike->cluster_id_ = 0;
        							}
        						}
        						else{
        							if (spike->crossesWaveShapeReconstructed(selected_channel_, xw1, yw1, xw2, yw2)){
        								spike->cluster_id_ = 0;
        							}
        						}
        					}
        				}


        			buf_pointer_ = 0;

        			x1_ = x2_ = y1_ = y2_ = -1;
        		}

        		break;

            case SDLK_ESCAPE:
                exit(0);
                break;
                
            // change scale
            case SDLK_KP_PLUS:
            	scale_ /= 1.1;
            	buf_pointer_ = 0;
            	break;
            case SDLK_KP_MINUS:
            	scale_ *= 1.1;
            	buf_pointer_ = 0;
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
            	if (kmod && KMOD_LCTRL)
            		selected_channel_ = 0;
            	else
            		disp_cluster_2 = 1 + shift;
            	break;
            case SDLK_2:
            	if (kmod && KMOD_LCTRL)
            		selected_channel_ = 1;
            	else
            		disp_cluster_2 = 2 + shift;
            	break;
            case SDLK_3:
            	if (kmod && KMOD_LCTRL)
            		selected_channel_ = 2;
            	else
            		disp_cluster_2 = 3 + shift;
            	break;
            case SDLK_4:
            	if (kmod && KMOD_LCTRL)
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

            default:
                need_redraw = false;
        }

        if (disp_cluster_1 != user_context_.SelectedCluster1()){
        	user_context_.SelectCluster1(disp_cluster_1);
        }
        if (disp_cluster_2 != user_context_.SelectedCluster2()){
        	user_context_.SelectCluster1(disp_cluster_2);
        }
    }

    if (need_redraw){
        reinit();
    }
}

void SDLWaveshapeDisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    // duplicates functionality in process_SDL_control_input, but is supposed to be called in all displays simultaneously
    targ_tetrode_ = display_tetrode;
    ReinitScreen();
}
