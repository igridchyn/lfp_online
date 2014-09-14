//
//  SDLPCADisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "SDLPCADisplayProcessor.h"

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
: SDLControlInputProcessor(buffer)
, SDLSingleWindowDisplay(window_name, window_width, window_height)
// paired qualitative brewer palette
, palette_(12, new int[12]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928})
, target_tetrode_(target_tetrode)
, display_unclassified_(display_unclassified)
, scale_(scale)
, shift_x_(shift_x)
, shift_y_(shift_y)
, time_end_(buffer->SAMPLING_RATE * 60)
, rend_freq_(buffer->config_->getInt("pcadisp.rend.rate", 5))
, poly_save_(buffer->config_->getBool("pcadisp.poly.save", false))
, poly_load_(buffer->config_->getBool("pcadisp.poly.load", false))
, poly_path_(buffer->config_->getString("pcadisp.poly.path", ""))
{
    nchan_ = buffer->tetr_info_->channels_numbers[target_tetrode];

    for(int t=0; t < buffer->tetr_info_->tetrodes_number; ++t){
    	polygon_clusters_.push_back(std::vector<PolygonCluster>());
    }

    if(poly_load_){

    	if (Utils::FS::FileExists(poly_path_)){

			std::ifstream fpoly(poly_path_);
			// n tetr / n polycs / polycs
			int ntetr = 0;
			fpoly >> ntetr;

			if (ntetr == buffer->tetr_info_->tetrodes_number){
				for(int t=0; t < ntetr; ++t){
					int nclust = 0;
					fpoly >> nclust;
					for (int c=0; c < nclust; ++c){
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
}

void SDLPCADisplayProcessor::process(){
    // TODO: parametrize displayed channels and pc numbers
    bool render = false;
    
    while (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_disp_pca];
        // wait until cluster is assigned
        
        if (spike->tetrode_ != target_tetrode_){
            buffer->spike_buf_no_disp_pca++;
            continue;
        }
        
        if (spike->pc == NULL || (spike->cluster_id_ == -1 && !display_unclassified_))
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

        x = spike->pc[comp1_ % nchan_][comp1_ / nchan_]/scale_ + shift_x_;
        y = spike->pc[comp2_ % nchan_][comp2_ / nchan_]/scale_ + shift_y_;

        // polygon cluster
        // TODO use scaled coordinates
        if (spike->cluster_id_ == -1){
        	for (int i=0; i < polygon_clusters_[target_tetrode_].size(); ++i){
        		// TODO use other dimensions if cluster has the other one
        		if (polygon_clusters_[target_tetrode_][i].Contains(x, y)){
        			spike->cluster_id_ = i + 1;
        		}
        	}
        }


        // time
        // TODO: check numeration
        if (comp1_ == 15){
        	x = (spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_;
        }
        if (comp2_ == 15){
        	y = (spike->pkg_id_ - time_start_) / (double)(time_end_ - time_start_) * window_width_;
        }

        const unsigned int cid = spike->cluster_id_ > -1 ? spike->cluster_id_ : 0;
        
        SDL_SetRenderTarget(renderer_, texture_);
		std::string sdlerror(SDL_GetError());
        //SDL_SetRenderDrawColor(renderer_, 255,255,255*((int)spike->cluster_id_/2),255);
        SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid),255);
        SDL_RenderDrawPoint(renderer_, x, y);
        
        buffer->spike_buf_no_disp_pca++;
        
        if (!(buffer->spike_buf_no_disp_pca % rend_freq_))
            render = true;
    }
    
    if (render){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);

        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer_, 0, shift_y_, window_width_, shift_y_);
        SDL_RenderDrawLine(renderer_, shift_x_, 0, shift_x_, window_height_);

        // polygon vertices
        for(int i=0; i < polygon_x_.size(); ++i){
        	SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 255);
        	int x = polygon_x_[i];
        	int y = polygon_y_[i];
        	int w = 3;
        	SDL_RenderDrawLine(renderer_, x-w, y, x+w, y);
        	SDL_RenderDrawLine(renderer_, x, y-w, x, y+w);

        	if (i > 0){
        		int px = polygon_x_[i-1];
        		int py = polygon_y_[i-1];
        		SDL_RenderDrawLine(renderer_, px, py, x, y);
        	}
        }
        if (polygon_closed_){
        	SDL_RenderDrawLine(renderer_, polygon_x_[0], polygon_y_[0], polygon_x_[polygon_x_.size() - 1], polygon_y_[polygon_y_.size() - 1]);
        }

        SDL_RenderPresent(renderer_);
    }
}

void SDLPCADisplayProcessor::save_polygon_clusters() {
	if (poly_path_.length() == 0){
		buffer->Log("ERROR: Polygon cluster file path was not specified in the config");
		return;
	}

	std::ofstream fpoly(poly_path_);

	int ntetr = buffer->tetr_info_->tetrodes_number;
	fpoly << ntetr << "\n";

	for(int t=0; t < ntetr; ++t){
		fpoly << polygon_clusters_[t].size() << "\n";
		for (int c=0; c < polygon_clusters_[t].size(); ++c){
			polygon_clusters_[t][c].Serialize(fpoly);
		}
	}

	fpoly.close();
}

void SDLPCADisplayProcessor::process_SDL_control_input(const SDL_Event& e){
	if (e.type == SDL_MOUSEBUTTONDOWN){
		if (e.button.button == SDL_BUTTON_LEFT){
			std::cout << "Left button at " << e.button.x << ", " << e.button.y << "\n";
			if (!polygon_closed_){
				polygon_x_.push_back(e.button.x);
				polygon_y_.push_back(e.button.y);
			}else{
				polygon_closed_ = false;
				polygon_x_.clear();
				polygon_y_.clear();
			}
		}else if(e.button.button == SDL_BUTTON_MIDDLE){
			polygon_closed_ = true;
			buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN;


		}
	}

	bool need_redraw = false;

	if (e.type == SDL_MOUSEWHEEL){
//		std::cout << e.wheel.y << "\n";
		scale_ *= pow(1.1, e.wheel.y);
		need_redraw = true;
	}

    if( e.type == SDL_KEYDOWN )
    {
    	need_redraw = true;
        SDL_Keymod kmod = SDL_GetModState();
        
        // select clusters from 10 to 29
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift = 10;
        } else{
        	if (kmod & KMOD_RSHIFT){
        		shift = 20;
        	}
        }

        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
        	// polygon operations
        	case SDLK_a:
        		polygon_clusters_[target_tetrode_].push_back(PolygonCluster(PolygonClusterProjection(polygon_x_, polygon_y_, comp1_, comp2_)));
        		buffer->Log("Created new polygon cluster, total clusters = ", polygon_clusters_[target_tetrode_].size());
        		save_polygon_clusters();

        		polygon_closed_ = false;
        		polygon_x_.clear();
        		polygon_y_.clear();
        		break;

        	// D: delete all clusters
        	case SDLK_d:
        		if (kmod & KMOD_LSHIFT){
        			polygon_clusters_[target_tetrode_].clear();
        			buffer->Log("Deleted all clusters");

        			// TODO ? coordinate with model clustering
        			for(int si = buffer->SPIKE_BUF_HEAD_LEN; si < buffer->spike_buf_no_disp_pca; ++si){
        				Spike *spike = buffer->spike_buffer_[si];
        				if (spike->tetrode_ == target_tetrode_){
        					spike->cluster_id_ = -1;
        				}
        			}

        		}else{
        			need_redraw = false;
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
                comp1_ = 6;
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
                break;
            case SDLK_KP_2:
                comp2_ = 2 + shift;
                break;
            case SDLK_KP_3:
                comp2_ = 3 + shift;
                break;
            case SDLK_KP_4:
                comp2_ = 4 + shift;
                break;
            case SDLK_KP_5:
                comp2_ = 5 + shift;
                break;
            case SDLK_KP_6:
                comp2_ = 6;
                break;
            case SDLK_KP_7:
                comp2_ = 7;
                break;
            case SDLK_KP_8:
                comp2_ = 8;
                break;
            case SDLK_KP_9:
                comp2_ = 9;
                break;
            case SDLK_KP_0:
                comp2_ = 0 + shift;
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
    }

    if (need_redraw){
                // TODO: case-wise
                buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN;

                time_start_ = buffer->spike_buffer_[buffer->SPIKE_BUF_HEAD_LEN]->pkg_id_;
                if (buffer->spike_buf_pos_unproc_ > 1 && buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1] != NULL)
                time_end_ = buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_;

                // TODO: EXTRACT
                SDL_SetRenderTarget(renderer_, texture_);
                SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
                SDL_RenderClear(renderer_);
                SDL_RenderPresent(renderer_);
            }
}

void SDLPCADisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode) {
	target_tetrode_ = display_tetrode;
	ReinitScreen();

	buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN;

	polygon_x_.clear();
	polygon_y_.clear();
}
