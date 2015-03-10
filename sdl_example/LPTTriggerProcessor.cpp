/*
 * LPTTriggerProcessor.cpp
 *
 *  Created on: Aug 15, 2014
 *      Author: igor
 */

#include "LPTTriggerProcessor.h"
#ifdef _WIN32
#include "inpout32.h"
#endif

LPTTriggerProcessor::LPTTriggerProcessor(LFPBuffer *buffer)
	: LFPProcessor(buffer)
	, channel_(buffer->config_->getInt("lpt.trigger.channel"))
	, trigger_cooldown_(buffer->config_->getInt("lpt.trigger.cooldown"))
	, trigger_start_delay_(buffer->config_->getInt("lpt.trigger.start.delay") * buffer->SAMPLING_RATE)
	, dominance_confidence_threshold_(buffer->config_->getFloat("lpt.trigger.confidence.threshold", 0.01))
	, sync_max_duration_(buffer->config_->getInt("lpt.trigger.sync.max.duration", 0))
	, spike_buf_limit_ptr_(buffer->spike_buf_pos)
{
	Log("Constructor start");

	// TODO : read from config
	trigger_type_ = LPTTriggerType::EnvironmentDominance;
	std::string ttpath = "/tmp/tt";
	Log("Write trigger timestamps to " + ttpath);
	timestamp_log_.open(ttpath);

	// trigger type-specific initialization
	switch(trigger_type_){
		case LPTTriggerType::EnvironmentDominance:
			spike_buf_limit_ptr_ = buffer->spike_buf_pos_unproc_;
			break;
		default:
			break;
	}

#ifdef _WIN32
	//Opendriver();

	// way 2
	hInpOutDll = LoadLibrary("InpOut32.DLL");
	if (hInpOutDll != nullptr)
	{
		gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
		gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
		gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
		gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");
	}
	else{
		buffer->Log("ERROR: cannot load inpout32.DLL");
	}
#endif

	setLow();
	Log("Constructor done");
}

LPTTriggerProcessor::~LPTTriggerProcessor() {
	setLow();
	timestamp_log_.close();
}

void LPTTriggerProcessor::setLow() {
	//Out32(0x0378, 0xFF);
	//Out32(0xE050, 0xFF);

	// way2
#ifdef _WIN32
	gfpOut32(iPort, 0x00);
#endif

	LPT_is_high_ = false;

//	Log("detect DROP at ", buffer->buf_pos_trig_);
}

void LPTTriggerProcessor::setHigh() {
	//Out32(0x0378, 0x00);
	//Out32(0xE050, 0x00);

	//way 2
#ifdef _WIN32
	gfpOut32(iPort, 0xFF);
#endif

	LPT_is_high_ = true;
	last_trigger_time_ = buffer->last_pkg_id;

//	Log("detect CLIMB at ", buffer->buf_pos_trig_);
}

void LPTTriggerProcessor::process() {
	// check if need to turn off first
	if (LPT_is_high_ && buffer->last_pkg_id - last_trigger_time_ > pulse_length_){
		Log("STOP INHIBITION at ", buffer->last_pkg_id);
		setLow();
		LPT_is_high_ = false;

		Log("Statistics (ITH, ITO, ATH, ATO, TOT_I, TOT_A) :");
		buffer->log_string_stream_ << "\t" << events_inhibited_thold_ << " / " << events_inhibited_timeout_ << " / "
				<< events_allowed_thold_ << " / " << events_allowed_timeout_ << "; "
				<< (events_inhibited_thold_ + events_inhibited_timeout_) << " / " << (events_allowed_thold_ + events_allowed_timeout_) << "\n";
		buffer->Log();
	}

	// TODO detect start not since 0 but since first package id
	if (buffer->last_pkg_id < trigger_start_delay_){
		return;
	}
	else if (average_spikes_in_synchrony_tetrodes_ < 0){
		average_spikes_in_synchrony_tetrodes_ = buffer->AverageSynchronySpikesWindow();

		std::stringstream ss;
		ss << "Start inhibition, average number of spikes on synchrony tetrodes in population window of length " << buffer->POP_VEC_WIN_LEN << " = " << average_spikes_in_synchrony_tetrodes_;
		Log(ss.str());
	}

	switch(trigger_target_){
	case LPTTriggerTarget::LPTTargetLFP:
		while(buffer->buf_pos_trig_ < buffer->buf_pos){

			switch(trigger_type_){
			case DoubleThresholdCrossing:
				if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] > 0 &&
						buffer->signal_buf[channel_][buffer->buf_pos_trig_] < 0)
				{
					setLow();
				}
				if (buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] < -2400 &&
						buffer->signal_buf[channel_][buffer->buf_pos_trig_] > -2400)
				{
					setHigh();
				}
				break;
			case RegularFalshes:
				if (LPT_is_high_){
					if (buffer->last_pkg_id - last_trigger_time_ > pulse_length_){
						setLow();
					}
				}
				else if (buffer->last_pkg_id - last_trigger_time_ > trigger_cooldown_ && buffer->last_pkg_id > trigger_start_delay_){
					setHigh();
				}
				break;
			default:
				Log("Chosen trigger type is not possible to use with LFP as a target");
				break;
			}

			buffer->buf_pos_trig_ ++;
		}
		break;

	case LPTTriggerTarget::LPTTargetSpikes:
		// check if inhibiting and has to stop
		if (LPT_is_high_){
			if (buffer->last_pkg_id - last_trigger_time_ > pulse_length_){
				setLow();
				buffer->Log("STOP INHIBITION at ", buffer->last_pkg_id);
			}

			buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
			return;
		}

		// TODO switch while and switch
		bool break_cycle_ = false;
		while(buffer->spike_buf_pos_lpt_ < spike_buf_limit_ptr_){

			if (break_cycle_)
				break;

			switch(trigger_type_){

			case LPTTriggerType::HighSynchronyTrigger:
				if (buffer->IsHighSynchrony(average_spikes_in_synchrony_tetrodes_) && buffer->last_pkg_id - last_trigger_time_ > trigger_cooldown_){
					timestamp_log_ << (int)round((last_trigger_time_) * 20 / 24.0) << "\n";
					timestamp_log_.flush();

					setHigh();
				}
				break;

			case LPTTriggerType::EnvironmentDominance:
				// TODO: extract synchrony detection to a separate processor

				// it is assumed that avilable prediction matrix in buffer is up to the (spike_buf_limit_ptr_ - 1)-th
				// 	spike inclusively

				// if in synchrony event, check
				// 		if dominance confidence is above threshold and make final decision if yes)
				//		if went beyond max window and make final decision if yes
				if (swr_ptr_ < buffer->swrs_.size()){
					// need more data before making decision
					// 	never the case if synchrony detection window is larger than min duration
					if (buffer->spike_buffer_[spike_buf_limit_ptr_ - 1]->pkg_id_ < buffer->swrs_[swr_ptr_][0] + sync_min_duration_){
						buffer->spike_buf_pos_lpt_ ++;
						continue;
					}

					// inhibition / allowance time out ?
					if (buffer->spike_buffer_[spike_buf_limit_ptr_ - 1]->pkg_id_ > buffer->swrs_[swr_ptr_][0] + sync_max_duration_){
						// have to decide now
						buffer->Log("TIMEOUT for classification decision is reached with a spike at ", buffer->spike_buffer_[spike_buf_limit_ptr_ - 1]->pkg_id_);

						// TODO configurable threshold
						if (environment_dominance_confidence_() > 0){
							buffer->Log("\t decision: INHIBIT, start at ", buffer->last_pkg_id);
							buffer->Log("\t confidence: ", environment_dominance_confidence_());
							events_inhibited_timeout_ ++;

							setHigh();
						} else {
							events_allowed_timeout_ ++;

							buffer->Log("\t decision: DON'T INHIBIT, start at ", buffer->last_pkg_id);
						}


						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
						swr_ptr_ ++;
						break_cycle_ = true;
					}

					double envdomconf = environment_dominance_confidence_();

					// evaluate environment dominance confidence and inhibit if higher that threshold
					if (envdomconf > dominance_confidence_threshold_){
						// DEBUG
						buffer->log_string_stream_ << "Environment dominance confidence = " << environment_dominance_confidence_() <<
								" is higher that the threshold = " << dominance_confidence_threshold_ << ", start INHIBITION at " << buffer->last_pkg_id << "\n";
						buffer->Log();
						events_inhibited_thold_ ++;

						// DEBUG
						std::string savepath = buffer->config_->getString("out.path.base") + std::string("trig/inh_") + Utils::Converter::int2str(events_inhibited_thold_) + ".mat";
						buffer->last_predictions_[0].save(savepath, arma::raw_ascii);

						setHigh();
						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
						buffer->buf_pos_trig_ = buffer->last_pkg_id;
						swr_ptr_ ++;
						continue;
					} else if (envdomconf < - dominance_confidence_threshold_){
						buffer->log_string_stream_ << "environment dominance confidence = " << environment_dominance_confidence_() <<
								" is lower that the -threshold = " << dominance_confidence_threshold_ << ", decide NO INHIBITION at " << buffer->last_pkg_id << "\n";
						buffer->Log();
						events_allowed_thold_ ++;

						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
						buffer->buf_pos_trig_ = buffer->last_pkg_id;
						swr_ptr_ ++;
					}
					// otherwise keep collecting data
				} else{ // not in synchrony (IDLE or INHIBITING)
					// otherwise check for synchrony
					if (!LPT_is_high_ && buffer->fr_estimated_ && buffer->IsHighSynchrony(average_spikes_in_synchrony_tetrodes_) && buffer->last_pkg_id - last_synchrony_ > trigger_cooldown_){
						timestamp_log_ << (int)round((last_synchrony_) * 20/24.0) << "\n";
						timestamp_log_.flush();

						// create new synchrony event in the buffer
						// TODO detect exactly with last processed spike ???
						unsigned int sync_start = buffer->last_pkg_id - buffer->POP_VEC_WIN_LEN * buffer->SAMPLING_RATE / 1000.0;

						Log("Synchrony detected at ",sync_start);
						buffer->swrs_.push_back(std::vector<unsigned int>());
						buffer->swrs_[buffer->swrs_.size() - 1].push_back(sync_start);
						// set SW PEAK and end
						buffer->swrs_[buffer->swrs_.size() - 1].push_back(sync_start + sync_max_duration_ / 2);
						buffer->swrs_[buffer->swrs_.size() - 1].push_back(sync_start + sync_max_duration_);

						last_synchrony_ = buffer->last_pkg_id;

						// prediction has to be evaluated for newly created event first before continuing
						break_cycle_ = true;
						// to predict ASAP: cause to review the spike
						buffer->spike_buf_pos_clusts_[0] --;
						break;
					}
				}

				// TODO !!! discretize to be spike-wise !!!
				// (more than 1 spike can be detected)
				buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
				break;
			default:
				Log("Chosen trigger type is not possible to use with spikes as a target");
				break;
			}
		}

		buffer->buf_pos_trig_ ++;
		break;
	}
}

double LPTTriggerProcessor::environment_dominance_confidence_() {
	const arma::fmat & pred = buffer->last_predictions_[0];
	arma::fmat pred1 = pred.submat(0, 0, pred.n_rows / 2 - 1, pred.n_cols - 1);
	arma::fmat pred2 = pred.submat(pred.n_rows / 2, 0 , pred.n_rows - 1, pred.n_cols - 1);
	double prob1 = pred1.max();
	double prob2 = pred2.max();

	return prob1 - prob2;
}


std::string LPTTriggerProcessor::name() {
	return "LPT Trigger";
}
