/*
 * SpeedEstimationProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "SpeedEstimationProcessor.h"
#include "OnlineEstimator.cpp"

SpeedEstimationProcessor::SpeedEstimationProcessor(LFPBuffer *buffer)
: LFPProcessor(buffer)
, ESTIMATE_WINDOW_SPIKE_NUMBER(buffer->config_->getBool("speed.est.estimate.meansn", false))
, WIN_LEN(buffer->config_->getInt("speed.est.meansn.win"))
, RUNNING_SPEED_THOLD(buffer->config_->getInt("speed.est.meansn.thold"))
, SN_ESTIMATE_START(buffer->config_->getInt("speed.est.meansn.start", 0))
, SN_ESTIMATE_END(buffer->config_->getInt("speed.est.meansn.end"))
, estimate_directional_variance_(buffer->config_->getBool("speed.est.estimate.dirvar", false)){
	dump_.open("../spike_speed.txt");

	last_window_end_ = SN_ESTIMATE_START;
}

SpeedEstimationProcessor::~SpeedEstimationProcessor() {
}

void SpeedEstimationProcessor::process(){
	// speed estimation
	// TODO: deal with missing points

	// SPEED is estimated as a mean displacement in the range of 16 position samples across few subsequent displacements

	if (ESTIMATE_WINDOW_SPIKE_NUMBER && buffer->last_pkg_id > SN_ESTIMATE_END && !sn_estimate_reported_){
		sn_estimate_reported_ = true;
		std::stringstream ss;
		if (mean_spike_number_estimator_.n_samples() == 0){
			ss << "WARNING: cannot estimate number of spikes because no data points have been collected (probably, due to lack of tracking)\n";
		}else{
			ss << "Mean number of spikes int the " << WIN_LEN << " window and speed threshold " << RUNNING_SPEED_THOLD << " equals " << mean_spike_number_estimator_.get_mean_estimate();
		}
		Log(ss.str());
	}

	while(buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS < buffer->pos_buf_pos_){
		float bx = buffer->positions_buf_[buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS].x_pos();
		float by = buffer->positions_buf_[buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS].y_pos();

		if (buffer->pos_buf_pos_speed_est > ESTIMATION_RADIUS && !Utils::Math::Isnan(bx) && buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].valid){
			float dx = bx - buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].x_pos();
			float dy = by - buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].y_pos();

			// calculate head direction (dump if need)
			if (estimate_directional_variance_){
				float sinsum = .0f;
				float cossum = .0f;
				unsigned int nsamples = 0;
				for (unsigned int hdpos = buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS; hdpos < buffer->pos_buf_pos_speed_est + ESTIMATION_RADIUS; ++hdpos){
					SpatialInfo *si = &buffer->positions_buf_[hdpos], *sinext =&buffer->positions_buf_[hdpos + 1];
					if (Utils::Math::Isnan(si->x_pos()) || Utils::Math::Isnan(sinext->x_pos())){
						continue;
					}

					float angle = atan2(sinext->x_pos() - si->x_pos(), sinext->y_pos() - si->y_pos());
					sinsum += sin(angle);
					cossum += cos(angle);
					nsamples ++;
				}

				if (nsamples > 4){
					float dirvar = 1 - sqrt(sinsum * sinsum + cossum * cossum) / nsamples;
	//				dump_ << dirvar << " " << bx << " " << by << " " << buffer->speedEstimator_->get_mean_estimate() << "\n";
					buffer->positions_buf_[buffer->pos_buf_pos_speed_est].dirvar_ = dirvar;
				}
			}

			buffer->speedEstimator_->push(sqrt(dx * dx + dy * dy));
			buffer->positions_buf_[buffer->pos_buf_pos_speed_est].speed_ = buffer->speedEstimator_->get_mean_estimate();

			// update spike speed
			// TODO: independent on previous operation ?
			unsigned int known_speed_pkg_id = buffer->positions_buf_[buffer->pos_buf_pos_speed_est].pkg_id_;
			while (buffer->spike_buf_pos_speed_ < buffer->spike_buf_pos){
				Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];
				if (spike->pkg_id_ > known_speed_pkg_id){
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
				spike->speed = ( buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_].speed_ * w_bef + buffer->positions_buf_[buffer->pos_buf_pos_speed_est].speed_ * w_aft) / (float)(w_bef + w_aft);

				// dump_ << spike->x << " " << spike->y << " " << spike->speed << "\n";

				buffer->spike_buf_pos_speed_ ++;

				if (ESTIMATE_WINDOW_SPIKE_NUMBER && spike->pkg_id_ > SN_ESTIMATE_START && spike->pkg_id_ < SN_ESTIMATE_END){
					if (spike->pkg_id_ > last_window_end_ + WIN_LEN){
						if (spike->speed > RUNNING_SPEED_THOLD){
							mean_spike_number_estimator_.push(current_window_spikes_);
						}
						current_window_spikes_ = 1;
						last_window_end_ = last_window_end_ + WIN_LEN;
					} else if (!spike->discarded_){
						current_window_spikes_ ++;
					}
				}
			}
		}

		buffer->pos_buf_pos_speed_est ++;
	}

	// ignore estimation for spikes being to far from the first known estimate point
	if (buffer->pos_buf_pos_speed_est > ESTIMATION_RADIUS){
		while (buffer->spike_buf_pos_speed_< buffer->spike_buf_pos){
			Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];

			if (spike->pkg_id_ > buffer->positions_buf_[buffer->pos_buf_pos_speed_est - ESTIMATION_RADIUS].pkg_id_)
				break;

			// leave speed estimation as nanf("")
			buffer->spike_buf_pos_speed_ ++;
		}
	}
}
