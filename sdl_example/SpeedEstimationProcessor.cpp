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
	// TODO Auto-generated constructor stub

}

SpeedEstimationProcessor::~SpeedEstimationProcessor() {
	// TODO Auto-generated destructor stub
}

void SpeedEstimationProcessor::process(){
	// speed estimation
	// TODO: use average of bx, sx or alike
	// TODO: deal with missing points

	while(buffer->pos_buf_pos_speed_est < buffer->pos_buf_pos_){
		unsigned int bx = buffer->positions_buf_[buffer->pos_buf_pos_speed_est][0];
		unsigned int by = buffer->positions_buf_[buffer->pos_buf_pos_speed_est][1];

		if (buffer->pos_buf_pos_speed_est > 16 && bx != 1023 && buffer->positions_buf_[buffer->pos_buf_pos_speed_est - 16][0] != 1023){
			float dx = (float)bx - buffer->positions_buf_[buffer->pos_buf_pos_speed_est - 16][0];
			float dy = (float)by - buffer->positions_buf_[buffer->pos_buf_pos_speed_est - 16][1];
			// TODO: make internal
			buffer->speedEstimator_->push(sqrt(dx * dx + dy * dy));
			// TODO: float / scale ?
			buffer->positions_buf_[buffer->pos_buf_pos_speed_est - 8][5] = buffer->speedEstimator_->get_mean_estimate();
			//            std::cout << "speed= " << buffer->speedEstimator_->get_mean_estimate() << "\n";

			// update spike speed
			// TODO: independent on previous operation ?
			int known_speed_pkg_id =  buffer->positions_buf_[buffer->pos_buf_pos_speed_est - 8][4];
			while (true){
				Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];
				if (spike == NULL || spike->pkg_id_ > known_speed_pkg_id){
					break;
				}

				// find last position sample before the spike
				while(buffer->pos_buf_pos_spike_speed_ < buffer->pos_buf_pos_speed_est - 8 && buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_ + 1][4] < spike->pkg_id_){
					buffer->pos_buf_pos_spike_speed_ ++;
				}

				// interpolate speed during spike :
				int diff_bef = spike->pkg_id_ - buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_][4];
				int diff_aft = (int)(known_speed_pkg_id - spike->pkg_id_);
				float w_bef = 1/(float)(diff_bef + 1);
				float w_aft = 1/(float)(diff_aft + 1);
				// TODO: weights ?
				spike->speed = ( buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_][5] * w_bef + buffer->positions_buf_[buffer->pos_buf_pos_speed_est - 8][5] * w_aft) / (float)(w_bef + w_aft);
				//                std::cout << "Spike speed " << spike->speed << "\n";

				buffer->spike_buf_pos_speed_ ++;
			}
		}

		buffer->pos_buf_pos_speed_est ++;
	}
}
