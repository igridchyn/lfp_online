/*
 * FiringRateEstimatorProcessor.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: igor
 */

#include "FiringRateEstimatorProcessor.h"

FiringRateEstimatorProcessor::FiringRateEstimatorProcessor(LFPBuffer *buf)
	: LFPProcessor(buf)
	, fr_estimate_delay_(buf->config_->getInt("frest.delay"))
	, wait_speed_(buf->config_->getBool("kd.wait.speed"))
	, speed_thold_(buf->config_->getFloat("kd.speed.thold"))
{
	// TODO frest sampling delay
//	spike_numbers_.resize(buf->tetr_info_->tetrodes_number());
}

FiringRateEstimatorProcessor::~FiringRateEstimatorProcessor() {

}

void FiringRateEstimatorProcessor::process() {
//	if (buffer->fr_estimated_ || buffer->last_pkg_id < fr_estimate_delay_)
//		return;
//
//	// estimate firing rates
//	// TODO: count before the estimate delay in a member vector !!!
//	std::vector<unsigned int> spike_numbers_;
//	spike_numbers_.resize(buffer->tetr_info_->tetrodes_number());
//	// TODO which pointer ?
//	for (unsigned int i=0; i < (wait_speed_ ? buffer->spike_buf_pos_speed_ : buffer->spike_buf_pos_unproc_); ++i){
//		Spike *spike = buffer->spike_buffer_[i];
//		if (spike == nullptr || spike->discarded_ || spike->speed < speed_thold_){
//			continue;
//		}
//
//		spike_numbers_[spike->tetrode_] ++;
//	}
//
//	for (size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
//			double firing_rate = spike_numbers_[t] * buffer->SAMPLING_RATE / double(fr_estimate_delay_);
//			std::stringstream ss;
//			ss << "Estimated firing rate for tetrode #" << t << ": " << firing_rate << " spk / sec\n";
//			unsigned int est_sec_left = (buffer->input_duration_ - SAMPLING_DELAY) / buffer->SAMPLING_RATE;
//			ss << "Estimated remaining data duration: " << est_sec_left / 60 << " min, " << est_sec_left % 60 << " sec\n";
////			tetrode_sampling_rates_.push_back(std::max<int>(0, (int)round(est_sec_left * firing_rate / MIN_SPIKES) - 1));
////			ss << "\t sampling rate (with speed thold) set to: " << tetrode_sampling_rates_[t] << "\n";
//			Log(ss.str());
//
//			buffer->fr_estimated_ = true;
//			buffer->fr_estimates_.push_back(firing_rate);
//	}
}
