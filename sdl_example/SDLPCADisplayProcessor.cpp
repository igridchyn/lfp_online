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
, SDLSingleWindowDisplay(window_name, window_width, window_height)
// paired qualitative brewer palette
, palette_(24, new int[24]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928,
	0xD69EB3, 0x4FA884, 0xE2AF5A, 0x63D05C, 0xCB6AC9, 0xB34A4C, 0xBDEF3F, 0xAFAF30, 0xFA82A6, 0x9A6D6A, 0xBFBFC9, 0xE18958})
, target_tetrode_(target_tetrode)
, display_unclassified_(display_unclassified)
, scale_(scale)
, shift_x_(shift_x)
, shift_y_(shift_y)
, time_end_(buffer->SAMPLING_RATE * 60)
, rend_freq_(buffer->config_->getInt("pcadisp.rend.rate", 5))
, poly_save_(buffer->config_->getBool("pcadisp.poly.save", false))
, poly_load_(buffer->config_->getBool("pcadisp.poly.load", false))
, poly_path_(buffer->config_->getOutPath("pcadisp.poly.path", "poly.dat"))
, num_pc_(buffer->config_->getInt("pca.num.pc"))
, power_thold_nstd_(buffer->config_->getInt("spike.detection.nstd"))
{
    nchan_ = buffer->tetr_info_->channels_number(target_tetrode);

    for(size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
    	polygon_clusters_.push_back(std::vector<PolygonCluster>());
    	// insert fake 0 cluster (interpreted as artifact cluster)
    	polygon_clusters_[t].push_back(PolygonCluster());
    }

    if(poly_load_){

    	if (Utils::FS::FileExists(poly_path_)){

			std::ifstream fpoly(poly_path_);
			// n tetr / n polycs / polycs
			unsigned int ntetr = 0;
			fpoly >> ntetr;

			if (ntetr == buffer->tetr_info_->tetrodes_number()){
				for(unsigned int t=0; t < ntetr; ++t){
					unsigned int nclust = 0;
					fpoly >> nclust;
					polygon_clusters_[t].clear();
					for (unsigned int c=0; c < nclust; ++c){
						polygon_clusters_[t].push_back(PolygonCluster(fpoly));
					}
				}
			}
			else{
				buffer->Log("Number of tetrodes in clustering file is different from current number of tetrodes");
			}
    	}else{
    		buffer->Log("ERROR: Polygon clusters file not found!");
    	}
    }

    points_ = new SDL_Point[1000000];

    spikes_to_draw_.resize(MAX_CLUST);
    for (unsigned int c = 0; c < MAX_CLUST; ++c) {
    	spikes_to_draw_[c] = new SDL_Point[spikes_draw_freq_];
	}
    spikes_counts_.resize(MAX_CLUST);

    display_cluster_.resize(MAX_CLUST, true);
}

void SDLPCADisplayProcessor::process(){
    bool render = false;

    if (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_){
    	SDL_SetRenderTarget(renderer_, texture_);
    }

    while (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_disp_pca];
        // wait until cluster is assigned

		// TODO !!! no nullptr spikes, report and prevent by architecture (e.g. rewind to start level)
        // TODO add filtering by power threshold capability
        if (spike == nullptr){
            buffer->spike_buf_no_disp_pca++;
            continue;
        }

        // TODO !!! take channel with the spike peak (save if not available)
        double power_thold = buffer->powerEstimatorsMap_[buffer->tetr_info_->tetrode_channels[spike->tetrode_][0]]->get_std_estimate() * power_thold_nstd_ * power_threshold_factor_;

        if (abs(spike->power_) < power_thold){
        	buffer->spike_buf_no_disp_pca++;
        	spike->cluster_id_ = -1;
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

        int x;
        int y;
        getSpikeCoords(spike, x ,y);

        // polygon cluster
        // TODO use scaled coordinates
        if (spike->cluster_id_ == -1 && need_clust_check_){
        	for (size_t i=0; i < polygon_clusters_[spike->tetrode_].size(); ++i){
        		// TODO use other dimensions if cluster has the other one

        		// TODO !!! incapsulate after linearizing PC
        		int contains = polygon_clusters_[spike->tetrode_][i].Contains(spike, nchan_);
        		if (contains){
        			spike->cluster_id_ = i;
        		}

        	}
        }

        if (spike->tetrode_ != target_tetrode_){
        	buffer->spike_buf_no_disp_pca++;
        	continue;
        }

        // TODO ??? don't display artifacts and unknown with the same color
        const unsigned int cid = spike->cluster_id_ > -1 ? spike->cluster_id_ : 0;

        if (! display_cluster_[cid]){
        	buffer->spike_buf_no_disp_pca++;
        	continue;
        }

        // if not online, collect and draw in batches
        if (buffer->pipeline_status_ != PIPELINE_STATUS_ONLINE){
        	spikes_to_draw_[cid][spikes_counts_[cid] ++] = {x, y};
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
				DrawCross(3, x, y, 5);
        	}

        	refractory_last_time_ = spike->pkg_id_;
        }

        buffer->spike_buf_no_disp_pca++;

        int freqmult = buffer->pipeline_status_ == PIPELINE_STATUS_READ_FET ? 30000 : 1;
        if (!(buffer->spike_buf_no_disp_pca % (rend_freq_ * freqmult)))
            render = true;
    }

    if (render){
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer_, 0, shift_y_, window_width_, shift_y_);
        SDL_RenderDrawLine(renderer_, shift_x_, 0, shift_x_, window_height_);

        // polygon vertices
        // TODO don't redraw every time ???
        for(size_t i=0; i < polygon_x_.size(); ++i){
        	SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 255);
        	int x = polygon_x_[i] / scale_ + shift_x_;
        	int y = polygon_y_[i] / scale_ + shift_y_;
        	int w = 3;
        	SDL_RenderDrawLine(renderer_, x-w, y, x+w, y);
        	SDL_RenderDrawLine(renderer_, x, y-w, x, y+w);

        	if (i > 0){
        		int px = polygon_x_[i-1] / scale_ + shift_x_;
        		int py = polygon_y_[i-1] / scale_ + shift_y_;
        		SDL_RenderDrawLine(renderer_, px, py, x, y);
        	}
        }

        // TODO !!! don't redraw every time?
        // TODO ??? select unknown cluster
        if (user_context_.SelectedCluster1() > 0 && polygon_clusters_[target_tetrode_].size() > (unsigned int)user_context_.SelectedCluster1()){
        	for(size_t p=0; p < polygon_clusters_[target_tetrode_][user_context_.SelectedCluster1()].projections_inclusive_.size(); ++p){
        		PolygonClusterProjection& proj = polygon_clusters_[target_tetrode_][user_context_.SelectedCluster1()].projections_inclusive_[p];
        		// TODO Process inverted dim1/dim2 - everywhere
        		if ((unsigned int)proj.dim1_ == comp1_ && (unsigned int)proj.dim2_ == comp2_){
        			SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 255);
        			for (size_t pt=1;pt < proj.coords1_.size(); ++pt){
        				int x = scale_x(proj.coords1_[pt-1]);
        				int y = scale_y(proj.coords2_[pt-1]);
        				int x2 = scale_x(proj.coords1_[pt]);
        				int y2 = scale_y(proj.coords2_[pt]);
        				SDL_RenderDrawLine(renderer_, x, y, x2, y2);
        			}
        		}
        	}
        }
        if (user_context_.SelectedCluster2() >= 0 && polygon_clusters_[target_tetrode_].size() > (unsigned int)user_context_.SelectedCluster2()){
                	for(size_t p=0; p < polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()].projections_inclusive_.size(); ++p){
                		PolygonClusterProjection& proj = polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()].projections_inclusive_[p];
                		// TODO Process inverted dim1/dim2 - everywhere
                		if ((unsigned int)proj.dim1_ == comp1_ && (unsigned int)proj.dim2_ == comp2_){
                			SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
                			for (size_t pt=1;pt < proj.coords1_.size(); ++pt){
                				int x = scale_x(proj.coords1_[pt-1]);
                				int y = scale_y(proj.coords2_[pt-1]);
                				int x2 = scale_x(proj.coords1_[pt]);
                				int y2 = scale_y(proj.coords2_[pt]);
                				SDL_RenderDrawLine(renderer_, x, y, x2, y2);
                			}
                		}
                	}
                }

		if (polygon_closed_ && polygon_x_.size() > 0){
			SDL_SetRenderDrawColor(renderer_, 0, 255, 0,255);
        	SDL_RenderDrawLine(renderer_, scale_x(polygon_x_[0]), scale_y(polygon_y_[0]), scale_x(polygon_x_[polygon_x_.size() - 1]), scale_y(polygon_y_[polygon_y_.size() - 1]));
        }

		// too slow, don't display during load
		if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER){
			ResetTextStack();
			std::string text = std::string("Tetrode # ") + Utils::NUMBERS[target_tetrode_ + 1] + "( channels:";
			for (int c=0; c < buffer->tetr_info_->channels_number(target_tetrode_); ++c){
				text += std::string(" ") + Utils::Converter::int2str((int)buffer->tetr_info_->tetrode_channels[target_tetrode_][c]);
			}
			text += ")";
			TextOut(text);

			// cluster numbers
			for (int c=1; c < 20; ++c){
				if (display_cluster_[c])
					TextOut(std::string(Utils::NUMBERS[c]) + " ", palette_.getColor(c), false);
			}
		}
		double power_thold = buffer->powerEstimatorsMap_[buffer->tetr_info_->tetrode_channels[target_tetrode_][0]]->get_std_estimate() * power_thold_nstd_ * power_threshold_factor_;
		std::stringstream ss;
		ss << "Power threshold: " << power_thold;
		if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER){
			TextOut(ss.str());
		}

		// if were redrawing in an off-line mode, draw all collected spikes
		if (buffer->pipeline_status_ != PIPELINE_STATUS_ONLINE){
			for (size_t cid=0; cid <  polygon_clusters_[target_tetrode_].size(); ++cid){
				SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid), 255);
				SDL_RenderDrawPoints(renderer_, &spikes_to_draw_[cid][0], spikes_counts_[cid]);
				spikes_counts_[cid] = 0;
			}
		}

    	// doesn't draw with render OR texture target
        SDL_SetRenderTarget(renderer_, nullptr);
        // without copying only part is displayed AND only before redrawing
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
		SDL_RenderPresent(renderer_);
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
		fpoly << polygon_clusters_[t].size() << "\n";
		for (size_t c=0; c < polygon_clusters_[t].size(); ++c){
			polygon_clusters_[t][c].Serialize(fpoly);
		}
	}

	fpoly.close();
}

// TODO ???!!!
void SDLPCADisplayProcessor::reset_spike_pointer(){
	if (buffer->spike_buffer_[10] == nullptr)
		buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN;
	else
		buffer->spike_buf_no_disp_pca = 1;
}

void SDLPCADisplayProcessor::getSpikeCoords(const Spike *const spike, int& x, int& y) {
	 // time
	// TODO: check numeration
	if (comp1_ == 16){
		x = (spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_;
	}
	else{
		float rawx = spike->getFeature(comp1_); // spike->pc[comp1_ % nchan_][comp1_ / nchan_];
		x = rawx / scale_ + shift_x_;

	}
	if (comp2_ == 16){
		y = (spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_;
	}
	else{
		float rawy = spike->getFeature(comp2_); //spike->pc[comp2_ % nchan_][comp2_ / nchan_];
		y = rawy / scale_ + shift_y_;
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
			// select cluster
			if (kmod & KMOD_LCTRL){
				float rawx = (e.button.x - shift_x_) * scale_;
				float rawy = (e.button.y - shift_y_) * scale_;

				for(size_t c=0; c < polygon_clusters_[target_tetrode_].size(); ++c){
					if (polygon_clusters_[target_tetrode_][c].Contains(rawx, rawy)){
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
			}else if (kmod & KMOD_LSHIFT){
				float rawx = (e.button.x - shift_x_) * scale_;
				float rawy = (e.button.y - shift_y_) * scale_;

				for(size_t c=0; c < polygon_clusters_[target_tetrode_].size(); ++c){
					if (polygon_clusters_[target_tetrode_][c].Contains(rawx, rawy)){
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
			else{
				// buffer->log_string_stream_ << "Left button at " << e.button.x << ", " << e.button.y << "\n";
				// buffer->Log();
				if (!polygon_closed_){
					polygon_x_.push_back((e.button.x - shift_x_) * scale_);
					polygon_y_.push_back((e.button.y - shift_y_) * scale_);

					SDL_SetRenderTarget(renderer_, texture_);
					DrawCross(3, e.button.x, e.button.y, 4);
					if (polygon_x_.size() > 1){
						int last = polygon_x_.size() - 1;
						SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 0);
						SDL_RenderDrawLine(renderer_, scale_x(polygon_x_[last]), scale_y(polygon_y_[last]), scale_x(polygon_x_[last - 1]), scale_y(polygon_y_[last-1]));
					}
					SDL_SetRenderTarget(renderer_, NULL);
					SDL_RenderCopy(renderer_, texture_, NULL, NULL);
					SDL_RenderPresent(renderer_);
				}else{
					polygon_closed_ = false;
					polygon_x_.clear();
					polygon_y_.clear();
				}
			}
		}
		else if (e.button.button == SDL_BUTTON_MIDDLE && polygon_y_.size() > 0){
			// TODO !!! draw line without reset
			polygon_closed_ = true;
			//reset_spike_pointer();
			SDL_SetRenderTarget(renderer_, texture_);
			if (polygon_x_.size() > 1){
				int last = polygon_x_.size() - 1;
				SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 0);
				SDL_RenderDrawLine(renderer_, scale_x(polygon_x_[last]), scale_y(polygon_y_[last]), scale_x(polygon_x_[0]), scale_y(polygon_y_[0]));
			}
			SDL_SetRenderTarget(renderer_, NULL);
			SDL_RenderCopy(renderer_, texture_, NULL, NULL);
			SDL_RenderPresent(renderer_);
		}
	}

	if (e.type == SDL_MOUSEWHEEL){
//		std::cout << e.wheel.y << "\n";
		scale_ *= pow(1.1, e.wheel.y);
		need_redraw = true;
		need_clust_check_ = false;
	}

    if( e.type == SDL_KEYDOWN )
    {
    	need_redraw = true;
    	need_clust_check_ = false;

        // select clusters from 10 to 29
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift = 10;
        } else{
        	if (kmod & KMOD_RSHIFT){
        		shift = 20;
        	}
        }

        unsigned int old_comp1 = comp1_;
        unsigned int old_comp2 = comp2_;

        bool kp_pressed_ = false;

        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
        	// change power threshold + udpate ACs
        	case SDLK_EQUALS:
        		power_threshold_factor_ *= power_threshold_factor_step_;
        		buffer->ResetAC(target_tetrode_);
        		break;

        	case SDLK_MINUS:
        	    power_threshold_factor_ /= power_threshold_factor_step_;
        	    buffer->ResetAC(target_tetrode_);
        	    break;

        	// merge clusters
        	case SDLK_m:
        		need_redraw = false;
        		if (user_context_.SelectedCluster1() < 0 || user_context_.SelectedCluster2() < 0)
        			break;
        		else{
        			mergeClusters();
        		}

        		break;

        	// polygon operations
        	case SDLK_s:{
        		addCluster();
        		need_redraw = false;
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

        	// D: delete all clusters, d: delete last polygon point
        	case SDLK_d:
        		if (kmod & KMOD_LSHIFT){
        			deleteAllClusters();
        		}else{
        			if (polygon_x_.size() > 0){
        				polygon_x_.erase(polygon_x_.end() - 1);
        				polygon_y_.erase(polygon_y_.end() - 1);
        			}
        			// need_redraw = false;
        		}
        		break;

        	// c: clear cluster -> show spikes in the refractory period
        	case SDLK_c:
        		// caancel  refractory spikes display
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

            case SDLK_ESCAPE:
                exit(0);
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
            	scale_ *= 1.1;
            	break;
            case SDLK_KP_PLUS:
            	scale_ /= 1.1;
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
        	// TODO : flexibility
        	if (comp1_ >= num_pc_ * nchan_ + 4){
        		comp1_ = old_comp1;
        	}

        	if (comp2_ >= num_pc_ * nchan_ + 4){
        		comp2_ = old_comp2;
        	}

        	// TODO parametrize
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

                ReinitScreen();
            }
}

void SDLPCADisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode) {
	target_tetrode_ = display_tetrode;
	ReinitScreen();

	comp1_ = 0;
	comp2_ = 1;

	// WORKAROUND UNTIL CIRC. BUFFER	
	reset_spike_pointer();

	polygon_x_.clear();
	polygon_y_.clear();

	nchan_ = buffer->tetr_info_->channels_number(target_tetrode_);

	user_context_.SelectCluster1(-1);
	user_context_.SelectCluster2(-1);
}

void SDLPCADisplayProcessor::deleteCluster() {
	if (user_context_.SelectedCluster2() >= 0){
		// update spikes
		for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
			Spike *spike = buffer->spike_buffer_[sind];
			if (spike == nullptr || spike->tetrode_ != target_tetrode_)
				continue;

			if (spike -> cluster_id_ == user_context_.SelectedCluster2()){
				spike->cluster_id_ = -1;
			}
		}

		// TODO : use acions list for synchronization
		//sbuffer->ResetAC(target_tetrode_, user_context_.SelectedCluster2());

		user_context_.DelleteCluster(polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()]);

		buffer->ResetPopulationWindow();
	}
}


void SDLPCADisplayProcessor::addExclusiveProjection() {
	PolygonClusterProjection tmpproj(polygon_x_, polygon_y_, comp1_, comp2_);

	if (user_context_.SelectedCluster2() > -1){
		// TODO either implement polygon intersection or projections logical operations
		for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
			Spike *spike = buffer->spike_buffer_[sind];
			if (spike == nullptr || spike->tetrode_ != target_tetrode_)
				continue;

			if (spike -> cluster_id_ == user_context_.SelectedCluster2()){
				float rawx = spike->getFeature(comp1_); //spike->pc[comp1_ % nchan_][comp1_ / nchan_];
				float rawy = spike->getFeature(comp2_);; //spike->pc[comp2_ % nchan_][comp2_ / nchan_];

				if (tmpproj.Contains(rawx, rawy)){
					spike->cluster_id_ = -1;
				}
			}
		}

		polygon_closed_ = false;
		polygon_x_.clear();
		polygon_y_.clear();

		polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()].projections_exclusive_.push_back(tmpproj);

		user_context_.AddExclusiveProjection(tmpproj);

		// reset AC
		// TODO extract
		//buffer->ResetAC(target_tetrode_, user_context_.SelectedCluster2());

		// needed here?
		buffer->ResetPopulationWindow();
	}
}


void SDLPCADisplayProcessor::deleteAllClusters() {
	polygon_clusters_[target_tetrode_].clear();
	buffer->Log("Deleted all clusters");

	// TODO ? coordinate with model clustering
	for(unsigned int si = buffer->SPIKE_BUF_HEAD_LEN; si < buffer->spike_buf_no_disp_pca; ++si){
		Spike *spike = buffer->spike_buffer_[si];
		if (spike->tetrode_ == target_tetrode_){
			spike->cluster_id_ = -1;
		}
	}

	buffer->ResetAC(target_tetrode_);

	save_polygon_clusters();

	buffer->ResetPopulationWindow();
}


void SDLPCADisplayProcessor::addCluster() {
	PolygonCluster new_clust_ = PolygonCluster(PolygonClusterProjection(polygon_x_, polygon_y_, comp1_, comp2_));
	unsigned int clun = (unsigned int)user_context_.CreateClsuter(polygon_clusters_[target_tetrode_].size(), new_clust_.projections_inclusive_[0]);

	// push back new or replace cluster invalidated before
	if (clun == polygon_clusters_[target_tetrode_].size()){
		polygon_clusters_[target_tetrode_].push_back(new_clust_);
	}
	else{
		polygon_clusters_[target_tetrode_][clun] = new_clust_;
	}

	buffer->Log("Created new polygon cluster, total clusters = ", (int)polygon_clusters_[target_tetrode_].size());
	save_polygon_clusters();

	// TODO ClearPolygon()
	polygon_closed_ = false;
	polygon_x_.clear();
	polygon_y_.clear();

	// TODO !!! buffer->resetAC()
	// set to where it was reset...
	// TODO check whether correct cluster number is given
	// buffer->ResetAC(target_tetrode_, polygon_clusters_.size() - 1);

	buffer->ResetPopulationWindow();

	// not to redraw: iterate through spikes and redraw
//	SDL_SetRenderTarget(renderer_, nullptr);
//	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_SetRenderTarget(renderer_, texture_);
	SetDrawColor(clun);
	int count = 0;
	for (unsigned int s=0; s < buffer->spike_buf_pos_unproc_; ++s){
		Spike *spike = buffer->spike_buffer_[s];
		if (spike == nullptr || spike->discarded_ || spike->tetrode_ != target_tetrode_ || spike->cluster_id_ >= 0){
			continue;
		}

		if (new_clust_.Contains(spike, spike->num_channels_)){
			int x, y;
			getSpikeCoords(spike, x, y);
			spike->cluster_id_ = clun;
			//SDL_RenderDrawPoint(renderer_, x, y);
			points_[count++ ] = {x, y};
		}
	}
	SDL_RenderDrawPoints(renderer_, points_, count - 1);

	SDL_SetRenderTarget(renderer_, nullptr);
	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_RenderPresent(renderer_);
}

void SDLPCADisplayProcessor::mergeClusters() {

	// keep the cluster with the lowers numbers
	if (user_context_.SelectedCluster1() > user_context_.SelectedCluster2()){
		int swap = user_context_.SelectedCluster1();
		user_context_.SelectCluster1(user_context_.SelectedCluster2());
		user_context_.SelectCluster2(swap);
	}

	// copy projections
	// TODO: exclusive as well ? (can affect other clusters)
	polygon_clusters_[target_tetrode_][user_context_.SelectedCluster1()].projections_inclusive_.insert(polygon_clusters_[target_tetrode_][user_context_.SelectedCluster1()].projections_inclusive_.end(),
			polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()].projections_inclusive_.begin(), polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()].projections_inclusive_.end());

	// ??? change spike cluster from the beginning, redraw after
	// TODO: just set to -1 ?
	int scount = 0;
	SDL_SetRenderTarget(renderer_, texture_);
	SetDrawColor(user_context_.SelectedCluster1());
	for(unsigned int sind = 0; sind < buffer->spike_buf_no_disp_pca; ++sind){
		Spike *spike = buffer->spike_buffer_[sind];
		if (spike->tetrode_ != target_tetrode_)
			continue;

		if (spike->cluster_id_ == user_context_.SelectedCluster2()){
			spike->cluster_id_ = user_context_.SelectedCluster1();
			int x,y;
			getSpikeCoords(spike, x, y);
			points_[scount++ ] = {x, y};
		}
	}

	// remove cluster from list of tetrode poly clusters
	user_context_.MergeClusters(polygon_clusters_[target_tetrode_][user_context_.SelectedCluster1()], polygon_clusters_[target_tetrode_][user_context_.SelectedCluster2()]);
	buffer->ResetPopulationWindow();

	// TODO !!! extract
	SDL_RenderDrawPoints(renderer_, points_, scount - 1);
	SDL_SetRenderTarget(renderer_, nullptr);
	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_RenderPresent(renderer_);
}
