//
//  AutocorrelogramProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 27/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "AutocorrelogramProcessor.h"

AutocorrelogramProcessor::AutocorrelogramProcessor(LFPBuffer* buf)
:AutocorrelogramProcessor(buf,
		buf->config_->getFloat("ac.bin.size.ms"),
		buf->config_->getInt("ac.n.bins")
){
}

AutocorrelogramProcessor::AutocorrelogramProcessor(LFPBuffer *buf, const float bin_size_ms, const unsigned int nbins)
: SDLSingleWindowDisplay("Autocorrelogramms", buf->config_->getInt("ac.window.width"), buf->config_->getInt("ac.window.height"))
, SDLControlInputProcessor(buf)
, LFPProcessor(buf)
, BIN_SIZE(buf->SAMPLING_RATE/1000 * bin_size_ms)
, NBINS(nbins)
, wait_clustering_(buffer->config_->getBool("ac.wait.clust", true))
, user_context_(buffer->user_context_){
	buf->spike_buf_pos_auto_ = buffer->SPIKE_BUF_HEAD_LEN;

	const unsigned int tetrn = buf->tetr_info_->tetrodes_number;

	autocorrs_.resize(tetrn);
	cross_corrs_.resize(tetrn);
	spike_times_lists_.resize(tetrn);

	for (int i=0; i < tetrn; ++i) {
		autocorrs_[i].resize(MAX_CLUST);
		cross_corrs_[i].resize(MAX_CLUST);
		for (int c = 0; c < MAX_CLUST; ++c) {
			cross_corrs_[i][c].resize(MAX_CLUST);
			for (int c2 = 0; c2 < MAX_CLUST; ++c2) {
				cross_corrs_[i][c][c2].resize(NBINS);
			}
		}
		spike_times_lists_[i].resize(MAX_CLUST);

		for (int c=0; c < MAX_CLUST; ++c) {
			autocorrs_[i][c].resize(NBINS);
		}
	}
}

void AutocorrelogramProcessor::process(){
	// IF the ACs on one tetrode have to be recalculated
	if (buffer->ac_reset_){
		int tetr_reset = buffer->ac_reset_tetrode_;

		// reset all
		if (buffer->ac_reset_cluster_ == -1){
			for (int c = 0; c < MAX_CLUST; ++c){
				for (int b = 0; b < NBINS; ++b){
					autocorrs_[buffer->ac_reset_tetrode_][c][b] = 0;
				}
			}
		}
		// reset 1 cluster
		else{
			for (int b = 0; b < NBINS; ++b){
				autocorrs_[buffer->ac_reset_tetrode_][buffer->ac_reset_cluster_][b] = 0;
			}
		}

		buffer->ac_reset_ = false;
		buffer->ac_reset_cluster_ = -1;
		buffer->ac_reset_tetrode_ = -1;
	}

	// TODO process user actions (no need in reset above then)
	if (buffer->spike_buf_pos_auto_ < buffer->spike_buf_no_disp_pca){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_auto_];
		if (spike != nullptr && user_context_.HasNewAction(spike->pkg_id_)){
			// process new user action

			switch(user_context_.last_user_action_){
			case UA_SELECT_CLUSTER1:
				break;
			}
		}
	}

	while(buffer->spike_buf_pos_auto_ < buffer->spike_buf_no_disp_pca){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_auto_];

		if (spike == nullptr || spike->discarded_){
			buffer->spike_buf_pos_auto_++;
			continue;
		}

		if (spike->cluster_id_ == -1){
			if (wait_clustering_)
				break;
			else{
				buffer->spike_buf_pos_auto_++;
				continue;
			}
		}

		unsigned int tetrode = spike->tetrode_;
		unsigned int cluster_id = spike->cluster_id_;
		unsigned int stime = spike->pkg_id_;

		// update autocorrelation bins
		std::list<unsigned int>& prev_spikes_queue = spike_times_lists_[tetrode][cluster_id];

		int max_diff = BIN_SIZE * NBINS;

		for (std::list<unsigned int>::iterator si = prev_spikes_queue.begin(); si != prev_spikes_queue.end(); ++si) {
			if (stime - *si > max_diff){
				continue;
			}

			// 2 ms bins
			// TODO: configurable bitrate
			unsigned int bin = (stime - *si) / (BIN_SIZE );
			if (bin >= NBINS || bin < 0){
				continue;
			}

			autocorrs_[tetrode][cluster_id][bin]++;
		}

		// fill cross-correlograms - no need to delete here, start from end until
		// TODO number of clusters ?
		for (int c = 0; c < MAX_CLUST; ++c) {
			if (c == spike->cluster_id_)
				continue;

			std::list<unsigned int>& prev_spikes_ = spike_times_lists_[tetrode][c];
			for (std::list<unsigned int>::iterator si = prev_spikes_.begin(); si != prev_spikes_.end(); ++si) {
				if (stime - *si > max_diff){
					continue;
				}

				unsigned int bin = (stime - *si) / (BIN_SIZE );
				if (bin >= NBINS || bin < 0){
					continue;
				}

				cross_corrs_[tetrode][cluster_id][c][bin]++;
			}
		}

		while(!prev_spikes_queue.empty() && stime - prev_spikes_queue.front() > max_diff)
			prev_spikes_queue.pop_front();

		// replace new spike with last in the buf and advance pointer
		prev_spikes_queue.push_back(stime);

		buffer->spike_buf_pos_auto_++;

		plotAC(tetrode, cluster_id);
	}
}

unsigned int AutocorrelogramProcessor::getXShift(int clust) {
	return ((BWIDTH + 1) * NBINS + 15) * (clust % XCLUST) + 30;
}

unsigned int AutocorrelogramProcessor::getYShift(int clust) {
	return (clust / XCLUST) * ypix_ + 100;
}

void AutocorrelogramProcessor::drawClusterRect(int clust) {
	int xsh = getXShift(clust);
	int ysh = getYShift(clust);

	SDL_Rect rect;
	int height = ypix_;
	rect.h = height;
	rect.w = (BWIDTH + 1) * NBINS;
	rect.x = xsh;
	rect.y = ysh - height;
	SDL_RenderDrawRect(renderer_, &rect);
}

int AutocorrelogramProcessor::getClusterNumberByCoords(const unsigned int& x,
		const unsigned int& y) {
	int cx = (x - 30) / ((BWIDTH + 1) * NBINS + 15);
	int cy = y / ypix_;
	return cy * XCLUST + cx;
}

void AutocorrelogramProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
	if (display_tetrode_ >= buffer->tetr_info_->tetrodes_number)
		return;

	// TODO extract redraw

	display_tetrode_ = display_tetrode;

	SDL_SetRenderTarget(renderer_, texture_);
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);

	for (int c=0; c < MAX_CLUST; ++c) {
		plotAC(display_tetrode_, c);
	}

	if (user_context_.selected_cluster1_ >= 0){
		SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 255);
		drawClusterRect(user_context_.selected_cluster1_);
	}



	if (user_context_.selected_cluster2_ >= 0){
		SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
		drawClusterRect(user_context_.selected_cluster2_);
	}

	SDL_SetRenderTarget(renderer_, nullptr);
	SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
	SDL_RenderPresent(renderer_);
}

void AutocorrelogramProcessor::process_SDL_control_input(const SDL_Event& e){
	// TODO: implement

	SDL_Keymod kmod = SDL_GetModState();

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.windowID == GetWindowID()){
		if (e.button.button == SDL_BUTTON_LEFT){
			// select cluster 1
			if (kmod & KMOD_LCTRL){
				int clun = getClusterNumberByCoords(e.button.x, e.button.y);
				if (clun >= 0){
					if (clun == user_context_.selected_cluster1_){
						user_context_.SelectCluster1(-1);
					}
					else{
						user_context_.SelectCluster1(clun);
					}
					SetDisplayTetrode(display_tetrode_);
				}
			}

			if (kmod & KMOD_LSHIFT){
				int clun = getClusterNumberByCoords(e.button.x, e.button.y);
				if (clun >= 0){
					if (clun == user_context_.selected_cluster2_){
						user_context_.SelectCluster2(-1);
					}
					else{
						user_context_.SelectCluster2(clun);
					}
					SetDisplayTetrode(display_tetrode_);
				}
			}
		}
	}

	if (e.type == SDL_WINDOWEVENT) {
		if( e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
			SetDisplayTetrode(display_tetrode_);

			// redraw those that have been reported already
			for (int c=0; c < MAX_CLUST; ++c){
				plotAC(display_tetrode_, c);
			}
		}
	}

	if( e.type == SDL_KEYDOWN )
	{
		switch(e.key.keysym.sym){
		// refresh
		case SDLK_r:
			SetDisplayTetrode(display_tetrode_);
			break;
		default:
			break;
		}

	}
}

// plot the autocorrelogramms function
void AutocorrelogramProcessor::plotAC(const unsigned int tetr, const unsigned int cluster){
	if (tetr != display_tetrode_)
		return;

	// shift for the plot

	const int xsh = getXShift(cluster);
	const int ysh = getYShift(cluster);

	ColorPalette palette_ = ColorPalette::BrewerPalette12;

	SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
	SDL_RenderDrawLine(renderer_, xsh, 0, xsh, window_height_);

	int maxh = 0;
	for (int b = 0; b < NBINS; ++b) {
		int height = autocorrs_[tetr][cluster][b];
		if (height > maxh)
			maxh = height;
	}

	for (int b=0; b < NBINS; ++b) {
		int height = (autocorrs_[tetr][cluster][b]) * ypix_/ maxh;

		SDL_Rect rect;
		rect.h = height;
		rect.w = BWIDTH;
		rect.x = xsh + b * (BWIDTH + 1);
		rect.y = ysh - height;

		SDL_SetRenderDrawColor(renderer_, palette_.getR(cluster % palette_.NumColors()), palette_.getG(cluster % palette_.NumColors()), palette_.getB(cluster % palette_.NumColors()), 255);
		SDL_RenderFillRect(renderer_, &rect);
	}
}


