//
//  PositionDisplayProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PositionDisplayProcessor.h"

PositionDisplayProcessor::PositionDisplayProcessor(LFPBuffer *buf)
: PositionDisplayProcessor(buf,
		buf->config_->getString("posdisp.window.name"),
		buf->config_->getInt("posdisp.window.width"),
		buf->config_->getInt("posdisp.window.height"),
		buf->config_->getInt("posdisp.tetrode"),
		buf->config_->getInt("posdisp.tail.length")
		){}

PositionDisplayProcessor::PositionDisplayProcessor(LFPBuffer *buf, std::string window_name, const unsigned int& window_width,
		const unsigned int& window_height, const unsigned int& target_tetrode, const unsigned int& tail_length)
	: LFPProcessor(buf)
	, SDLControlInputProcessor(buf)
    , SDLSingleWindowDisplay(window_name, window_width, window_height)
    , target_tetrode_(target_tetrode)
	, TAIL_LENGTH(tail_length)
	, WAIT_PREDICTION(buf->config_->getBool("posdisp.wait.prediction"))
	, DISPLAY_PREDICTION(buf->config_->getBool("posdisp.display.prediction"))
	, wait_clust_(buf->config_->getBool("posdisp.wait.clust", false))
	, pos_buf_pointer_limit_(buf->GetPosBufPointer(buf->config_->getString("posdisp.pointer.limit", "pos")))
	, scale_(buf->config_->getFloat("posdisp.scale", 1.0f))
	, speed_limit_(buf->config_->getFloat("posdisp.speed.limit", .0f))
	, draw_circles_(buf->config_->getBool("posdisp.draw.circles", false))
{
	std::string pos_buf_name = buf->config_->getString("posdisp.pointer.limit", "pos");
	if (pos_buf_name == buf->pos_buf_pointer_names_.POS_BUF_SPEED_EST){
		estimate_speed_ = true;
	}

    display_cluster_.resize(200, false);
    
}

void PositionDisplayProcessor::draw_circle(int n_cx, int n_cy, int radius)
{
    // if the first pixel in the screen is represented by (0,0) (which is in sdl)
    // remember that the beginning of the circle is not in the middle of the pixel
    // but to the left-top from it:

    double error = (double)-radius;
    double x = (double)radius -0.5;
    double y = (double)0.5;
    double cx = n_cx - 0.5;
    double cy = n_cy - 0.5;

    SDL_Point *circ_points = new SDL_Point[100000];

    unsigned int npoints = 0;
    while (x >= y)
    {
    	circ_points[npoints].x = (int)(cx + x);
		circ_points[npoints++].y = (int)(cy + y);

		circ_points[npoints].x = (int)(cx + y);
		circ_points[npoints++].y = (int)(cy + x);

        if (x != 0)
        {
        	circ_points[npoints].x = (int)(cx - x);
        	circ_points[npoints++].y = (int)(cy + y);

        	circ_points[npoints].x = (int)(cx + y);
        	circ_points[npoints++].y = (int)(cy - x);
        }

        if (y != 0)
        {
        	circ_points[npoints].x = (int)(cx + x);
        	circ_points[npoints++].y = (int)(cy - y);

        	circ_points[npoints].x = (int)(cx - y);
        	circ_points[npoints++].y = (int)(cy + x);
        }

        if (x != 0 && y != 0)
        {
        	circ_points[npoints].x = (int)(cx - x);
        	circ_points[npoints++].y = (int)(cy - y);

        	circ_points[npoints].x = (int)(cx - y);
        	circ_points[npoints++].y = (int)(cy - x);
        }

        error += y;
        ++y;
        error += y;

        if (error >= 0)
        {
            --x;
            error -= x;
            error -= x;
        }

        npoints ++;
    }

    SDL_RenderDrawPoints(renderer_, circ_points, npoints);
    delete[] circ_points;
}

void PositionDisplayProcessor::process(){
    const int rend_freq = 5;
    bool render = false;
    
    if (buffer->pipeline_status_ != PIPELINE_STATUS_INPUT_OVER){
    	return;
    }

//    while (buffer->pos_buf_disp_pos_ < buffer->pos_buf_pos_) {
    while (buffer->pos_buf_disp_pos_ < pos_buf_pointer_limit_) {

    	// if exceeded clustering prediction - exit
    	if (WAIT_PREDICTION && (buffer->positions_buf_[buffer->pos_buf_disp_pos_].pkg_id_ > buffer->last_preidction_window_ends_[processor_number_])){
    		break;
    	}

        float x = buffer->positions_buf_[buffer->pos_buf_disp_pos_].x_pos() * scale_;
        float y = buffer->positions_buf_[buffer->pos_buf_disp_pos_].y_pos() * scale_;
        
        const unsigned int imm_level = estimate_speed_ ? 150 : 80;
        unsigned int grey_level = imm_level;

        // TODO parametrize speed display parameters
        if (estimate_speed_ && buffer->positions_buf_[buffer->pos_buf_disp_pos_].speed_ > 2.0f){
            grey_level = MIN(255, (int)buffer->positions_buf_[buffer->pos_buf_disp_pos_].speed_ * 5 + 50);
        }
        
        SDL_SetRenderTarget(renderer_, texture_);
        SDL_SetRenderDrawColor(renderer_, grey_level, grey_level, grey_level, 255);
        SDL_RenderDrawPoint(renderer_, (int)x, (int)y);
        
        buffer->pos_buf_disp_pos_++;
        
        if (!(buffer->pos_buf_disp_pos_ % rend_freq))
            render = true;

        // display predicted position
        if (DISPLAY_PREDICTION){
			float predx = buffer->positions_buf_[buffer->pos_buf_disp_pos_ - 1].x_pos() * scale_;
			float predy = buffer->positions_buf_[buffer->pos_buf_disp_pos_ - 1].y_pos() * scale_;
			if (predx > 0 && predy > 0){
				SDL_SetRenderDrawColor(renderer_, 200, 0, 0, 255);
	//        	SDL_RenderDrawPoint(renderer_, predx, predy);
				FillRect((int)predx, (int)predy, 0, 2, 2);
			}
        }
    }
    
    // display spikes on a target tetrode
    while (buffer->spike_buf_pos_draw_xy < buffer->spike_buf_pos_speed_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_draw_xy];
        // wait until cluster is assigned
        
        if ((unsigned int)spike->tetrode_ != target_tetrode_){
            buffer->spike_buf_pos_draw_xy++;
            continue;
        }
        
        if (spike->discarded_ || ((spike->cluster_id_ > -1) && !display_cluster_[spike->cluster_id_])
			|| (!wait_clust_ && (spike->cluster_id_ == -1)) || spike->speed < speed_limit_ || Utils::Math::Isnan(spike->speed)){
        	buffer->spike_buf_pos_draw_xy++;
        	continue;
        }

        FillRect(spike->x * scale_, spike->y * scale_, spike->cluster_id_);
        buffer->spike_buf_pos_draw_xy++;

        if (!(buffer->spike_buf_pos_draw_xy % (rend_freq * 10))){
			render = true;
        }
    }
    
    if (render){
    	SDL_SetRenderDrawColor(renderer_, 200, 0, 0, 255);

    	if (draw_circles_){
    		draw_circle(164, 188, 158);
    		draw_circle(508, 185, 158);
    	}

        Render();
    }
}

void PositionDisplayProcessor::process_SDL_control_input(const SDL_Event& e){
    if( e.type == SDL_KEYDOWN )
    {
        bool need_reset = true;
        
        SDL_Keymod kmod = SDL_GetModState();
        
        int shift = 0;
        if (kmod & KMOD_LSHIFT){
            shift = 10;
        }

        switch( e.key.keysym.sym )
        {
            case SDLK_ESCAPE:
                buffer->processing_over_ = true;
                break;
            case SDLK_0:
                display_cluster_[0+shift] = !display_cluster_[0+shift];
                break;
            case SDLK_1:
                display_cluster_[1+shift] = !display_cluster_[1+shift];
                break;
            case SDLK_2:
                display_cluster_[2+shift] = !display_cluster_[2+shift];
                break;
            case SDLK_3:
                display_cluster_[3+shift] = !display_cluster_[3+shift];
                break;
            case SDLK_4:
                display_cluster_[4+shift] = !display_cluster_[4+shift];
                break;
            case SDLK_5:
                display_cluster_[5+shift] = !display_cluster_[5+shift];
                break;
            case SDLK_6:
                display_cluster_[6+shift] = !display_cluster_[6+shift];
                break;
            case SDLK_7:
                display_cluster_[7+shift] = !display_cluster_[7+shift];
                break;
            case SDLK_8:
                display_cluster_[8+shift] = !display_cluster_[8+shift];
                break;
            case SDLK_9:
                display_cluster_[9+shift] = !display_cluster_[9+shift];
                break;

            default:
                need_reset = false;
                break;

            case SDLK_a:
            	disp_mode_ = POS_DISPLAY_ALL;
            	need_reset = true;
            	break;
            case SDLK_t:
            	disp_mode_ = POS_DISPLAY_TAIL;
            	need_reset = true;
            	break;
        }
        
        if (need_reset){
            buffer->spike_buf_pos_draw_xy = 0;
            buffer->pos_buf_disp_pos_ =  (disp_mode_ == POS_DISPLAY_ALL) ? 0 : buffer->pos_buf_pos_ - TAIL_LENGTH;

            RenderClear();
            Render();
            SDL_SetRenderTarget(renderer_, texture_);
        }
    }
    
}

void PositionDisplayProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
    target_tetrode_ = display_tetrode;
    buffer->spike_buf_pos_draw_xy = 0;
}
