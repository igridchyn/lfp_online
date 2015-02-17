/*
 * SpeedEstimationProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "SpeedEstimationProcessor.h"
#include "OnlineEstimator.cpp"

SpeedEstimationProcessor::SpeedEstimationProcessor(LFPBuffer *buffer)
: LFPProcessor(buffer) {
	dump_.open("../out/spike_speed.txt");
}

SpeedEstimationProcessor::~SpeedEstimationProcessor() {
}

void SpeedEstimationProcessor::process(){
	// speed estimation
	// TODO: use average of bx, sx or alike
	// TODO: deal with missing points

	// SPEED is estimated as a mean displacement in the range of 16 position samples across few subsequent displacements

	while(buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS < buffer->pos_buf_pos_){
		float bx = buffer->positions_buf_[buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS].x_pos();
		float by = buffer->positions_buf_[buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS].y_pos();

		if (buffer->pos_buf_pos_speed_est > ESTIMATION_RADIUS && bx != buffer->pos_unknown_ && buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].valid){
			float dx = bx - buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].x_pos();
			float dy = by - buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].y_pos();
			// TODO: make internal
			buffer->speedEstimator_->push(sqrt(dx * dx + dy * dy));
			// TODO: float / scale ?
			buffer->positions_buf_[buffer->pos_buf_pos_speed_est].speed_ = buffer->speedEstimator_->get_mean_estimate();
			//            std::cout << "speed= " << buffer->speedEstimator_->get_mean_estimate() << "\n";

			// update spike speed
			// TODO: independent on previous operation ?
			int known_speed_pkg_id = buffer->positions_buf_[buffer->pos_buf_pos_speed_est].pkg_id_;
			while (true){
				Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];
				if (spike == nullptr || spike->pkg_id_ > known_speed_pkg_id){
					break;
				}

				// find last position sample before the spike
				while(buffer->pos_buf_pos_spike_speed_ < buffer->pos_buf_pos_speed_est && buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_ + 1].pkg_id_ < spike->pkg_id_){
					buffer->pos_buf_pos_spike_speed_ ++;
				}

				// interpolate speed during spike :
				int diff_bef = spike->pkg_id_ - buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_].pkg_id_;
				int diff_aft = (int)(known_speed_pkg_id - spike->pkg_id_);
				float w_bef = 1/(float)(diff_bef + 1);
				float w_aft = 1/(float)(diff_aft + 1);
				// TODO: weights ?
				spike->speed = ( buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_].speed_ * w_bef + buffer->positions_buf_[buffer->pos_buf_pos_speed_est].speed_ * w_aft) / (float)(w_bef + w_aft);
				//                std::cout << "Spike speed " << spike->speed << "\n";

				dump_ << spike->x << " " << spike->y << " " << spike->speed << "\n";

				buffer->spike_buf_pos_speed_ ++;
			}
		}

		buffer->pos_buf_pos_speed_est ++;
	}

	// ignore estimation for spikes being to far from the first known estimate point
	if (buffer->pos_buf_pos_speed_est > ESTIMATION_RADIUS){
		while (buffer->spike_buf_pos_speed_< buffer->spike_buf_pos){
			Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];

			if (spike == nullptr)
				break;

			if (spike->pkg_id_ > buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].pkg_id_)
				break;

			// leave speed estimation as nanf("")
			buffer->spike_buf_pos_speed_ ++;
		}
	}
}
