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
	, sync_max_duration_(buffer->config_->getInt("lpt.trigger.sync.max.duration", 0))
	, spike_buf_limit_ptr_(buffer->spike_buf_pos_unproc_)
	, pulse_length_(buffer->config_->getInt("lpt.trigger.pulse.length"))
	, confidence_avg_(buffer->config_->getFloat("lpt.trigger.confidence.average", .0))
	, confidence_high_left_(buffer->config_->getFloat("lpt.trigger.confidence.high.left", -0.1f))
	, confidence_high_right_(buffer->config_->getFloat("lpt.trigger.confidence.high.right", 0.1f))
	, inhibit_nonconf_(buffer->config_->getBool("lpt.trigger.inhibit.nonconf", false))
	, swap_environments_(buffer->config_->getBool("lpt.swap.environments", false))
	, inhibition_map_path_(buffer->config_->getOutPath("lpt.trigger.inhibition.map", ""))
	, use_inhibition_map_(buffer->config_->getBool("lpt.trigger.use.map", false))
	, limit_trigger_area_(buffer->config_->getBool("lpt.trigger.limit.area", false))
	, trigger_at_path_(buffer->config_->getOutPath("lpt.trigger.limit.area.map", ""))
	, bin_size(buffer->config_->getFloat("bin.size", 4.0))
	, adjust_inhibition_rate_(buffer->config_->getBool("lpt.trigger.adjust.inhibition.rate", false))
{
	Log("Constructor start");

	std::string trigger_type_string = buffer->config_->getString("lpt.trigger.type", "regular");
	if (trigger_type_string == "dominance"){
		trigger_type_ = LPTTriggerType::EnvironmentDominance;
		trigger_target_ = LPTTriggerTarget::LPTTargetSpikes;
	} else if (trigger_type_string == "synchrony"){
		trigger_type_ = LPTTriggerType::HighSynchronyTrigger;
		trigger_target_ = LPTTriggerTarget::LPTTargetSpikes;
	} else if (trigger_type_string == "regular"){
		trigger_type_ = LPTTriggerType::RegularFalshes;
		trigger_target_ = LPTTriggerTarget::LPTTargetLFP;
	} else if (trigger_type_string == "threshold"){
		trigger_type_ = LPTTriggerType::DoubleThresholdCrossing;
		trigger_target_ = LPTTriggerTarget::LPTTargetLFP;
	} else if (trigger_type_string == "position"){
		trigger_type_ = LPTTriggerType::Position;
		trigger_target_ = LPTTriggerTarget::LPTTargetSpikes;
	} else {
		Log("Invalid trigger type. Allowed values are: dominance / synchrony / threshold / regular / position");
		exit(29875);
	}

	// this one should contain ${timestamp} in it to avoid overwriting old timestamps
	std::string ttpath = buffer->config_->getString("lpt.trigger.ttpath");
	Log("Write trigger timestamps to " + ttpath);
	timestamp_log_.open(ttpath);

	debug_log_.open(ttpath + ".debug");

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

	if (limit_trigger_area_){
		Utils::FS::CheckFileExistsWithError(trigger_at_path_, (Utils::Logger*)this);

		trigger_locations_.load(trigger_at_path_, arma::raw_ascii);
		std::stringstream ss;
		ss << "Loaded trigger limit area map, with size : " << trigger_locations_.n_rows << " X " << trigger_locations_.n_cols;
		ss << " Number of bins to be inhibited: " << arma::sum(trigger_locations_);
		Log(ss.str());
	}

	if (use_inhibition_map_){
		Utils::FS::CheckFileExistsWithError(inhibition_map_path_, (Utils::Logger*)this);

		inhibition_map_.load(inhibition_map_path_, arma::raw_ascii);

		std::stringstream ss;
		ss << "Loaded inhibition map, with size : " << inhibition_map_.n_rows << " X " << inhibition_map_.n_cols;
		ss << " Number of bins to be inhibited: " << arma::sum(inhibition_map_);
		Log(ss.str());
	}

	 // TODO !!! PARAMETRIZE
	inhibitionThresholdAdapter_ = new Utils::NewtonSolver(0.9, 24000*60*3, -120, confidence_avg_);

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
	if (LPT_is_high_ && buffer->last_pkg_id - last_trigger_time_ >= pulse_length_ && trigger_type_ != LPTTriggerType::Position){
		Log("STOP INHIBITION at ", buffer->last_pkg_id);
		setLow();

		Log("Statistics (ITH, ITO, ATH, ATO, TOT_I, TOT_A) :");
		buffer->log_string_stream_ << "\t" << events_inhibited_thold_ << " / " << events_inhibited_timeout_ << " / "
				<< events_allowed_thold_ << " / " << events_allowed_timeout_ << "; "
				<< (events_inhibited_thold_ + events_inhibited_timeout_) << " / " << (events_allowed_thold_ + events_allowed_timeout_) << "\n";
		buffer->Log();

		// adjust inhibition threshold
		if (adjust_inhibition_rate_ && trigger_type_ == LPTTriggerType::EnvironmentDominance && inhibitionThresholdAdapter_->NeedUpdate(buffer->last_pkg_id)){
			// calculate inhibition frequency in the last 1 minute
			unsigned int i=buffer->swrs_.size() - 1;
			unsigned int inhibited = inhibition_history_[i];
			while (i > 0 && buffer->swrs_[i-1][0] > inhibitionThresholdAdapter_->last_update_){
				i --;
				inhibited += inhibition_history_[i];
			}

			double frequency =  inhibited / double(buffer->swrs_.size() - i + 1);
			std::stringstream ss;
			ss << "Target function value: " << 0.9 << ", current value: " << frequency << ", current threshold: " << inhibitionThresholdAdapter_->Current_x() << ", ";
			confidence_avg_ = inhibitionThresholdAdapter_->Update(buffer->last_pkg_id, frequency);
			ss << " new threshold: " <<inhibitionThresholdAdapter_->Current_x();
			Log(ss.str());
		}
	}

	if (limit_trigger_area_){
		SpatialInfo lastSi = buffer->positions_buf_[buffer->pos_buf_pos_ - 1];
		if (!lastSi.valid)
			return;

		// TODO: + 1/2 ?
		int binx = lastSi.x_pos() / bin_size;
		int biny = lastSi.y_pos() / bin_size;

		// TODO
		if (trigger_locations_(biny, binx) < 0.5f)
			return;
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

	int thold = 30;

	switch(trigger_target_){
	case LPTTriggerTarget::LPTTargetLFP:
		while(buffer->buf_pos_trig_ < buffer->buf_pos){

			switch(trigger_type_){
			case DoubleThresholdCrossing:
				if ((buffer->signal_buf[channel_][buffer->buf_pos_trig_ - 1] + thold) > 0 &&
						(buffer->signal_buf[channel_][buffer->buf_pos_trig_] + thold) < 0 &&
						buffer->last_pkg_id - last_trigger_time_ > trigger_cooldown_)
				{
					setHigh();
					Log("High at ", buffer->last_pkg_id);
					timestamp_log_ << last_trigger_time_ << "\n";
					timestamp_log_.flush();
				}
				break;
			case RegularFalshes:
				if (LPT_is_high_){
					if (buffer->last_pkg_id - last_trigger_time_ >= pulse_length_){
						setLow();
					}
				}
				else if (buffer->last_pkg_id - last_trigger_time_ >= trigger_cooldown_ && buffer->last_pkg_id >= trigger_start_delay_){
					setHigh();
					timestamp_log_ << last_trigger_time_ << "\n";
					timestamp_log_.flush();
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
		if (LPT_is_high_ && trigger_type_ != LPTTriggerType::Position){
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
				if (buffer->IsHighSynchrony() && buffer->last_pkg_id - last_trigger_time_ >= trigger_cooldown_){
					setHigh();
					timestamp_log_ << (int)round((last_trigger_time_)) << "\n";
					timestamp_log_.flush();

					// TODO: extract (also available below)
					unsigned int sync_start = buffer->last_pkg_id - (unsigned int)(buffer->POP_VEC_WIN_LEN * buffer->SAMPLING_RATE / 1000.0);
					Log("Synchrony detected at (window start) ",sync_start);
					buffer->swrs_.push_back(std::vector<unsigned int>());
					buffer->swrs_[buffer->swrs_.size() - 1].push_back(sync_start);
					// set SW PEAK and end
					buffer->swrs_[buffer->swrs_.size() - 1].push_back(buffer->last_pkg_id);
					buffer->swrs_[buffer->swrs_.size() - 1].push_back(std::max(sync_start + sync_max_duration_, buffer->last_pkg_id));
				}
				buffer->spike_buf_pos_lpt_ ++;
				break;

			case LPTTriggerType::EnvironmentDominance:
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
					if (buffer->spike_buffer_[spike_buf_limit_ptr_ - 1]->pkg_id_ >= buffer->swrs_[swr_ptr_][0] + sync_max_duration_){
						// have to decide now
						buffer->Log("TIMEOUT for classification decision is reached with a spike at ", buffer->spike_buffer_[spike_buf_limit_ptr_ - 1]->pkg_id_);

						double env_dom_conf_ = environment_dominance_confidence_();

						if (env_dom_conf_ > confidence_avg_ || inhibit_nonconf_){
							buffer->Log("\t decision: INHIBIT, start at ", buffer->last_pkg_id);
							buffer->Log("\t confidence: ", env_dom_conf_);
							events_inhibited_timeout_ ++;

							setHigh();

							// DEBUG
							buffer->CheckPkgIdAndReportTime(buffer->spike_buffer_[spike_buf_limit_ptr_ - 1]->pkg_id_, "INHIBITION started\n");

//							timestamp_log_ << buffer->last_pkg_id << "\n";
//							timestamp_log_.flush();

							inhibition_history_.push_back(1);

						} else {
							events_allowed_timeout_ ++;

							buffer->Log("\t decision: DON'T INHIBIT, start at ", buffer->last_pkg_id);
							inhibition_history_.push_back(0);
						}

						debug_log_ << env_dom_conf_ << "\n";

						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
						swr_ptr_ ++;
						break_cycle_ = true;
					}

					double envdomconf = environment_dominance_confidence_();

					// evaluate environment dominance confidence and inhibit if higher that threshold
					if (envdomconf > confidence_high_right_){
						// DEBUG
						buffer->log_string_stream_ << "Environment dominance confidence = " << environment_dominance_confidence_() <<
								" is higher that the threshold = " << confidence_high_right_ << ", start INHIBITION at " << buffer->last_pkg_id << "\n";
						buffer->Log();
						events_inhibited_thold_ ++;

						// DEBUG
//						std::string savepath = buffer->config_->getString("out.path.base") + std::string("trig/inh_") + Utils::Converter::int2str(events_inhibited_thold_) + ".mat";
//						buffer->last_predictions_[0].save(savepath, arma::raw_ascii);

						setHigh();
						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
						buffer->buf_pos_trig_ = buffer->last_pkg_id;
						swr_ptr_ ++;

						inhibition_history_.push_back(1);

						//timestamp_log_ << buffer->last_pkg_id << "\n";
						//timestamp_log_.flush();

						continue;
					} else if (envdomconf < confidence_high_left_){
						buffer->log_string_stream_ << "environment dominance confidence = " << environment_dominance_confidence_() <<
								" is lower that the -threshold = " << confidence_high_left_ << ", decide NO INHIBITION at " << buffer->last_pkg_id << "\n";
						buffer->Log();
						events_allowed_thold_ ++;

						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
						buffer->buf_pos_trig_ = buffer->last_pkg_id;
						swr_ptr_ ++;

						inhibition_history_.push_back(0);
					}
					// otherwise keep collecting data
				} else{ // not in synchrony (IDLE or INHIBITING)
					// otherwise check for synchrony
					if (!LPT_is_high_ && buffer->IsHighSynchrony() && buffer->last_pkg_id - last_synchrony_ >= trigger_cooldown_){
						timestamp_log_ << (int)round((last_synchrony_)) << "\n";
						timestamp_log_.flush();

						// create new synchrony event in the buffer
						// TODO detect exactly with last processed spike ???
						unsigned int sync_start = buffer->last_pkg_id - (unsigned int)(buffer->POP_VEC_WIN_LEN * buffer->SAMPLING_RATE / 1000.0);

						Log("Synchrony detected at (window start) ",sync_start);
						buffer->swrs_.push_back(std::vector<unsigned int>());
						buffer->swrs_[buffer->swrs_.size() - 1].push_back(sync_start);
						// set SW PEAK and end
						buffer->swrs_[buffer->swrs_.size() - 1].push_back(buffer->last_pkg_id);
						buffer->swrs_[buffer->swrs_.size() - 1].push_back(std::max(sync_start + sync_max_duration_, buffer->last_pkg_id));

						last_synchrony_ = buffer->last_pkg_id;

						buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;

						// prediction has to be evaluated for newly created event first before continuing
						break_cycle_ = true;
						// to predict ASAP: cause to review the spike
//						buffer->spike_buf_pos_clusts_[0] --;
						break;
					}
				}

				// TODO !!! discretize to be spike-wise !!!
				// (more than 1 spike can be detected)
				buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
				break;

			case LPTTriggerType::Position: {
					// consider last position only
					SpatialInfo & si = buffer->positions_buf_[buffer->pos_buf_pos_ - 1];

					if (use_inhibition_map_ && !Utils::Math::Isnan(si.x_pos())){
						double bin_size = 4.0;
						unsigned int binx = si.x_pos() / bin_size;
						unsigned int biny = si.y_pos() / bin_size;

						if (binx < inhibition_map_.n_cols && biny < inhibition_map_.n_rows){
							if(inhibition_map_(biny, binx) > 0){
								if (!LPT_is_high_){
									Log("Enter inhibition zone at ", buffer->last_pkg_id);
								}
								setHigh();
							} else {
								if (LPT_is_high_){
									Log("Leave inhibition zone at ", buffer->last_pkg_id);
								}
								setLow();
							}
						}
					}

					buffer->spike_buf_pos_lpt_ = spike_buf_limit_ptr_;
					break;
			}

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

	if (use_inhibition_map_){
		unsigned int maxy, maxx;
		double maxprob = pred.max(maxy, maxx);
		Log("Inh maxx: ", maxx);
		Log("Inh maxy: ", maxy);
		return (inhibition_map_(maxx, maxy) - 0.5) * maxprob;
	}

	// separation by X
	arma::fmat pred1 = pred.submat(0, 0, pred.n_rows / 2 - 1, pred.n_cols - 1);
	arma::fmat pred2 = pred.submat(pred.n_rows / 2, 0 , pred.n_rows - 1, pred.n_cols - 1);

	// separation by Y
	/*arma::fmat pred1 = pred.submat(0, 0, pred.n_rows - 1, 22 - 1);
	arma::fmat pred2 = pred.submat(0, 22, pred.n_rows - 1, pred.n_cols - 1);*/

	// TODO: sum ?
	double prob1 = pred1.max();
	double prob2 = pred2.max();

	return swap_environments_ ? prob2 - prob1 : prob1 - prob2;
}


std::string LPTTriggerProcessor::name() {
	return "LPT Trigger";
}
