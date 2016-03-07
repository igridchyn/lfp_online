/*
 * TransProbEstimationProcessor.cpp
 *
 *  Created on: Jun 28, 2014
 *      Author: igor
 */

#include "TransProbEstimationProcessor.h"
#include "PlaceField.h"

#include <assert.h>

TransProbEstimationProcessor::TransProbEstimationProcessor(LFPBuffer* buf)
	: TransProbEstimationProcessor(buf,
			buf->config_->getInt("nbinsx"),
			buf->config_->getInt("nbinsy"),
			buf->config_->getInt("bin.size"),
			buf->config_->getInt("kd.hmm.neighb.rad")*2 + 1,
			buf->config_->getInt("tp.step"),
			buf->config_->getOutPath("kd.path.base"),
			buf->config_->getBool("tp.save"),
			buf->config_->getBool("tp.load"),
			buf->config_->getBool("tp.smooth"),
			buf->config_->getBool("tp.use.parametric"),
			buf->config_->getFloat("tp.par.sigma"),
			buf->config_->getInt("tp.par.spread")
	){
}

TransProbEstimationProcessor::TransProbEstimationProcessor(LFPBuffer *buf, const unsigned int nbinsx, const unsigned int nbinsy, const unsigned int bin_size,
		const unsigned int neighb_size, const unsigned int step, const std::string base_path, const bool save,
		const bool load, const bool smooth, const bool use_parametric, const float sigma, const int spread)
	: LFPProcessor(buf)
	, NBINSX(nbinsx)
	, NBINSY(nbinsy)
	, BIN_SIZE(bin_size)
	, NEIGHB_SIZE(neighb_size)
	, STEP(step)
	, BASE_PATH(base_path)
	, pos_buf_ptr_(STEP)
	, SAVE(save)
	, LOAD(load)
	, SMOOTH(smooth)
	, USE_PARAMETRIC(use_parametric)
	, SIGMA(sigma)
	, SPREAD(spread)
	, SAMPLING_END_(buf->config_->getInt("tp.sampling.end", 15000000)){
	assert(NEIGHB_SIZE % 2);
	trans_probs_.resize(NBINSX * NBINSY, arma::mat(NEIGHB_SIZE, NEIGHB_SIZE, arma::fill::zeros));

	if (LOAD){
		buffer->log_string_stream_ << "Load TPs, smoothing " << ( SMOOTH ? "enabled" : "disabled" ) << "...";
		arma::mat tps;
		tps.load(BASE_PATH + "tps.mat");

		if (USE_PARAMETRIC){
			buffer->log_string_stream_ << "WARNING: parametric TPs used instead of loading estimates...\n";
		}

		for (unsigned int b = 0; b < NBINSX * NBINSY; ++b) {
			// extract
			trans_probs_[b] = tps.cols(b*NEIGHB_SIZE, (b+1)*NEIGHB_SIZE-1);

			// DEBUG
//			trans_probs_[b].save(BASE_PATH + "tp_" + Utils::Converter::int2str(b) + "_raw.mat", arma::raw_ascii);

			// TODO do the following in the estimation stage, before saving
			// SMOOTH NON-PAREMETRIC ESTIMATE (counts)
			if (SMOOTH){
				PlaceField tp_pf(trans_probs_[b], SIGMA, BIN_SIZE, SPREAD);
				trans_probs_[b] = tp_pf.Smooth().Mat();
			}

			// PARAMETRIC: GAUSSIAN
			if (USE_PARAMETRIC){
				PlaceField tp_pf(arma::mat(NEIGHB_SIZE, NEIGHB_SIZE, arma::fill::zeros), SIGMA, BIN_SIZE, SPREAD);
				tp_pf(NEIGHB_SIZE/2, NEIGHB_SIZE/2) = 1;
				trans_probs_[b] = tp_pf.Smooth().Mat();
			}

			// normalize
			double sum = arma::sum(arma::sum(trans_probs_[b]));

			trans_probs_[b] /= sum;

			// log-transform
			trans_probs_[b] = arma::log(trans_probs_[b]);

			// replace nans with large negative value
			for (unsigned int dx = 0; dx < NEIGHB_SIZE; ++dx) {
				for (unsigned int dy = 0; dy < NEIGHB_SIZE; ++dy) {
					// WORKAROUND
					if (std::isnan<float>(trans_probs_[b](dx, dy))){ // || isinf(trans_probs_[b](dx, dy))){
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

void TransProbEstimationProcessor::process() {
	// TODO delay

	while(pos_buf_ptr_ < buffer->pos_buf_pos_){
		if (!buffer->positions_buf_[pos_buf_ptr_].valid || !buffer->positions_buf_[pos_buf_ptr_ - STEP].valid){
			pos_buf_ptr_ ++;
			continue;
		}

		// bins in shift rather than shift in bins (more precise)
		int b_shift_x = (int) round(((int)buffer->positions_buf_[pos_buf_ptr_].x_pos() - (int)buffer->positions_buf_[pos_buf_ptr_ - STEP].x_pos()) / (float)BIN_SIZE);
		int b_shift_y = (int) round(((int)buffer->positions_buf_[pos_buf_ptr_].y_pos() - (int)buffer->positions_buf_[pos_buf_ptr_ - STEP].y_pos()) / (float)BIN_SIZE);

		unsigned int tmpx = buffer->positions_buf_[pos_buf_ptr_ - STEP].x_pos();
		int xb =  (int)round(buffer->positions_buf_[pos_buf_ptr_ - STEP].x_pos() / (float)BIN_SIZE - 0.5);
		unsigned int tmpy = buffer->positions_buf_[pos_buf_ptr_ - STEP].y_pos();
		int yb =  (int)round(buffer->positions_buf_[pos_buf_ptr_ - STEP].y_pos() / (float)BIN_SIZE - 0.5);

		if (tmpx == 0)
			xb = 0;

		if (tmpy == 0)
			yb = 0;

		unsigned int shift_coord_x = b_shift_x + (int)NEIGHB_SIZE / 2;
		unsigned int shift_coord_y = b_shift_y + (int)NEIGHB_SIZE / 2;

		if (shift_coord_x >=0 && shift_coord_x < NEIGHB_SIZE && shift_coord_y >=0 && shift_coord_y < NEIGHB_SIZE){
			trans_probs_[NBINSX * yb + xb](shift_coord_x, shift_coord_y) += 1;
		}

		pos_buf_ptr_ ++;
	}

	// TODO configurable interval
	if (buffer->last_pkg_id > SAMPLING_END_ && !saved && SAVE){
		buffer->Log("save tps...");

		arma::mat tps(NEIGHB_SIZE, NBINSX * NBINSY * NEIGHB_SIZE);

		for (unsigned int b = 0; b < NBINSX * NBINSY; ++b) {
			double psum = arma::sum(arma::sum(trans_probs_[b]));
			trans_probs_[b] /= psum;
			tps.cols(b*NEIGHB_SIZE, (b+1)*NEIGHB_SIZE-1) = trans_probs_[b];
		}
		tps.save(BASE_PATH + "tps.mat");
		Log(std::string("Saved tps at ") + BASE_PATH + "tps.mat");

		buffer->tps_ = trans_probs_;
		saved = true;
		buffer->Log("done");
	}
}
