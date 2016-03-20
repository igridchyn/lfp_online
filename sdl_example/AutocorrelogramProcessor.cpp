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
: LFPProcessor(buf)
, SDLControlInputProcessor(buf)
, SDLSingleWindowDisplay("Autocorrelograms", buf->config_->getInt("ac.window.width"), buf->config_->getInt("ac.window.height"))
, BIN_SIZE(buf->SAMPLING_RATE/1000 * bin_size_ms)
, MAX_CLUST(buffer->config_->getInt("ac.max.clust", 30))
, NBINS(nbins)
, wait_clustering_(buffer->config_->getBool("ac.wait.clust", true)){

	const unsigned int tetrn = buf->tetr_info_->tetrodes_number();

	autocorrs_.resize(tetrn);
	cross_corrs_.resize(tetrn);
	spike_times_lists_.resize(tetrn);

	for (unsigned int i=0; i < tetrn; ++i) {
		autocorrs_[i].resize(MAX_CLUST);
		cross_corrs_[i].resize(MAX_CLUST);
		for (unsigned int c = 0; c < MAX_CLUST; ++c) {
			cross_corrs_[i][c].resize(MAX_CLUST);
			for (unsigned int c2 = 0; c2 < MAX_CLUST; ++c2) {
				cross_corrs_[i][c][c2].resize(NBINS);
			}
		}
		spike_times_lists_[i].resize(MAX_CLUST);

		for (unsigned int c=0; c < MAX_CLUST; ++c) {
			autocorrs_[i][c].resize(NBINS);
		}
	}
}

AutocorrelogramProcessor::~AutocorrelogramProcessor(){

}

void AutocorrelogramProcessor::process(){
	// IF the ACs on one tetrode have to be recalculated
	if (buffer->ac_reset_){

		// reset all
		if (buffer->ac_reset_cluster_ == -1){
			for (unsigned int c = 0; c < MAX_CLUST; ++c){
				for (unsigned int b = 0; b < NBINS; ++b){
					autocorrs_[buffer->ac_reset_tetrode_][c][b] = 0;
				}
			}
		}
		// reset 1 cluster
		else{
			for (unsigned int b = 0; b < NBINS; ++b){
				autocorrs_[buffer->ac_reset_tetrode_][buffer->ac_reset_cluster_][b] = 0;
			}
		}

		buffer->ac_reset_ = false;
		buffer->ac_reset_cluster_ = -1;
		buffer->ac_reset_tetrode_ = -1;
	}

	// TODO process user actions (no need in reset above then)
	// TODO don't process at every call

	while(user_context_.HasNewAction(last_proc_ua_id_)){
		// process new user action

		const UserAction *ua = user_context_.GetNextAction(last_proc_ua_id_);
		last_proc_ua_id_ = ua->id_;

		switch(ua->action_type_){
		case UA_CREATE_CLUSTER:
			// TODO extract from main and new cluster computation of AC
//			for (int i=0; i < buffer->spike_buf_pos_auto_; ++i){
//				Spike *spike = buffer->spike_buffer_[i];
//				if (spike == nullptr || spike->discarded_ || spike->cluster_id_ != ua->cluster_number_1_){
//					continue;
//				}
//			}

			// TODO !! extract start reset
			reset_mode_ = true;
			reset_cluster_ = ua->cluster_number_1_;
			reset_mode_end_ = buffer->spike_buf_pos_auto_;
			buffer->spike_buf_pos_auto_ = 0;
			break;

		case UA_SELECT_CLUSTER1:
			SetDisplayTetrode(display_tetrode_);

			break;
		case UA_SELECT_CLUSTER2:

			SetDisplayTetrode(display_tetrode_);

			break;
		case UA_DELETE_CLUSTER:
			clearACandCCs(ua->cluster_number_1_);
			SetDisplayTetrode(display_tetrode_);
			break;
		case UA_MERGE_CLUSTERS:{
			// cluster 1 was deleted and cluster 2 has to be updated
			int clun = ua->cluster_number_1_;
			SetDisplayTetrode(display_tetrode_);

			// merge CCs and ACs
			for(unsigned int i = 0; i < NBINS; ++i){
				autocorrs_[display_tetrode_][clun][i] += autocorrs_[display_tetrode_][ua->cluster_number_2_][i];
				autocorrs_[display_tetrode_][clun][i] += cross_corrs_[display_tetrode_][ua->cluster_number_2_][clun][i];
				autocorrs_[display_tetrode_][clun][i] += cross_corrs_[display_tetrode_][clun][ua->cluster_number_2_][i];
			}

			for (unsigned int c = 0; c < MAX_CLUST; ++c) {
				for (unsigned int b = 0; b < NBINS; ++b) {
					cross_corrs_[display_tetrode_][clun][c][b] += cross_corrs_[display_tetrode_][ua->cluster_number_2_][c][b];
					cross_corrs_[display_tetrode_][c][clun][b] += cross_corrs_[display_tetrode_][c][ua->cluster_number_2_][b];
				}
			}

			clearACandCCs(ua->cluster_number_2_);

			break;
		}

		case UA_ADD_EXCLUSIVE_PROJECTION:
			reset_mode_ = true;
			reset_mode_end_ = buffer->spike_buf_pos_auto_;
			reset_cluster_ = ua->cluster_number_1_;
			buffer->spike_buf_pos_auto_ = 0;
			clearACandCCs(ua->cluster_number_1_);

			break;

		case UA_REMOVE_PROJECTION:
			break;

		case UA_ADD_INCLUSIVE_PROJECTION:
			break;

		case UA_CUT_SPIKES:
			break;

		case UA_NONE:
			break;
		}
	}

	while(buffer->spike_buf_pos_auto_ < buffer->spike_buf_no_disp_pca){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_auto_];

		if (spike->discarded_){
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

		if (reset_mode_ && (unsigned int)spike->tetrode_ != display_tetrode_){
			buffer->spike_buf_pos_auto_++;
			continue;
		}

		unsigned int tetrode = spike->tetrode_;
		unsigned int cluster_id = spike->cluster_id_;
		unsigned int stime = spike->pkg_id_;

		// update autocorrelation bins
		std::list<unsigned int>& prev_spikes_queue = spike_times_lists_[tetrode][cluster_id];

		unsigned int max_diff = BIN_SIZE * NBINS;

		// update autocorrs for new spike`
		for (std::list<unsigned int>::iterator si = prev_spikes_queue.begin(); si != prev_spikes_queue.end(); ++si) {
			if (stime - *si > max_diff){
				continue;
			}

			unsigned int bin = (stime - *si) / (BIN_SIZE );
			if (bin >= NBINS || bin < 0){
				continue;
			}

			autocorrs_[tetrode][cluster_id][bin]++;
		}

		// fill cross-correlograms - no need to delete here, start from end until
		// TODO number of clusters ?
		for (unsigned int c = 0; c < MAX_CLUST; ++c) {
			if (c == (unsigned int)spike->cluster_id_)
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

		// remove spikes from queue beyond AC / CC windows
		while(!prev_spikes_queue.empty() && stime - prev_spikes_queue.front() > max_diff)
			prev_spikes_queue.pop_front();

		// replace new spike with last in the buf and advance pointer
		prev_spikes_queue.push_back(stime);

		buffer->spike_buf_pos_auto_++;

		// TODO ??? check change in performance
		//plotACorCCs(tetrode, spike->cluster_id_);


	}

	// RESET
	// TODO process only given cluster in the reset mode
	if (reset_mode_ && buffer->spike_buf_pos_auto_ >= reset_mode_end_){
		reset_mode_ = false;
		reset_cluster_ = -1;
		SetDisplayTetrode(display_tetrode_);
	}
}

// center if the cluster autocorrelograms
unsigned int AutocorrelogramProcessor::getXShift(int clust) {
	return ((BWIDTH + 1) * NBINS * 2 + 15) * (clust % (XCLUST / 2) + 0.5);
}

unsigned int AutocorrelogramProcessor::getYShift(int clust) {
	return (clust / (XCLUST / 2)) * ypix_ + 100;
}


unsigned int AutocorrelogramProcessor::getCCXShift(const unsigned int& clust1,
		const unsigned int& clust2) {
	// TODO below
	// 	+ set corresponding when a pair is chosen
	//  + cluster select
	// 	+ miniaturized preview (with 1-sized bins without interval)
	 return ((CC_BWIDTH + 1) * NBINS * 2 + 15) * (((int)clust1) % XCLUST_CC) + (CC_BWIDTH + 1) * NBINS;
}

unsigned int AutocorrelogramProcessor::getCCYShift(const unsigned int& clust1,
		const unsigned int& clust2) {
	unsigned int shift = (((int)clust2 % YCLUST_CC)) * ypix_ + 100;
	return shift;
}


void AutocorrelogramProcessor::drawClusterRect(int clust) {
	int xsh = getXShift(clust);
	int ysh = getYShift(clust);

	SDL_Rect rect;
	int height = ypix_;
	rect.h = height;
	rect.w = 2 * (BWIDTH + 1) * NBINS;
	rect.x = xsh - (BWIDTH + 1) * NBINS;
	rect.y = ysh - height;
	SDL_RenderDrawRect(renderer_, &rect);
}

int AutocorrelogramProcessor::getClusterNumberByCoords(const unsigned int& x,
		const unsigned int& y) {
	int cx = round( ((int)x - (int)(BWIDTH + 1) * (int)NBINS) / float((BWIDTH + 1) * NBINS * 2 + 15));
	int cy = y / ypix_;

	return cy * (display_mode_ == AC_DISPLAY_MODE_AC ? XCLUST / 2 : XCLUST) + cx;
}

void AutocorrelogramProcessor::plotACorCCs(int tetrode, int cluster) {
	if (display_mode_ == AC_DISPLAY_MODE_AC){
		plotAC(tetrode, cluster);
	}
	else{
		for (int c = 0; c < std::min<int>(YCLUST_CC, MAX_CLUST - shift_xx_y_ * YCLUST_CC); ++c){
			plotCC(tetrode, cluster + shift_xx_x_ * XCLUST_CC, c + shift_xx_y_ * YCLUST_CC);
		}
	}
}

void AutocorrelogramProcessor::clearACandCCs(const unsigned int& clu) {
	for (unsigned int i=0; i < NBINS; ++i){
		autocorrs_[display_tetrode_][clu][i] = 0;
	}

	for (unsigned int c=0; c < MAX_CLUST; ++c){
		for (unsigned int i=0; i < NBINS; ++i){
			cross_corrs_[display_tetrode_][clu][c][i] = 0;
			cross_corrs_[display_tetrode_][c][clu][i] = 0;
		}
	}
}

void AutocorrelogramProcessor::resetACandCCs(const unsigned int& clu) {
}

void AutocorrelogramProcessor::SetDisplayTetrode(const unsigned int& display_tetrode){
	if (display_tetrode_ >= buffer->tetr_info_->tetrodes_number())
		return;

	// TODO extract redraw

	display_tetrode_ = display_tetrode;

	SDL_SetRenderTarget(renderer_, texture_);
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	//SDL_RenderPresent(renderer_);
	const unsigned int YCLUST = (window_height_ - 100)/ ypix_;
	int limit = (display_mode_ == AC_DISPLAY_MODE_AC ? (unsigned int)(XCLUST / 2 * YCLUST ) : (std::min<int>(XCLUST_CC, (int)MAX_CLUST - XCLUST_CC * shift_xx_x_)));
	for (int c=0; c < limit; ++c) {
		plotACorCCs(display_tetrode_, c);
	}

	if (display_mode_ == AC_DISPLAY_MODE_AC){
		if (user_context_.SelectedCluster1() >= 0){
			SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 255);
			drawClusterRect(user_context_.SelectedCluster1());
		}

		if (user_context_.SelectedCluster2() >= 0){
			SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
			drawClusterRect(user_context_.SelectedCluster2());
		}
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
					if (clun == user_context_.SelectedCluster1()){
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
					if (clun == user_context_.SelectedCluster2()){
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
			for (unsigned int c=0; c < MAX_CLUST; ++c){
				plotACorCCs(display_tetrode_, c);
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

		// switch between modes
		case SDLK_TAB:
			if (display_mode_ == AC_DISPLAY_MODE_AC){
				display_mode_ = AC_DISPLAY_MODE_CC;
				SDL_SetWindowTitle(window_, "Cross-correlograms");
			}
			else{
				display_mode_ = AC_DISPLAY_MODE_AC;
				SDL_SetWindowTitle(window_, "Auto-correlograms");
			}
			SetDisplayTetrode(display_tetrode_);



			break;

		case SDLK_RIGHT:
			shift_xx_x_ += 1;
			SetDisplayTetrode(display_tetrode_);
			break;

		case SDLK_LEFT:
			if(shift_xx_x_ > 0)
				shift_xx_x_ -= 1;
			SetDisplayTetrode(display_tetrode_);
			break;

		case SDLK_UP:
			if(shift_xx_y_ > 0)
				shift_xx_y_ -= 1;
			SetDisplayTetrode(display_tetrode_);
			break;

		case SDLK_DOWN:
			shift_xx_y_ ++;
			SetDisplayTetrode(display_tetrode_);
			break;

		default:
			break;
		}
	}
}

void AutocorrelogramProcessor::plotCC(const unsigned int& tetr,
		const unsigned int& cluster1, const unsigned int& cluster2) {
	if (tetr != display_tetrode_)
			return;

	// shift for the plot

	const int xsh = getCCXShift(cluster1, cluster2);
	const int ysh = getCCYShift(cluster1, cluster2);

	ColorPalette palette_ = ColorPalette::BrewerPalette12;

	SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
	SDL_RenderDrawLine(renderer_, xsh, 0, xsh, window_height_);

	int maxh = 0;
	for (unsigned int b = 0; b < NBINS; ++b) {
		int height = cross_corrs_[tetr][cluster1][cluster2][b];
		if (height > maxh)
			maxh = height;
		height = cross_corrs_[tetr][cluster2][cluster1][b];
		if (height > maxh)
			maxh = height;
	}

	for (unsigned int b=0; b < NBINS; ++b) {
		int height = (cross_corrs_[tetr][cluster1][cluster2][b]) * ypix_/ maxh;

		SDL_Rect rect;
		rect.h = height;
		rect.w = CC_BWIDTH;
		rect.x = xsh + b * (CC_BWIDTH + 1);
		rect.y = ysh - height;

		SDL_SetRenderDrawColor(renderer_, palette_.getR(cluster1 % palette_.NumColors()), palette_.getG(cluster1 % palette_.NumColors()), palette_.getB(cluster1 % palette_.NumColors()), 255);
		SDL_RenderFillRect(renderer_, &rect);

		// other side
		height = (cross_corrs_[tetr][cluster2][cluster1][b]) * ypix_/ maxh;
		rect.h = height;
		rect.x = xsh - b * (CC_BWIDTH + 1);
		rect.y = ysh - height;

		SDL_SetRenderDrawColor(renderer_, palette_.getR(cluster2 % palette_.NumColors()), palette_.getG(cluster2 % palette_.NumColors()), palette_.getB(cluster2 % palette_.NumColors()), 255);
		SDL_RenderFillRect(renderer_, &rect);
	}

	// if refractoriness is good, mark couple with a frame
	// TODO parametrize which bins are accounted for as refractory ac.refractory.bins=2 ...
	int sum_total = 0;
	int sum_refractory = cross_corrs_[tetr][cluster1][cluster2][0] + cross_corrs_[tetr][cluster1][cluster2][1] + cross_corrs_[tetr][cluster2][cluster1][0] + cross_corrs_[tetr][cluster2][cluster1][1];;
	for (unsigned int b=0; b < NBINS; ++b){
		sum_total += cross_corrs_[tetr][cluster1][cluster2][b];
		sum_total += cross_corrs_[tetr][cluster2][cluster1][b];
	}

	if (sum_refractory / (double)sum_total < refractory_fraction_threshold_){
		// draw frame
		SDL_Rect rect;
		rect.h = ypix_;
		rect.w = (CC_BWIDTH + 1) * 2 * NBINS;
		rect.x = xsh - (CC_BWIDTH + 1) * NBINS;
		rect.y = ysh - ypix_;

		SDL_SetRenderDrawColor(renderer_, 255, 255, 0, 255);
		SDL_RenderDrawRect(renderer_, &rect);
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
	for (unsigned int b = 0; b < NBINS; ++b) {
		int height = autocorrs_[tetr][cluster][b];
		if (height > maxh)
			maxh = height;
	}

	for (unsigned int b=0; b < NBINS; ++b) {
		int height = (autocorrs_[tetr][cluster][b]) * ypix_/ maxh;

		SDL_Rect rect;
		rect.h = height;
		rect.w = BWIDTH;
		rect.x = xsh + b * (BWIDTH + 1);
		rect.y = ysh - height;

		SDL_Rect rectmir;
		rectmir.h = height;
		rectmir.w = BWIDTH;
		rectmir.x = xsh - (b + 1) * (BWIDTH + 1);
		rectmir.y = ysh - height;

		SDL_SetRenderDrawColor(renderer_, palette_.getR(cluster % palette_.NumColors()), palette_.getG(cluster % palette_.NumColors()), palette_.getB(cluster % palette_.NumColors()), 255);
		SDL_RenderFillRect(renderer_, &rect);
		SDL_RenderFillRect(renderer_, &rectmir);
	}
}


