/*
 * LFPBuffer.cpp
 *
 *  Created on: Sep 28, 2014
 *      Author: Igor Gridchyn
 */

#include "LFPBuffer.h"
#include "OnlineEstimator.cpp"
#include "time.h"

#ifdef _WIN32
	#define DATEFORMAT "%Y-%m-%d_%H-%M-%S"
#else
	#define DATEFORMAT "%F_%T"
#endif

void LFPBuffer::Reset(Config* config)
{
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
	spike_buf_pos_featext_collected_ = 0;
	spike_buf_pos_binary_classifier_ = 0;
	spike_buf_pos_predetect_ = 0;

	// main tetrode info
	if (tetr_info_)
		delete tetr_info_;
	tetr_info_ = new TetrodesInfo(config->getString("tetr.conf.path"));

	if (tetr_info_->tetrodes_number() != config_->tetrodes.size() &&  config_->tetrodes.size() > 0){
		Log("ERROR: Tetrodes list in main config does not correspond to the information in tetrodes config.");
		exit(LFPONLINE_BAD_TETRODES_CONFIG);
	}

	if (tetr_info_->status_ == TI_STATUS_BAD_TETRODES_NUMBER){
		Log("ERROR: Bad tetrodes nuber, exiting...");
		exit(LFPONLINE_BAD_TETRODES_CONFIG);
	}

	// set dimensionalities for each number of channels per tetrode
	std::vector<int> pc_per_chan;
	pc_per_chan.push_back(0);
	const unsigned int npc4 = config->getInt("pca.num.pc", 3);
	for (int nc = 1; nc < 5; ++ nc){
		pc_per_chan.push_back(npc4); // - nc + 4);
	}

	Log("Number of PCs per channel for a group of 4 channels: ", pc_per_chan[4]);
	Log("Number of PCs per channel for a group of 3 channels: ", pc_per_chan[3]);
	Log("Number of PCs per channel for a group of 2 channels: ", pc_per_chan[2]);

	for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
		int nchan = tetr_info_->channels_number(t);
		int npc = pc_per_chan[nchan];
		feature_space_dims_.push_back(nchan * npc);
	}

	// load alternative TetrodeInfos (used by some processors)
	bool tiexists = true;
	std::string tipath = config->getString("tetr.conf.path.0", config->getString("tetr.conf.path"));
	int ticount = 1;
	while (tiexists){
		TetrodesInfo *new_ti = new TetrodesInfo(tipath);
		if (new_ti -> status_ == TI_STATUS_BAD_TETRODES_NUMBER){
			Log("Bad tetrodes nuber, exiting...");
			exit(LFPONLINE_BAD_TETRODES_CONFIG);
		}
		alt_tetr_infos_.push_back(new_ti);
		tipath = config_->getString(std::string("tetr.conf.path.") + Utils::NUMBERS[ticount], "");

		if (tipath.length() == 0){
			tiexists = false;
		}

		ticount ++;
	}
	std::stringstream ss;
	ss << "Loaded " << ticount-1 << " alternative tetrode configurations...";
	Log(ss.str());

	cluster_spike_counts_ = arma::fmat(tetr_info_->tetrodes_number(), 40, arma::fill::zeros);
	log_stream << "Buffer reset\n";
	log_stream << "INFO: # of tetrodes: " << tetr_info_->tetrodes_number() << "\n";
	log_stream << "INFO: set memory...";
	log_stream << "done\n";

	log_stream << "INFO: Create online estimators...";
	for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
		powerEstimators_.push_back(OnlineEstimator<float, float>(config->getInt("spike.detection.min.power.samples", 500000)));
	}

    speedEstimator_ = new OnlineEstimator<float, float>(SPEED_ESTIMATOR_WINDOW_);
	log_stream << "done\n";

    tetr_info_->tetrode_by_channel = new unsigned int[CHANNEL_NUM];

    if (last_spike_pos_)
    	delete[] last_spike_pos_;
    last_spike_pos_ = new int[tetr_info_->tetrodes_number()];

    // create a map of pointers to tetrode power estimators for each electrode
    for (size_t tetr = 0; tetr < tetr_info_->tetrodes_number(); ++tetr ){
        for (size_t ci = 0; ci < tetr_info_->channels_number(tetr); ++ci){
            powerEstimatorsMap_[tetr_info_->tetrode_channels[tetr][ci]] = &(powerEstimators_[0]) + tetr;
            is_valid_channel_[tetr_info_->tetrode_channels[tetr][ci]] = true;

            tetr_info_->tetrode_by_channel[tetr_info_->tetrode_channels[tetr][ci]] = tetr;
        }
    }

    for (size_t c = 0; c < CHANNEL_NUM; ++c){
    	if (signal_buf[c])
    		delete signal_buf[c];

    	if (filtered_signal_buf[c])
    		delete filtered_signal_buf[c];

    	if (power_buf[c])
    		delete power_buf[c];

    	if (is_valid_channel_[c]){
    		signal_buf[c] = new signal_type[LFP_BUF_LEN + WS_SHIFT];
    		filtered_signal_buf[c] = new ws_type[LFP_BUF_LEN + WS_SHIFT];
    		power_buf[c] = new int[LFP_BUF_LEN + WS_SHIFT];

	        memset(signal_buf[c], 0, (LFP_BUF_LEN + WS_SHIFT) * sizeof(signal_type));
	        memset(filtered_signal_buf[c], 0, LFP_BUF_LEN * sizeof(ws_type));
	        memset(power_buf[c], 0, LFP_BUF_LEN * sizeof(int));
    	}
    }

    population_vector_window_.clear();
    population_vector_window_.resize(tetr_info_->tetrodes_number());
    // initialize for counting unclusterred spikes
    // clustering processors should resize this to use for clustered spikes clusters
    for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
    	population_vector_window_[t].push_back(0);
    }

	log_stream << "INFO: BUFFER CREATED\n";

	// ??? rather init all spikes ???
	// memset(spike_buffer_, 0, SPIKE_BUF_LEN * sizeof(Spike*));

	ISIEstimators_ = new OnlineEstimator<float, float>*[tetr_info_->tetrodes_number()];
	for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
		ISIEstimators_[t] = new OnlineEstimator<float, float>();
	}

	previous_spikes_pkg_ids_ = new unsigned int[tetr_info_->tetrodes_number()];
	// fill with fake spikes to avoid unnnecessery checks because of the first time
	// (memory for 1 spike per tetrode will be lost)
	// TODO fix for package IDs starting not with nullptr
	// TODO warn packages strating not with 0
	for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
		previous_spikes_pkg_ids_[t] = 0;
	}

	is_high_synchrony_tetrode_ = new bool[tetr_info_->tetrodes_number()];
	memset(is_high_synchrony_tetrode_, 0, sizeof(bool) * tetr_info_->tetrodes_number());
	for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		is_high_synchrony_tetrode_[config_->synchrony_tetrodes_[t]] =  true;
	}

	high_synchrony_tetrode_spikes_ = 0;

	user_context_.Init(tetr_info_->tetrodes_number());
}

LFPBuffer::~LFPBuffer(){
	Log("Buffer destructor called");

	for (size_t s = 0; s < SPIKE_BUF_LEN; ++s) {
		// DEBUG
		// TODO remove
		Log("Delete spike #", s);
		if (spike_buffer_[s] != nullptr)
			delete spike_buffer_[s];
	}
	delete[] spike_buffer_;

	Log("Buffer destructor: delete signal / filtered signal / power buffers");

	for (size_t c = 0; c < CHANNEL_NUM; ++c){
		delete[] signal_buf[c];
		delete[] filtered_signal_buf[c];
		delete[] power_buf[c];
	}

	Log("Buffer destructor: delete speed estimator");
	delete speedEstimator_;

	Log("Buffer destructor: delete last spike positions");
	 if (last_spike_pos_)
	    	delete[] last_spike_pos_;

	 Log("Buffer destructor: delete ISI estimators");
	 for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
		 delete ISIEstimators_[t];
	 }
	 delete[] ISIEstimators_;

	 delete[] previous_spikes_pkg_ids_;

	 Log("Buffer destructor: delete high synchrony tetrodes info");
	 delete[] is_high_synchrony_tetrode_;

	Log("Buffer destructor: delete tetrode info");
	if (tetr_info_)
		delete tetr_info_;

	Log("Buffer destructor finished");
}

LFPBuffer::LFPBuffer(Config* config)
: CHANNEL_NUM(config->getInt("channel.num", 64))
, POP_VEC_WIN_LEN(config->getInt("pop.vec.win.len.ms"))
, SAMPLING_RATE(config->getInt("sampling.rate"))
, pos_unknown_(config->getInt("pos.unknown", 1023))
, SPIKE_BUF_LEN(config->getInt("spike.buf.size", 1 << 24))
, SPIKE_BUF_HEAD_LEN(config->getInt("spike.buf.head", 1 << 18))
, LFP_BUF_LEN(config->getInt("buf.len", 1 << 11))
, BUF_HEAD_LEN(config->getInt("buf.head.len", 1 << 8))
, high_synchrony_factor_(config->getFloat("high.synchrony.factor", 2.0f))
, POS_BUF_LEN(config->getInt("pos.buf.len", 1000000))
, target_pkg_id_(config->getInt("debug.target.pkg", 0))
, target_buf_pos_(config->getInt("debug.target.bufpos", 0))
, SPEED_ESTIMATOR_WINDOW_(config->getInt("speed.est.window", 16))
, spike_waveshape_pool_size_(config->getInt("waveshape.pool.size", SPIKE_BUF_LEN))
, FR_ESTIMATE_DELAY(config->getInt("kd.frest.delay"))
{
	buf_pos = BUF_HEAD_LEN;
	buf_pos_trig_ = BUF_HEAD_LEN;

	signal_buf = new signal_type*[CHANNEL_NUM];
	memset(signal_buf, 0, sizeof(signal_type*) * CHANNEL_NUM);
	filtered_signal_buf.resize(CHANNEL_NUM);
	power_buf.resize(CHANNEL_NUM);
	is_valid_channel_ = new bool[CHANNEL_NUM];
	powerEstimatorsMap_.resize(CHANNEL_NUM);

	for (size_t i = 0; i < CHANNEL_NUM; i++)
	{
		powerEstimatorsMap_[i] = nullptr;
		is_valid_channel_[i] = false;
	}

	spike_buffer_ = new Spike*[SPIKE_BUF_LEN];
	tmp_spike_buf_ = new Spike*[SPIKE_BUF_HEAD_LEN];

	std::string log_path_prefix = config->getString("log.path.prefix", "lfponline_LOG_");

	// OLD LOG NAME SOLUTION
//	srand(time(nullptr));
//	int i = rand() % 64;
//	log_stream.open(log_path_prefix + Utils::NUMBERS[i] + ".txt", std::ios_base::app);

	// generate temporary file with current date _ time
	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer, 80, DATEFORMAT, timeinfo);
	buffer[13] = '-';
	buffer[16] = '-';
	std::string logpath = config->getString("out.path.base") + log_path_prefix + buffer + ".txt";
	log_stream.open(logpath, std::ios_base::app);

	Log("Created LOG");

    Reset(config);

    spike_buf_pos_clusts_.resize(100);
    last_preidction_window_ends_.resize(100);

    positions_buf_ = new SpatialInfo[POS_BUF_LEN];
    POS_BUF_HEAD_LEN = POS_BUF_LEN / 10;

    // allocate memory for waveshapes
    // TODO : allocate according to channels number tetrode-wise
    spikes_ws_pool_ = new PseudoMultidimensionalArrayPool<ws_type>(4, 128, spike_waveshape_pool_size_);
    spikes_ws_final_pool_ = new PseudoMultidimensionalArrayPool<int>(4, 16, SPIKE_BUF_LEN);
    // TODO !!!! use maximum dimension
    // TODO !!!!!! pools with dynamic size for each unmber of features
    spike_features_pool_ = new LinearArrayPool<float>(8, SPIKE_BUF_LEN);
    spike_extra_features_ptr_pool_ = new LinearArrayPool<float *>(4, SPIKE_BUF_LEN);

    spike_pool_ = new Spike[SPIKE_BUF_LEN];
    for (unsigned int s=0; s < SPIKE_BUF_LEN; ++s){
    	spike_buffer_[s] = spike_pool_ + s;
//    	AllocateExtraFeaturePointerMemory(spike_pool_ + s);
    }

    //DEBUG
    head_start_ = spike_buffer_[0];
    tail_start_ = spike_buffer_[SPIKE_BUF_LEN - SPIKE_BUF_HEAD_LEN];

    chunk_buf_ = new unsigned char[chunk_buf_len_];
    chunk_buf_ptr_in_ = 0;

    debug_stream_.open("debug.txt");
}

template <class T>
LinearArrayPool<T>::LinearArrayPool(unsigned int dim, unsigned int pool_size)
	: QueueInterface<T*>(pool_size)
	, dim_(dim)
	, pool_size_(pool_size) {
	array_ = new T [dim * pool_size];

	for (unsigned int s=0; s < pool_size_; ++s){
		this->MemoryFreed(array_ + dim * s);
	}
}

template<class T>
PseudoMultidimensionalArrayPool<T>::PseudoMultidimensionalArrayPool(unsigned int dim1, unsigned int dim2, unsigned int pool_size)
	: QueueInterface<T**>(pool_size)
	, dim1_(dim1)
	, dim2_(dim2)
	, pool_size_(pool_size)

{
	array_ = new T [dim2_ * dim1_ * pool_size_];
	array_rows_ = new T*[dim1_ * pool_size_];
    for (unsigned int s=0; s < dim1_ * pool_size_; ++s){
    	array_rows_[s] = array_ + s * dim2_;
    }

    for (unsigned int s=0; s < pool_size; ++s){
    	this->MemoryFreed(array_rows_ + dim1_ * s);
    }
}

void LFPBuffer::ResetPopulationWindow(){
	while (!population_vector_stack_.empty())
		population_vector_stack_.pop();

	for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t){
		for (size_t c = 0; c < population_vector_window_[t].size(); ++c){
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

    if (stop == nullptr){
    	return;
    }

    while (stop->pkg_id_ < right_border - POP_VEC_WIN_LEN * SAMPLING_RATE / 1000.f) {
    	// this duplicates population_vector_window_ + config->synchrony tetrodes, but is needed for efficiency ...
    	if (is_high_synchrony_tetrode_[stop->tetrode_])
    		high_synchrony_tetrode_spikes_ --;

        population_vector_stack_.pop();

		// TODO implemet smarter reset so that this check is not needed / warn if not true
		if (population_vector_window_[stop->tetrode_].size() > (unsigned int)(stop->cluster_id_ + 1) && population_vector_window_[stop->tetrode_][stop->cluster_id_ + 1] > 0)
		{
			population_vector_window_[stop->tetrode_][stop->cluster_id_ + 1] --;
			population_vector_total_spikes_--;
		}

        if (population_vector_stack_.empty()){
            break;
        }

        stop = population_vector_stack_.front();

        if (stop == nullptr){
          	return;
        }
    }
}

void LFPBuffer::UpdateWindowVector(Spike *spike){
    // TODO: make right border of the window more precise: as close as possible to lst_pkg_id but not containing unclassified spikes
    // (left border = right border - POP_VEC_WIN_LEN)

	// is this check efficient ?
	if (population_vector_window_[spike->tetrode_].size() < (unsigned int)(spike->cluster_id_ + 2)){
		population_vector_window_[spike->tetrode_].resize(spike->cluster_id_ + 2);
	}

    population_vector_window_[spike->tetrode_][spike->cluster_id_ + 1] ++;
    population_vector_total_spikes_ ++;

    population_vector_stack_.push(spike);

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
//        for (int t=0; t < tetr_info_->tetrodes_number(); ++t) {
//            std::cout << "\t";
//            for (int c=0; c < population_vector_window_[t].size(); ++c) {
//                std::cout << population_vector_window_[t][c] << " ";
//            }
//            std::cout << "\n";
//        }
//    }
}


void LFPBuffer::AddSpike(Spike* spike, bool rewind) {
	// the spike is re-initialized rather than deleted and created again
//	if (spike_buffer_[spike_buf_pos] != nullptr){
//		FreeWaveshapeMemory(spike_buffer_[spike_buf_pos]);
//		FreeFinalWaveshapeMemory(spike_buffer_[spike_buf_pos]);
//		FreeFeaturesMemory(spike_buffer_[spike_buf_pos]);
//		FreeExtraFeaturePointerMemory(spike_buffer_[spike_buf_pos]);
//		delete spike_buffer_[spike_buf_pos];
//		spike_buffer_[spike_buf_pos] = nullptr;
//	}

	// the re-initialized spike is already there
	// spike_buffer_[spike_buf_pos] = spike;
	spike_buf_pos++;

	// check if rewind is requried
	if (spike_buf_pos == SPIKE_BUF_LEN && rewind){
		Rewind();
	}

	estimate_firing_rates();
}

double LFPBuffer::AverageSynchronySpikesWindow(){
	double average_spikes_window = .0f;

	for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
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

	if (fr_estimates_.empty())
		return false;

	// TETRODE-WISE increase + # of synchronous tetrodes
//	int nhigh = 0;
//	for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t){
//		if (population_vector_window_[config_->synchrony_tetrodes_[t]][0] > POP_VEC_WIN_LEN * fr_estimates_[config_->synchrony_tetrodes_[t]] / 1000.0 * high_synchrony_factor_){
//			nhigh ++;
//		}
//	}
//	return nhigh > 6;

	// TODO !!! cache
	double sync_spikes_window_ = .0;
	for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		sync_spikes_window_ += fr_estimates_[config_->synchrony_tetrodes_[t]] * POP_VEC_WIN_LEN / 1000.0;
	}

	// whether have at least synchrony.factor X average spikes at all tetrodes
	return (high_synchrony_tetrode_spikes_ >= sync_spikes_window_ * high_synchrony_factor_);
}


void LFPBuffer::Log() {
	std::cout << log_string_stream_.str();
	log_stream << log_string_stream_.str();
	// TODO !!!!!! don't fllush in RELEASE (all overloads)
	log_stream.flush();
	log_string_stream_.str(std::string());
}

void LFPBuffer::Log(std::string message, unsigned int num){
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	log_stream.flush();
}

void LFPBuffer::Log(std::string message, int num) {
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	log_stream.flush();
}

# ifndef _WIN32
void LFPBuffer::Log(std::string message, size_t num) {
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	log_stream.flush();
}
#endif

void LFPBuffer::Log(std::string message, double num) {
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	log_stream.flush();
}

void LFPBuffer::Log(std::string message) {
	std::cout << message << "\n";
	log_stream << message << "\n";
	log_stream.flush();
}

const unsigned int& LFPBuffer::GetSpikeBufPointer(std::string name) {
	if (name == "speed"){
		return spike_buf_pos_speed_;
	}else if (name == "pca"){
		return spike_buf_pos_unproc_;
	}else{
		Log("ERROR: Wrong or not supported name for position buffer pointer. Allowed names are: speed, pca");
		exit(LFPONLINE_ERROR_UNKNOWN_POS_BUFFER_POINTER);
	}
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
		exit(LFPONLINE_ERROR_UNKNOWN_POS_BUFFER_POINTER);
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

bool LFPBuffer::CheckPkgIdAndReportTime(const unsigned int& pkg_id,
		const std::string msg, bool set_checkpoint) {
	if (pkg_id == target_pkg_id_){
		log_string_stream_ << (clock() - checkpoint_) * 1000000 / CLOCKS_PER_SEC << " us " << msg;
		if (true){
			Log();
		}
		if (set_checkpoint){
			checkpoint_ = clock();
		}
		return true;
	}
	return false;
}

bool LFPBuffer::CheckPkgIdAndReportTime(const unsigned int pkg_id1,
		const unsigned int pkg_id2, const std::string msg,
		bool set_checkpoint) {
	if (pkg_id1 <= target_pkg_id_&& pkg_id2 >= target_pkg_id_){
		if (!set_checkpoint){
			log_string_stream_ << (clock() - checkpoint_) * 1000000 / CLOCKS_PER_SEC << " us " << msg;
		}else
		{
			log_string_stream_ << " - us " << msg;
		}
		if (true){
			Log();
		}
		if (set_checkpoint){
			checkpoint_ = clock();
		}
		return true;
	}
	return false;
}

void LFPBuffer::CheckBufPosAndReportTime(const unsigned int& buf_pos,
		const std::string msg) {
	if (buf_pos == target_buf_pos_){
		log_string_stream_ << (clock() - checkpoint_) * 1000000 / CLOCKS_PER_SEC << " us " << msg;
		Log();
	}
}

float AverageLEDs(const float & smallLED, const float & bigLED, const bool & valid){
//	if (!valid)
//		return -1.0;
//
//	if (smallLED < 0){
//		return bigLED;
//	}
//	else if (bigLED < 0){
//		return smallLED;
//	}
//	else{
		return (smallLED + bigLED) / 2.0;
//	}
}

float SpatialInfo::x_pos() {
	return AverageLEDs(x_small_LED_, x_big_LED_, valid);
}

float SpatialInfo::y_pos() {
	return AverageLEDs(y_small_LED_, y_big_LED_, valid);
}

// TODO extract code
void LFPBuffer::AllocateWaveshapeMemory(Spike *spike) {
	AllocatePoolMemory<ws_type*>(&spike->waveshape, spikes_ws_pool_);
}

void LFPBuffer::FreeWaveshapeMemory(Spike* spike) {
	FreetPoolMemory<ws_type*>(&spike->waveshape, spikes_ws_pool_);
}

void LFPBuffer::AllocateFinalWaveshapeMemory(Spike* spike) {
	AllocatePoolMemory<int*>(&spike->waveshape_final, spikes_ws_final_pool_);
}

void LFPBuffer::FreeFinalWaveshapeMemory(Spike* spike) {
	FreetPoolMemory<int*>(&spike->waveshape_final, spikes_ws_final_pool_);
}

void LFPBuffer::AllocateFeaturesMemory(Spike* spike) {
	AllocatePoolMemory<float>(&spike->pc, spike_features_pool_);
}

void LFPBuffer::FreeFeaturesMemory(Spike* spike) {
	FreetPoolMemory<float>(&spike->pc, spike_features_pool_);
}

void LFPBuffer::AllocateExtraFeaturePointerMemory(Spike* spike) {
	AllocatePoolMemory<float*>(&spike->extra_features_, spike_extra_features_ptr_pool_);
	spike->assignExtraFeaturePointers();
}

void LFPBuffer::FreeExtraFeaturePointerMemory(Spike* spike) {
	FreetPoolMemory<float*>(&spike->extra_features_, spike_extra_features_ptr_pool_);
}

void LFPBuffer::Rewind() {
	// DEBUG
	if ((head_start_ != spike_buffer_[0] && head_start_ != spike_buffer_[SPIKE_BUF_LEN - SPIKE_BUF_HEAD_LEN]) ||
			(tail_start_ != spike_buffer_[0] && tail_start_ != spike_buffer_[SPIKE_BUF_LEN - SPIKE_BUF_HEAD_LEN]) ){
		if (head_start_ != spike_buffer_[0]){
			Log("Buffer abused before rewind - HEAD!");
			if (spike_buffer_[0] == (Spike*)0xb74edb){
				Log("0xb74edb");
			}
		}
		else{
			Log("Buffer abused before rewind - TAIL!");
		}
	}

	// exchange head and tail1
	memcpy(tmp_spike_buf_, spike_buffer_ + spike_buf_pos - SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);
	// to reuse the same objects that are in the head
	memcpy(spike_buffer_ + spike_buf_pos - SPIKE_BUF_HEAD_LEN, spike_buffer_, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);
	memcpy(spike_buffer_, tmp_spike_buf_, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);

	// DEBUG
	if ((head_start_ != spike_buffer_[0] && head_start_ != spike_buffer_[SPIKE_BUF_LEN - SPIKE_BUF_HEAD_LEN]) ||
			(tail_start_ != spike_buffer_[0] && tail_start_ != spike_buffer_[SPIKE_BUF_LEN - SPIKE_BUF_HEAD_LEN]) ){
		if (head_start_ != spike_buffer_[0]){
			Log("Buffer abused after rewind - HEAD!");
		}
		else{
			Log("Buffer abused after rewind - TAIL!");
		}
	}

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
	spike_buf_pos_featext_collected_ -= std::min(shift_new_start, (int)spike_buf_pos_featext_collected_);
	spike_buf_pos_binary_classifier_ -= std::min(shift_new_start, (int)spike_buf_pos_binary_classifier_);
	spike_buf_pos_predetect_ -= std::min(shift_new_start, (int)spike_buf_pos_predetect_);

	for (size_t i=0; i < spike_buf_pos_clusts_.size(); ++i){
		spike_buf_pos_clusts_[i] -= std::min(shift_new_start, (int)spike_buf_pos_clusts_[i]);
	}

	Log("Spike buffer rewind at pos ", spike_buf_pos);

	spike_buf_pos = SPIKE_BUF_HEAD_LEN;
}

void LFPBuffer::add_data(unsigned char* new_data, size_t data_size) {
#ifdef PIPELINE_THREAD
	std::lock_guard<std::mutex> lk(chunk_access_mtx_);
#endif

	if (chunk_buf_ptr_in_ + data_size >= chunk_buf_len_){
		Log("ERROR: input buffer overflow, cut the data, bytes: ", chunk_buf_ptr_in_ + data_size - chunk_buf_len_ );
		// TODO : continue without data
		data_size = chunk_buf_len_ - chunk_buf_ptr_in_;
	}

	memcpy(chunk_buf_ + chunk_buf_ptr_in_, new_data, data_size);
	chunk_buf_ptr_in_ += data_size;
}

void LFPBuffer::estimate_firing_rates() {
	// estimate firing rates => spike sampling rates if model has not been loaded
	if (!fr_estimated_ && last_pkg_id > FR_ESTIMATE_DELAY){
		std::stringstream ss;
		ss << "FR estimate delay over (" << FR_ESTIMATE_DELAY << "). Estimate firing rates => sampling rates and start spike collection";
		Log(ss.str());

		// estimate firing rates
		std::vector<unsigned int> spike_numbers_, spikes_discarded_, spikes_slow_;
		spike_numbers_.resize(tetr_info_->tetrodes_number());
		spikes_slow_.resize(tetr_info_->tetrodes_number());
		spikes_discarded_.resize(tetr_info_->tetrodes_number());
		// TODO which pointer ?
		//				const unsigned int frest_pos = (WAIT_FOR_SPEED_EST ? spike_buf_pos_speed_ : spike_buf_pos_unproc_);
		const unsigned int frest_pos = spike_buf_pos_unproc_;
		Log("Estimate FR from spikes in buffer until position ", frest_pos);
		// first spike pkg_id
		const unsigned int first_spike_pkg_id = spike_buffer_[0]->pkg_id_;
		for (unsigned int i=0; i < frest_pos; ++i){
			Spike *spike = spike_buffer_[i];
			if (spike->discarded_){
				spikes_discarded_[spike->tetrode_] ++;
				continue;
			}

			//					if (spike->speed < SPEED_THOLD){

			// DEBUG
			//					if (spike->tetrode_ == 0){
			//						std::cout << spike->pkg_id_ << " ";
			//					}

			//						spikes_slow_[spike->tetrode_] ++;
			//						continue;
			//					}

			if (spike == nullptr || !spike->aligned_){
				continue;
			}

			spike_numbers_[spike->tetrode_] ++;
		}

		for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
			double firing_rate = spike_numbers_[t] * SAMPLING_RATE / double(FR_ESTIMATE_DELAY - first_spike_pkg_id);
			std::stringstream ss;
			ss << "Estimated firing rate for tetrode #" << t << ": " << firing_rate << " spk / sec (" << spike_numbers_[t] << " spikes)\n";
			ss << "   with " << spikes_discarded_[t] << " spikes DISCARDED\n";
//			ss << "   with " << spikes_slow_[t] << " spikes SLOW\n";

			fr_estimates_.push_back(firing_rate);
			Log(ss.str());
		}

		fr_estimated_ = true;
	}
}
