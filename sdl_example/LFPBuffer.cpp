/*
 * LFPBuffer.cpp
 *
 *  Created on: Sep 28, 2014
 *      Author: Igor Gridchyn
 */

#include "LFPBuffer.h"
#include "OnlineEstimator.cpp"

void LFPBuffer::Reset(Config* config) {
	if (config_)
		delete config_;

	config_ = config;

	spike_buf_pos = 0;
	spike_buf_nows_pos = 0;
	spike_buf_no_rec = 0;
	spike_buf_no_disp_pca = 0;
	spike_buf_pos_unproc_ = 0;
	spike_buf_pos_out = 0;
	spike_buf_pos_clust_ = 0;
	spike_buf_pos_draw_xy = 0;
	spike_buf_pos_speed_ = 0;
	spike_buf_pos_pop_vec_ = 0;
	spike_buf_pos_pf_ = 0;
	spike_buf_pos_auto_ = 0;
	spike_buf_pos_lpt_ = 0;
	spike_buf_pos_fet_writer_ = 0;
	spike_buf_pos_ws_disp_ = 0;

	if (tetr_info_)
		delete tetr_info_;
	tetr_info_ = new TetrodesInfo(config->getString("tetr.conf.path"));

	cluster_spike_counts_ = arma::mat(tetr_info_->tetrodes_number, 40, arma::fill::zeros);
	log_stream << "Buffer reset\n";
	log_stream << "INFO: # of tetrodes: " << tetr_info_->tetrodes_number << "\n";
	log_stream << "INFO: set memory...";

	for(int c=0; c < CHANNEL_NUM; ++c){
	        memset(signal_buf[c], 0, LFP_BUF_LEN * sizeof(signal_type));
	        memset(filtered_signal_buf[c], 0, LFP_BUF_LEN * sizeof(int));
	        memset(power_buf[c], 0, LFP_BUF_LEN * sizeof(int));
	        if (powerEstimatorsMap_[c] != nullptr){
	        	delete powerEstimatorsMap_[c];
	        	powerEstimatorsMap_[c] = nullptr;
	        }
	}

	log_stream << "done\n";

	log_stream << "INFO: Create online estimators...";
    powerEstimators_ = new OnlineEstimator<float, float>[tetr_info_->tetrodes_number];
    // TODO: configurableize
    speedEstimator_ = new OnlineEstimator<float, float>(16);
	log_stream << "done\n";

    memset(is_valid_channel_, 0, CHANNEL_NUM);
    tetr_info_->tetrode_by_channel = new int[CHANNEL_NUM];

    if (last_spike_pos_)
    	delete[] last_spike_pos_;
    last_spike_pos_ = new int[tetr_info_->tetrodes_number];

    // create a map of pointers to tetrode power estimators for each electrode
    for (int tetr = 0; tetr < tetr_info_->tetrodes_number; ++tetr ){
        for (int ci = 0; ci < tetr_info_->channels_numbers[tetr]; ++ci){
            powerEstimatorsMap_[tetr_info_->tetrode_channels[tetr][ci]] = powerEstimators_ + tetr;
            is_valid_channel_[tetr_info_->tetrode_channels[tetr][ci]] = true;

            tetr_info_->tetrode_by_channel[tetr_info_->tetrode_channels[tetr][ci]] = tetr;
        }
    }

    population_vector_window_.clear();
    population_vector_window_.resize(tetr_info_->tetrodes_number);
    // initialize for counting unclusterred spikes
    // clustering processors should resize this to use for clustered spikes clusters
    for (int t=0; t < tetr_info_->tetrodes_number; ++t){
    	population_vector_window_[t].push_back(0);
    }

	log_stream << "INFO: BUFFER CREATED\n";

	memset(spike_buffer_, 0, SPIKE_BUF_LEN * sizeof(Spike*));

	for (int pos_buf = 0; pos_buf < _POS_BUF_SIZE; ++pos_buf) {
		// TODO fix
		memset(positions_buf_[pos_buf], 0, 6 * sizeof(unsigned int));
	}

	ISIEstimators_ = new OnlineEstimator<float, float>*[tetr_info_->tetrodes_number];
	for (int t = 0; t < tetr_info_->tetrodes_number; ++t) {
		ISIEstimators_[t] = new OnlineEstimator<float, float>();
	}

	previous_spikes_pkg_ids_ = new unsigned int[tetr_info_->tetrodes_number];
	// fill with fake spikes to avoid unnnecessery checks because of the first time
	// (memory for 1 spike per tetrode will be lost)
	// TODO fix for package IDs starting not with nullptr
	// TODO warn packages strating not with 0
	for (int t = 0; t < tetr_info_->tetrodes_number; ++t) {
		previous_spikes_pkg_ids_[t] = 0;
	}

	is_high_synchrony_tetrode_ = new bool[tetr_info_->tetrodes_number];
	memset(is_high_synchrony_tetrode_, 0, sizeof(bool) * tetr_info_->tetrodes_number);
	for (int t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		is_high_synchrony_tetrode_[config_->synchrony_tetrodes_[t]] =  true;
	}

	// TODO check if all members are properly being reset
	high_synchrony_tetrode_spikes_ = 0;
}

LFPBuffer::~LFPBuffer(){
	Log("Buffer destructor called");

	for (int s = 0; s < SPIKE_BUF_LEN; ++s) {
		if (spike_buffer_[s] != nullptr)
			delete spike_buffer_[s];
	}
	delete[] spike_buffer_;

	delete[] CH_MAP;
	delete[] CH_MAP_INV;

	Log("Buffer destructor: delete signal / filtered signal / power buffers");

	for (int c = 0; c < CHANNEL_NUM; ++c){
		delete[] signal_buf[c];
		delete[] filtered_signal_buf[c];
		delete[] power_buf[c];
	}

	Log("Buffer destructor: delete tetrode info");
	if (tetr_info_)
			delete tetr_info_;

	Log("Buffer destructor: delete speed estimator");
	delete speedEstimator_;

	Log("Buffer destructor: delete last spike positions");
	 if (last_spike_pos_)
	    	delete[] last_spike_pos_;

	 Log("Buffer destructor: delete ISI estimators");
	 for (int t = 0; t < tetr_info_->tetrodes_number; ++t) {
		 delete ISIEstimators_[t];
	 }
	 delete[] ISIEstimators_;

	 delete[] previous_spikes_pkg_ids_;

	 Log("Buffer destructor: delete high synchrony tetrodes info");
	 delete[] is_high_synchrony_tetrode_;

	Log("Buffer destructor finished");
}

LFPBuffer::LFPBuffer(Config* config)
: POP_VEC_WIN_LEN(config->getInt("pop.vec.win.len.ms"))
, SAMPLING_RATE(config->getInt("sampling.rate"))
, pos_unknown_(config->getInt("pos.unknown", 1023))
, SPIKE_BUF_LEN(config->getInt("spike.buf.size", 1 << 24))
, SPIKE_BUF_HEAD_LEN(config->getInt("spike.buf.head", 1 << 18))
, LFP_BUF_LEN(config->getInt("buf.len", 1 << 11))
, BUF_HEAD_LEN(config->getInt("buf.head.len", 1 << 8))
, high_synchrony_factor_(config->getFloat("high.synchrony.factor", 2.0f))
{
	CH_MAP = new int[64]{8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55};

	// OLD
	//CH_MAP_INV = new int[64]{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 24, 25, 26, 27, 28, 29, 30, 31 };
	//CH_MAP_INV = new int[64]{48,49,50,51,52,53,54,55,32,33,34,35,36,37,38,39,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7,56,57,58,59,60,61,62,63,40,41,42,43,44,45,46,47,24,25,26,27,28,29,30,31,8,9,10,11,12,13,14,15};
	CH_MAP_INV = new int[64]{8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31,40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23,32,33,34,35,36,37,38,39,48,49,50,51,52,53,54,55};

	for (size_t i = 0; i < CHANNEL_NUM; i++)
	{
		powerEstimatorsMap_[i] = nullptr;
	}

	spike_buffer_ = new Spike*[SPIKE_BUF_LEN];

	// TODO !!! creatre normal random name

	srand(time(nullptr));
	int i = rand() % 64;
	std::string log_path_prefix = config->getString("log.path.prefix", "lfponline_LOG_");
	log_stream.open(log_path_prefix + Utils::NUMBERS[i] + ".txt", std::ios_base::app);
	std::cout << "Created LOG\n";

    for (int c = 0; c < CHANNEL_NUM; ++c){
    	unsigned int WS_SHIFT = 100;
    	signal_buf[c] = new signal_type[LFP_BUF_LEN + WS_SHIFT];
    	// + waveshape length
    	filtered_signal_buf[c] = new int[LFP_BUF_LEN + WS_SHIFT];
    	power_buf[c] = new int[LFP_BUF_LEN + WS_SHIFT];
    }

    Reset(config);
    user_context_.Init(tetr_info_->tetrodes_number);
}

void LFPBuffer::ResetPopulationWindow(){
	while (!population_vector_stack_.empty())
		population_vector_stack_.pop();

	for (int t = 0; t < tetr_info_->tetrodes_number; ++t){
		for (int c = 0; c < population_vector_window_[t].size(); ++c){
			population_vector_window_[t][c] = 0;
		}
	}
	population_vector_total_spikes_ = 0;
}

// pop spikes from the top of the queue who fall outside of population window of given length ending in last known PKG_ID
void LFPBuffer::RemoveSpikesOutsideWindow(const unsigned int& right_border){
    if (population_vector_stack_.empty()){
        return;
    }

    Spike *stop = population_vector_stack_.front();

    // TODO !!! prevent 0 spikes in the window !!!
    if (stop == nullptr){
    	return;
    }

    while (stop->pkg_id_ < right_border - POP_VEC_WIN_LEN * SAMPLING_RATE / 1000.f) {
    	// this duplicates population_vector_window_ + config->synchrony tetrodes, but is needed for efficiency ...
    	if (is_high_synchrony_tetrode_[stop->tetrode_])
    		high_synchrony_tetrode_spikes_ --;

        population_vector_stack_.pop();

		// TODO implemet smarter reset so that this check is not needed / warn if not true
		if (population_vector_window_[stop->tetrode_].size() > stop->cluster_id_ + 1 && population_vector_window_[stop->tetrode_][stop->cluster_id_ + 1] > 0)
		{
			population_vector_window_[stop->tetrode_][stop->cluster_id_ + 1] --;
			population_vector_total_spikes_--;
		}

        if (population_vector_stack_.empty()){
            break;
        }

        stop = population_vector_stack_.front();

        // TODO !!! see above
        if (stop == nullptr){
          	return;
        }
    }
}

void LFPBuffer::UpdateWindowVector(Spike *spike){
    // TODO: make right border of the window more precise: as close as possible to lst_pkg_id but not containing unclassified spikes
    // (left border = right border - POP_VEC_WIN_LEN)

	// is this check efficient ?
	if (population_vector_window_[spike->tetrode_].size() < spike->cluster_id_ + 2){
		population_vector_window_[spike->tetrode_].resize(spike->cluster_id_ + 2);
	}

    population_vector_window_[spike->tetrode_][spike->cluster_id_ + 1] ++;
    population_vector_total_spikes_ ++;

    population_vector_stack_.push(spike);

    // TODO separate processors for ISI estimation
    if (ISIEstimators_ != nullptr){
    	ISIEstimators_[spike->tetrode_]->push((spike->pkg_id_ - previous_spikes_pkg_ids_[spike->tetrode_]) / (float)SAMPLING_RATE);

    	// ??? do outside if prev_spikes used anywhere else
    	previous_spikes_pkg_ids_[spike->tetrode_] = spike->pkg_id_;
    }

    if (is_high_synchrony_tetrode_[spike->tetrode_])
    	high_synchrony_tetrode_spikes_ ++;

    // DEBUG - print pop vector occasionally
//    if (!(spike->pkg_id_ % 3000)){
//        std::cout << "Pop. vector: \n";
//        for (int t=0; t < tetr_info_->tetrodes_number; ++t) {
//            std::cout << "\t";
//            for (int c=0; c < population_vector_window_[t].size(); ++c) {
//                std::cout << population_vector_window_[t][c] << " ";
//            }
//            std::cout << "\n";
//        }
//    }
}


void LFPBuffer::AddSpike(Spike* spike) {
	spike_buffer_[spike_buf_pos] = spike;
	spike_buf_pos++;

	// check if rewind is requried
	if (spike_buf_pos == SPIKE_BUF_LEN - 1){
		memcpy(spike_buffer_, spike_buffer_ + spike_buf_pos - SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);
		//                    for (int del_spike = SPIKE_BUF_HEAD_LEN; del_spike < spike_buf_pos; ++del_spike) {
		//                    	delete spike_buffer_[del_spike];
		//                    	spike_buffer_[del_spike] = nullptr;
		//					}

		const int shift_new_start = spike_buf_pos - SPIKE_BUF_HEAD_LEN;

		spike_buf_no_rec -= std::min(shift_new_start, (int)spike_buf_no_rec);
		spike_buf_nows_pos -= std::min(shift_new_start, (int)spike_buf_nows_pos);
		spike_buf_pos_unproc_ -= std::min(shift_new_start, (int)spike_buf_pos_unproc_);
		spike_buf_no_disp_pca -= std::min(shift_new_start, (int)spike_buf_no_disp_pca );
		spike_buf_pos_out -= std::min(shift_new_start, (int)spike_buf_pos_out );
		spike_buf_pos_draw_xy -= std::min(shift_new_start, (int)spike_buf_pos_draw_xy );
		spike_buf_pos_speed_ -= std::min(shift_new_start, (int)spike_buf_pos_speed_);
		spike_buf_pos_pop_vec_ -= std::min(shift_new_start, (int)spike_buf_pos_pop_vec_);
		spike_buf_pos_clust_ -= std::min(shift_new_start, (int)spike_buf_pos_clust_);
		spike_buf_pos_pf_ -= std::min(shift_new_start, (int)spike_buf_pos_pf_);
		spike_buf_pos_auto_ -= std::min(shift_new_start, (int)spike_buf_pos_auto_);
		spike_buf_pos_lpt_ -= std::min(shift_new_start, (int)spike_buf_pos_lpt_);
		spike_buf_pos_fet_writer_ -= std::min(shift_new_start, (int)spike_buf_pos_fet_writer_);
		spike_buf_pos_ws_disp_ -= std::min(shift_new_start, (int)spike_buf_pos_ws_disp_);

		spike_buf_pos = SPIKE_BUF_HEAD_LEN;

		std::cout << "Spike buffer rewind (at pos " << buf_pos <<  ")!\n";
	}
}

double LFPBuffer::AverageSynchronySpikesWindow(){
	double average_spikes_window = .0f;

	for (int t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		average_spikes_window += 1.0 / ISIEstimators_[config_->synchrony_tetrodes_[t]]->get_mean_estimate();
	}
	average_spikes_window *= POP_VEC_WIN_LEN / 1000.0f;

	return average_spikes_window;
}

// whether current population window represents high synchrony activity
bool LFPBuffer::IsHighSynchrony() {
	return IsHighSynchrony(AverageSynchronySpikesWindow());
}

bool LFPBuffer::IsHighSynchrony(double average_spikes_window) {
	RemoveSpikesOutsideWindow(last_pkg_id);
	// whether have at least synchrony.factor X average spikes at all tetrodes
	return (high_synchrony_tetrode_spikes_ >= average_spikes_window * high_synchrony_factor_);
}

// TODO template
void LFPBuffer::Log(std::string message, int num) {
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	// TODO remove in release
	log_stream.flush();
}

void LFPBuffer::Log(std::string message, double num) {
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	// TODO remove in release
	log_stream.flush();
}

void LFPBuffer::Log(std::string message) {
	std::cout << message << "\n";
	log_stream << message << "\n";
	// TODO remove in release
	log_stream.flush();
}

const unsigned int& LFPBuffer::GetPosBufPointer(std::string name) {
	if (name == "pos"){
		return pos_buf_pos_;
	}
	else if (name == "disp"){
		return pos_buf_disp_pos_;
	}
	else if (name == "spike.pos"){
		return pos_buf_spike_pos_;
	}
	else if (name == "spike.speed"){
		return pos_buf_pos_spike_speed_;
	}
	else if (name == "speed.est"){
		return pos_buf_pos_speed_est;
	}
	else{
		Log("ERROR: Wrong name for position buffer pointer. Allowed names are: pos, disp, spike.pos, spike.speed, speed.est");
		exit(10);
	}
}

void LFPBuffer::ResetAC(const unsigned int& reset_tetrode,
		const int& reset_cluster) {

	spike_buf_pos_auto_ = 0;
	ac_reset_ = true;
	ac_reset_tetrode_ = reset_tetrode;
	ac_reset_cluster_ = reset_cluster;
}

void LFPBuffer::ResetAC(const unsigned int& reset_tetrode) {
	ResetAC(reset_tetrode, -1);
}
