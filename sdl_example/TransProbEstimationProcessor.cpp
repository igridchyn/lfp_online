/*
 * TransProbEstimationProcessor.cpp
 *
 *  Created on: Jun 28, 2014
 *      Author: igor
 */

#include "TransProbEstimationProcessor.h"
#include "PlaceField.h"
#include "Utils.h"

#include <assert.h>

// OPTIONS
// 1. Degree of interpolation. Default: only between 2 subsequent valid tracking entries
// 2. Overlapping of estimation windows. Default: no overlap.

TransProbEstimationProcessor::TransProbEstimationProcessor(LFPBuffer *buf)
	: LFPProcessor(buf)
	, NBINSX(buf->config_->getInt("nbinsx"))
	, NBINSY(buf->config_->getInt("nbinsy"))
	, BIN_SIZE(buf->config_->getInt("bin.size"))
	, NEIGHB_SIZE(buf->config_->getInt("kd.hmm.neighb.rad")*2 + 1)
	, STEP(buf->config_->getInt("tp.step"))
	, BASE_PATH(buf->config_->getOutPath("kd.path.base"))
	, pos_buf_ptr_(buf->pos_buf_trans_prob_est_)
	, SAVE(buf->config_->getBool("tp.save"))
	, LOAD(buf->config_->getBool("tp.load"))
	, SMOOTH(buf->config_->getBool("tp.smooth"))
	, USE_PARAMETRIC(buf->config_->getBool("tp.use.parametric"))
	, SIGMA(buf->config_->getFloat("tp.par.sigma"))
	, SPREAD(buf->config_->getInt("tp.par.spread"))
	, SAMPLING_END_(buf->config_->getInt("tp.sampling.end", 15000000))
	, ESTIMATION_WINDOW(buf->config_->getInt("tp.estimation.window"))
	, tp_path_(buf->config_->getOutPath("tp.filename")){

	assert(NEIGHB_SIZE % 2);
	if (NBINSY > 1){
		trans_probs_.resize(NBINSX * NBINSY, arma::fmat(NEIGHB_SIZE, NEIGHB_SIZE, arma::fill::zeros));
	} else {
		Log("Trans probs: linear environment");
		trans_probs_.resize(NBINSX * NBINSY, arma::fmat(NEIGHB_SIZE, 1, arma::fill::zeros));
	}

//	pos_buf_ptr_ += STEP;

	if (LOAD){
		buffer->log_string_stream_ << "Load TPs, smoothing " << ( SMOOTH ? "enabled" : "disabled" ) << "...";
		arma::fmat tps;
		if (! Utils::FS::FileExists(BASE_PATH + "tps.mat")){
			Log(std::string("ERROR: The following file wasn't found: ") + BASE_PATH + "tps.mat");
			exit(23456);
		}

		tps.load(BASE_PATH + "tps.mat");

		if (USE_PARAMETRIC){
			buffer->log_string_stream_ << "WARNING: parametric TPs used instead of loading estimates...\n";
		}

		unsigned int dim2 = NBINSY > 1 ? NEIGHB_SIZE :1;

		for (unsigned int b = 0; b < NBINSX * NBINSY; ++b) {
			// extract
			trans_probs_[b] = tps.cols(b*dim2, (b+1)*dim2-1);

			// DEBUG
//			trans_probs_[b].save(BASE_PATH + "tp_" + Utils::Converter::int2str(b) + "_raw.mat", arma::raw_ascii);

			// TODO do the following in the estimation stage, before saving
			// SMOOTH NON-PAREMETRIC ESTIMATE (counts)
			if (SMOOTH){
				PlaceField tp_pf(arma::conv_to<arma::mat>::from(trans_probs_[b]), SIGMA, BIN_SIZE, SPREAD);
				trans_probs_[b] = arma::conv_to<arma::fmat>::from(tp_pf.Smooth().Mat());
			}

			// PARAMETRIC: GAUSSIAN
			if (USE_PARAMETRIC){
				PlaceField tp_pf(arma::mat(NEIGHB_SIZE, NEIGHB_SIZE, arma::fill::zeros), SIGMA, BIN_SIZE, SPREAD);
				tp_pf(NEIGHB_SIZE/2, NEIGHB_SIZE/2) = 1;
				trans_probs_[b] = arma::conv_to<arma::fmat>::from(tp_pf.Smooth().Mat());
			}

			// normalize
			float sum = arma::sum(arma::sum(trans_probs_[b]));

			trans_probs_[b] /= sum;

			// log-transform
			trans_probs_[b] = arma::log(trans_probs_[b]);

			// replace nans with large negative value
			for (unsigned int dx = 0; dx < NEIGHB_SIZE; ++dx) {
				for (unsigned int dy = 0; dy < dim2; ++dy) {
					// WORKAROUND
					if (Utils::Math::Isnan(trans_probs_[b](dx, dy))){ // || isinf(trans_probs_[b](dx, dy))){
						trans_probs_[b](dx, dy) = -100000;
					}
				}
			}	

			// DEBUG
			// trans_probs_[b].save(BASE_PATH + "tp_" + Utils::Converter::int2str(b) + ".mat", arma::raw_ascii);
		}

		buffer->tps_ = trans_probs_;
		buffer->log_string_stream_ << "done\n";
		buffer->Log();
	}
}

TransProbEstimationProcessor::~TransProbEstimationProcessor() {
}

bool TransProbEstimationProcessor::interpolatedPositionAt(const unsigned int& pkg_id, const unsigned int& buf_pos, float& x, float& y){
	SpatialInfo& si = buffer->positions_buf_[buf_pos];
	SpatialInfo& si_prev = buffer->positions_buf_[buf_pos - 1];

	// if any of the two is invalid, try next window
	if (!(si.valid && si_prev.valid)){
		return false;
	}

	if (pkg_id == si.pkg_id_){
		x = si.x_pos();
		y = si.y_pos();
		return true;
	}

	if (pkg_id == si_prev.pkg_id_){
		x = si_prev.x_pos();
		y = si_prev.y_pos();
		return true;
	}

	float c1 = (si.pkg_id_ - pkg_id) / float(si.pkg_id_ - si_prev.pkg_id_);
	float c2 = (pkg_id - si_prev.pkg_id_) / float(si.pkg_id_ - si_prev.pkg_id_);
	x = c1 * si.x_pos() + c2 * si_prev.x_pos();
	y = c1 * si.y_pos() + c2 * si_prev.y_pos();

	return true;
}

void TransProbEstimationProcessor::process() {
	// if beyond estimation interval or data over
	if ( (buffer->last_pkg_id > SAMPLING_END_ || (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER && pos_buf_ptr_ == buffer->pos_buf_pos_)) && !saved  && SAVE ){
		buffer->Log("save tps...");

		unsigned int dim2 = (NBINSY > 1 ? NEIGHB_SIZE : 1);
		Log("Dim2 = ", dim2);
		arma::fmat tps(NEIGHB_SIZE, NBINSX * NBINSY * dim2);

		for (unsigned int b = 0; b < NBINSX * NBINSY; ++b) {
			float psum = arma::sum(arma::sum(trans_probs_[b]));
			trans_probs_[b] /= psum;
			tps.cols(b * dim2, (b+1) * dim2-1) = trans_probs_[b];
		}
		tps.save(BASE_PATH + "tps.mat", arma::raw_ascii);
		Log(std::string("Saved tps at ") + BASE_PATH + "tps.mat");

		buffer->tps_ = trans_probs_;
		saved = true;
		buffer->Log("done");
	}

	while(pos_buf_ptr_ < buffer->pos_buf_pos_){
		if (from_pkg_id_ == 0){
			from_pkg_id_ = buffer->positions_buf_[0].pkg_id_;
			Log("Start estimation from pkg id = ", from_pkg_id_);
			xf_ = buffer->positions_buf_[0].x_pos();
			yf_ = buffer->positions_buf_[0].y_pos();
		}

		// need estimate pos from?
		if (Utils::Math::Isnan(xf_)){
			// find firsrt larger
			while(pos_buf_ptr_ < buffer->pos_buf_pos_ && buffer->positions_buf_[pos_buf_ptr_].pkg_id_ < from_pkg_id_)
				pos_buf_ptr_ ++;

			// too early
			if (pos_buf_ptr_ == buffer->pos_buf_pos_)
				return;

			if (!interpolatedPositionAt(from_pkg_id_, pos_buf_ptr_, xf_, yf_)){
				from_pkg_id_ += ESTIMATION_WINDOW;
				continue;
			}
		}

		// at this stage have FROM coords
		while(pos_buf_ptr_ < buffer->pos_buf_pos_ && buffer->positions_buf_[pos_buf_ptr_].pkg_id_ < from_pkg_id_ + ESTIMATION_WINDOW)
						pos_buf_ptr_ ++;
		// too early
		if (pos_buf_ptr_ == buffer->pos_buf_pos_)
			return;

		float xt, yt;
		if (!interpolatedPositionAt(from_pkg_id_ + ESTIMATION_WINDOW, pos_buf_ptr_, xt, yt)){
			from_pkg_id_ += ESTIMATION_WINDOW;
			xf_ = nanf("");
			yf_ = nanf("");
			continue;
		}

		// bins in shift rather than shift in bins (more precise)
		// ??? HALF OF BIN SIZE IS A TRANSITION ???
		int b_shift_x = (int) (round(xt / (float)BIN_SIZE) -  round(xf_ / (float)BIN_SIZE));
		int b_shift_y = (int) (round(yt / (float)BIN_SIZE) -  round(yf_ / (float)BIN_SIZE));

		int xb =  (int)round(xf_ / (float)BIN_SIZE - 0.5);
		int yb =  (int)round(yf_ / (float)BIN_SIZE - 0.5);

		if (xb >= (int)NBINSX || yb >= (int)NBINSY || xb < 0 || yb < 0){
			Log("Position out of range, ignore");
			pos_buf_ptr_ ++;
			from_pkg_id_ += ESTIMATION_WINDOW;
			xf_ = nanf("");
			yf_ = nanf("");
			continue;
		}

		unsigned int shift_coord_x = b_shift_x + (int)NEIGHB_SIZE / 2;
		unsigned int shift_coord_y = NBINSY > 1 ? b_shift_y + (int)NEIGHB_SIZE / 2 : 0;

		if (shift_coord_x < NEIGHB_SIZE && shift_coord_y < NEIGHB_SIZE){
			trans_probs_[NBINSX * yb + xb](shift_coord_x, shift_coord_y) += 1;
		}

		pos_buf_ptr_ ++;

		from_pkg_id_ += ESTIMATION_WINDOW;
		xf_ = xt;
		yf_ = yt;
	}
}
