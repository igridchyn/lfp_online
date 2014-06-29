/*
 * TransProbEstimationProcessor.cpp
 *
 *  Created on: Jun 28, 2014
 *      Author: igor
 */

#include "TransProbEstimationProcessor.h"
#include <assert.h>

TransProbEstimationProcessor::TransProbEstimationProcessor(LFPBuffer *buf, const unsigned int nbins, const unsigned int bin_size,
		const unsigned int neighb_size, const unsigned int step, const std::string base_path)
	: LFPProcessor(buf)
	, NBINS(nbins)
	, BIN_SIZE(bin_size)
	, NEIGHB_SIZE(neighb_size)
	, STEP(step)
	, BASE_PATH(base_path)
	, pos_buf_ptr_(STEP){
	// TODO Auto-generated constructor stub
	assert(NEIGHB_SIZE % 2);
	trans_probs_.resize(NBINS * NBINS, arma::mat(NEIGHB_SIZE, NEIGHB_SIZE, arma::fill::zeros));
}

TransProbEstimationProcessor::~TransProbEstimationProcessor() {
	// TODO Auto-generated destructor stub
}

void TransProbEstimationProcessor::process() {
	// TODO delay
	while(pos_buf_ptr_ < buffer->pos_buf_pos_){
		if (buffer->positions_buf_[pos_buf_ptr_][0] == 1023 || buffer->positions_buf_[pos_buf_ptr_ - STEP][0] == 1023){
			pos_buf_ptr_ ++;
			continue;
		}

		// bins in shift rather than shift in bins (more precise)
		int b_shift_x = (int) round(((int)buffer->positions_buf_[pos_buf_ptr_][0] - (int)buffer->positions_buf_[pos_buf_ptr_ - STEP][0]) / (float)BIN_SIZE - 0.5);
		int b_shift_y = (int) round(((int)buffer->positions_buf_[pos_buf_ptr_][1] - (int)buffer->positions_buf_[pos_buf_ptr_ - STEP][1]) / (float)BIN_SIZE - 0.5);

		int xb =  (int)round(buffer->positions_buf_[pos_buf_ptr_ - STEP][0] / (float)BIN_SIZE - 0.5);
		int yb =  (int)round(buffer->positions_buf_[pos_buf_ptr_ - STEP][1] / (float)BIN_SIZE - 0.5);

		int shift_coord_x = b_shift_x + (int)NEIGHB_SIZE / 2;
		int shift_coord_y = b_shift_y + (int)NEIGHB_SIZE / 2;

		if (shift_coord_x >=0 && shift_coord_x < NEIGHB_SIZE && shift_coord_y >=0 && shift_coord_y < NEIGHB_SIZE){
			trans_probs_[NBINS * yb + xb](shift_coord_x, shift_coord_y) += 1;
		}

		pos_buf_ptr_ ++;
	}

	if (buffer->last_pkg_id > 30000000 && !saved){
		arma::mat tps(NEIGHB_SIZE, NBINS * NBINS * NEIGHB_SIZE);

		for (int b = 0; b < NBINS * NBINS; ++b) {
			double psum = arma::sum(arma::sum(trans_probs_[b]));
			trans_probs_[b] /= psum;
			tps.cols(b*NEIGHB_SIZE, (b+1)*NEIGHB_SIZE-1) = trans_probs_[b];
			tps.save(BASE_PATH + "tps.mat");
		}

		buffer->tps_ = trans_probs_;

		// DEBUG
		trans_probs_[210].save(BASE_PATH + "tp.mat", arma::raw_ascii);
		trans_probs_[190].save(BASE_PATH + "tp2.mat", arma::raw_ascii);
		trans_probs_[230].save(BASE_PATH + "tp3.mat", arma::raw_ascii);
		trans_probs_[192].save(BASE_PATH + "tp4.mat", arma::raw_ascii);
		saved = true;
	}
}
