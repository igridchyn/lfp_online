/*
 * LFPBuffer.cpp
 *
 *  Created on: Sep 28, 2014
 *      Author: Igor Gridchyn
 */

#include "LFPBuffer.h"
#include "OnlineEstimator.cpp"
#include "time.h"

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
	spike_buf_pos_pred_start_ = 0;

	// main tetrode info
	if (tetr_info_)
		delete tetr_info_;
	tetr_info_ = new TetrodesInfo(config->getString("tetr.conf.path"), this);
	config->setTetrodes(tetr_info_->tetrodes_number());

	if (tetr_info_->tetrodes_number() != config_->tetrodes.size() &&  config_->tetrodes.size() > 0){
		Log("ERROR: Tetrodes list in main config does not correspond to the information in tetrodes config.");
		exit(LFPONLINE_BAD_TETRODES_CONFIG);
	}

	if (tetr_info_->status_ == TI_STATUS_BAD_TETRODES_NUMBER){
		Log("ERROR: Bad tetrodes nuber, exiting...");
		exit(LFPONLINE_BAD_TETRODES_CONFIG);
	}

	cells_.resize(tetr_info_->tetrodes_number());
	for (size_t tetr = 0; tetr < tetr_info_->tetrodes_number(); ++tetr ){
		cells_[tetr].push_back(PutativeCell());

//		if (tetr_info_->channels_number(tetr) > 4){
//			Log("ERROR: GROUPS WITH MORE THAN 4 CHANNELS NOT SUPPROTED. SORRY :(. PLEASE, SELECT SUBSET OF MAX 4 CHANNELS AND RESTART");
//			exit(438762);
//		}
	}

	// set dimensionalities for each number of channels per tetrode
	std::vector<int> pc_per_chan;
	// dummy - 0 channels
	pc_per_chan.push_back(0);
	const unsigned int npc4 = config->getInt("pca.num.pc", 3);
	for (int nc = 1; nc < 9; ++ nc){
		const unsigned int npc_nc = config->getInt(Utils::Converter::Combine("pca.num.pc.", nc) , npc4);
		pc_per_chan.push_back(npc_nc); // - nc + 4);
	}

	Log("Number of PCs per channel for a group of 4 channels: ", (int)pc_per_chan[4]);
	Log("Number of PCs per channel for a group of 3 channels: ", (int)pc_per_chan[3]);
	Log("Number of PCs per channel for a group of 2 channels: ", (int)pc_per_chan[2]);

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
		TetrodesInfo *new_ti = new TetrodesInfo(tipath, this);
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
		powerEstimators_.push_back(new OnlineEstimator<float, float>(config->getInt("spike.detection.min.power.samples", 500000)));
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
        	// OOR with channel number?
        	if (tetr_info_->tetrode_channels[tetr][ci] > CHANNEL_NUM){
        		Log("ERROR: ONE OF THE CHANNELS IN THE TETRODE CONFIG IS BEYOND THE GIVEN CHANNEL NUMBER RANGE: ", CHANNEL_NUM);
        		exit(9234);
        	}

            powerEstimatorsMap_[tetr_info_->tetrode_channels[tetr][ci]] = powerEstimators_[tetr];
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
    		power_buf[c] = new unsigned int[LFP_BUF_LEN + WS_SHIFT];

	        memset(signal_buf[c], 0, (LFP_BUF_LEN + WS_SHIFT) * sizeof(signal_type));
	        memset(filtered_signal_buf[c], 0, LFP_BUF_LEN * sizeof(ws_type));
	        memset(power_buf[c], 0, LFP_BUF_LEN * sizeof(unsigned int));
    	}
    }

    population_vector_window_.clear();
    population_vector_window_.resize(tetr_info_->tetrodes_number());
    // initialize for counting unclusterred spikes
    // clustering processors should resize this to use for clustered spikes clusters
    for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
    	population_vector_window_[t].push_back(0);
    }

	// ??? rather init all spikes ???
	// memset(spike_buffer_, 0, SPIKE_BUF_LEN * sizeof(Spike*));

	is_high_synchrony_tetrode_ = new bool[tetr_info_->tetrodes_number()];
	memset(is_high_synchrony_tetrode_, 0, sizeof(bool) * tetr_info_->tetrodes_number());
	for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		is_high_synchrony_tetrode_[config_->synchrony_tetrodes_[t]] =  true;
	}

	high_synchrony_tetrode_spikes_ = 0;

	if (fr_load_){
		Utils::FS::CheckFileExistsWithError(fr_path_, this);

		std::ifstream fsrs(fr_path_);
		Log("Loading firing rate estimates\n");
		fr_estimated_ = true;
		std::stringstream ss;
		for (unsigned int t = 0; t < tetr_info_->tetrodes_number(); ++t){
			double fr;
			fsrs >> fr;
			fr_estimates_.push_back(fr);
			//tetrode_sampling_rates_.push_back(sr);
			ss << "Firing rate at tetrode " << t << ": " << fr << "\n";
		}

		for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
			sync_spikes_window_ += fr_estimates_[config_->synchrony_tetrodes_[t]] * POP_VEC_WIN_LEN / 1000.0;
		}

		ss << "Number of synchorny spikes in window of length " << POP_VEC_WIN_LEN << ": "  << sync_spikes_window_ << "\n";
		Log(ss.str());
	} else if (FR_ESTIMATE_DELAY < input_duration_){
		Log("ERROR: FR estimate delay < input duration!");
		exit(923459);
	}

	log_stream << "INFO: BUFFER CREATED\n";
	log_stream.flush();

	// LOG ALL PARAMS (was in separate file before)
	config->dump(log_stream);
	log_stream << "Tetrodes configuration:\n";
	log_stream << tetr_info_->tetrodes_number() << "\n";
	for (unsigned int t = 0; t < tetr_info_->tetrodes_number(); ++t){

		unsigned int chn = tetr_info_->channels_number(t);
		log_stream << chn << "\n";
		for (unsigned int c = 0; c < chn; ++c){
			log_stream << tetr_info_->tetrode_channels[t][c] << " ";
		}
		log_stream << "\n";
	}
}

LFPBuffer::~LFPBuffer(){
	Log("Delete pools");
	delete[] spike_pool_;

	delete spikes_ws_pool_;
	delete spikes_ws_final_pool_;
	// multiple pools implementation
//	for (unsigned int nchan=0; nchan<=8; ++nchan){
//		delete spikes_ws_pools_[nchan];
//		delete spikes_ws_final_pools_[nchan];
//	}

	delete spike_features_pool_;
	delete spike_extra_features_ptr_pool_;

	Log("Delete chunk buffer");
	delete[] chunk_buf_;

	Log("Buffer destructor called, delete spike pool and buffer");

	delete[] spike_buffer_;
	delete[] tmp_spike_buf_;

	delete[] positions_buf_;
	delete[] tmp_pos_buf_;

	Log("Buffer destructor: delete signal / filtered signal / power buffers");

	for (size_t c = 0; c < CHANNEL_NUM; ++c){
		if (is_valid_channel_){
			delete[] signal_buf[c];
			delete[] filtered_signal_buf[c];
			delete[] power_buf[c];
		}
	}
	delete[] signal_buf;

	Log("Buffer destructor: delete speed estimator");
	delete speedEstimator_;

	Log("Buffer destructor: delete last spike positions");
	 if (last_spike_pos_)
	    	delete[] last_spike_pos_;

	 Log("Buffer destructor: delete power estimators");
	 for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
		 delete powerEstimators_[t];
	 }

	 Log("Buffer destructor: delete high synchrony tetrodes info");
	 delete[] is_high_synchrony_tetrode_;

	Log("Buffer destructor: delete tetrode info");
	if (tetr_info_)
		delete tetr_info_;

	delete[] is_valid_channel_;

	for (size_t i = 0; i < alt_tetr_infos_.size(); ++i){
		delete alt_tetr_infos_[i];
	}

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
, chunk_buf_len_(config->getInt("buf.chunk.buf.len", 432*10000))
, target_pkg_id_(config->getInt("debug.target.pkg", 0))
, target_buf_pos_(config->getInt("debug.target.bufpos", 0))
, SPEED_ESTIMATOR_WINDOW_(config->getInt("speed.est.window", 16))
, spike_waveshape_pool_size_(config->getInt("waveshape.pool.size", SPIKE_BUF_LEN + SPIKE_BUF_HEAD_LEN * 2))
, FR_ESTIMATE_DELAY(config->getInt("buf.frest.delay"))
, REWIND_GUARD(config->getInt("buf.rewind.guard", 1000000))
, POS_SAMPLING_RATE(config->getInt("pos.sampling.rate", 480))
, fr_path_(config->getString("out.path.base") + config->getString("buf.fr.path", "frates.txt"))
, fr_save_(config->getBool("buf.fr.save", false))
, fr_load_(config->getBool("buf.fr.load", false))
, adjust_synchrony_rate_(config->getBool("buf.adjust.synchrony.rate", true))
, TARGET_SYNC_RATE(config->getFloat("high.sync.target.freq", 1.0f))
, clures_readonly_(config->getBool("buf.clures.readonly", true))
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

	std::string logpath = config->getString("out.path.base") + log_path_prefix + config->timestamp_+ ".log";
	log_stream.open(logpath, std::ios_base::app);

	Log("Created LOG");

    Reset(config);

    spike_buf_pos_clusts_.resize(500);
    last_preidction_window_ends_.resize(500);

    positions_buf_ = new SpatialInfo[POS_BUF_LEN];
    POS_BUF_HEAD_LEN = POS_BUF_LEN / 10;
    tmp_pos_buf_ = new SpatialInfo[POS_BUF_HEAD_LEN];

    // allocate memory for waveshapes
    // TODO : allocate according to channels number tetrode-wise

    // multiple pools implementation
    // dummy
//    spikes_ws_pools_.push_back(new PseudoMultidimensionalArrayPool<ws_type>(1, 128, 1));
//	spikes_ws_final_pools_.push_back(new PseudoMultidimensionalArrayPool<int>(1, 16, 1));

	// n groups of c channels
	std::vector<unsigned int> ngroups;
	ngroups.resize(9);

	unsigned int maxChanNum = 0;
	for (unsigned int t = 0;t < tetr_info_->tetrodes_number(); ++t){
		unsigned int chnum = tetr_info_->channels_number(t);
		ngroups[chnum] ++;
		if (chnum > maxChanNum)
			maxChanNum = chnum;
	}

	unsigned int maxFeatDim = *(std::max_element(feature_space_dims_.begin(), feature_space_dims_.end()));

	Log("Max number of channels: ", maxChanNum);
	Log("Max feature dimension : ", maxFeatDim);

//    for (unsigned int nchan = 1; nchan <= 8; ++nchan){
//    	// pool size equals <NUMBER TETRODES WITH nchan CHANNELS * (pool size given in config) >
//    	unsigned int pool_size = 1;
//    	if (ngroups[nchan] > 0){
//    		pool_size = spike_waveshape_pool_size_ * ngroups[nchan];
//    		Log(std::string("Pool size of number of channels ") + Utils::Converter::int2str(nchan) + " equals ", pool_size);
//    	}
//    	spikes_ws_pools_.push_back(new PseudoMultidimensionalArrayPool<ws_type>(nchan, 128, pool_size));
//
//    	spikes_ws_final_pools_.push_back(new PseudoMultidimensionalArrayPool<int>(nchan, 16, SPIKE_BUF_LEN + 100));
//    }
    spikes_ws_pool_ = new PseudoMultidimensionalArrayPool<ws_type>(maxChanNum, 128, spike_waveshape_pool_size_);
    spikes_ws_final_pool_ = new PseudoMultidimensionalArrayPool<int>(maxChanNum, 16, SPIKE_BUF_LEN + 100);

    // TODO pools with dynamic size for each number of features
    // extra pool size for local file reader objects
    spike_features_pool_ = new LinearArrayPool<float>(maxFeatDim, SPIKE_BUF_LEN + 100);
    spike_extra_features_ptr_pool_ = new LinearArrayPool<float *>(4, SPIKE_BUF_LEN + 100);

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
    timestamp_buf_len = chunk_buf_len_ / CHANNEL_NUM * 2;
    timestamp_buf_.reserve(timestamp_buf_len); // just optimization

    // TODO !!! PARAMETRIZE
    synchronyThresholdAdapter_ = new Utils::NewtonSolver(TARGET_SYNC_RATE, 24000*60, -0.5, high_synchrony_factor_);

    for (unsigned int st = 0; st < config_->synchrony_tetrodes_.size(); ++st){
    	if (config_->synchrony_tetrodes_[st] >= tetr_info_->tetrodes_number()){
    		Log("ERROR: one of the  synchrony tetodes is beyond available tetrode range.");
    		exit(23419);
    	}
    }

    global_cluster_number_shfit_.resize(tetr_info_->tetrodes_number());

    for (unsigned int s=0; s < config->spike_files_.size(); ++s){
    	std::string clupath = config_->spike_files_[s] + "clu";
    	if (Utils::FS::FileExists(clupath)){
    	    std::ifstream  src(clupath , std::ios::binary);
    	    std::ofstream  dst(clupath + ".old",   std::ios::binary);

    	    dst << src.rdbuf();
    	    src.close();
    	    dst.close();
    	}
    	std::string respath = config_->spike_files_[s] + "res";
    	if (Utils::FS::FileExists(respath)){
    	    std::ifstream  src(respath, std::ios::binary);
    	    std::ofstream  dst(respath + ".old",   std::ios::binary);

    	    dst << src.rdbuf();
    	    src.close();
    	    dst.close();
    	}
    }

    clusters_in_tetrode_.resize(tetr_info_->tetrodes_number());

	shared_values_int_[SHARED_VALUE_LAST_TRIGGERED_SWR] = -1;
	shared_values_int_[SHARED_VALUE_LAST_TRIGGER_PKG] = -1;
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

//template<class T>
//void PseudoMultidimensionalArrayPool<T>::Expand(){
//	this->ExpandPool();
//
//	T *new_array_ = new T [dim2_ * dim1_ * pool_size_];
//	T **new_array_rows_ = new T*[dim1_ * pool_size_];
//    for (unsigned int s=0; s < dim1_ * pool_size_; ++s){
//    	new_array_rows_[s] = new_array_ + s * dim2_;
//    }
//
//    for (unsigned int s=0; s < pool_size_; ++s){
//    	this->MemoryFreed(new_array_rows_ + dim1_ * s);
//    }
//
//    pool_size_ *= 2;
//}

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

    if (is_high_synchrony_tetrode_[spike->tetrode_])
    	high_synchrony_tetrode_spikes_ ++;
}


void LFPBuffer::AddSpike(bool rewind) {
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

bool LFPBuffer::IsHighSynchrony() {
	RemoveSpikesOutsideWindow(last_pkg_id);

	if (fr_estimates_.empty())
		return false;

	// try update
	if (adjust_synchrony_rate_ && synchronyThresholdAdapter_->NeedUpdate(last_pkg_id)){
		// find number of events and calculate last frequency
		unsigned i = swrs_.size() - 1;
		while (i > 0 && swrs_[i-1][0] > synchronyThresholdAdapter_->last_update_){
			i --;
		}

		double frequency =  (swrs_.size() - i + 1) * SAMPLING_RATE / float((last_pkg_id -  synchronyThresholdAdapter_->last_update_));
		std::stringstream ss;
		ss << "UPDATE HIGH SYNCHRONY THRESHOLD: target freqneucy = " << TARGET_SYNC_RATE << ", current frequency = " << frequency << ", old threshold = " <<
				high_synchrony_factor_;
		high_synchrony_factor_ = (float)synchronyThresholdAdapter_->Update(last_pkg_id, frequency);
		ss << ", new threshold = " << high_synchrony_factor_;
		Log(ss.str());
	}

	// TETRODE-WISE increase + # of synchronous tetrodes
//	int nhigh = 0;
//	for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t){
//		if (population_vector_window_[config_->synchrony_tetrodes_[t]][0] > POP_VEC_WIN_LEN * fr_estimates_[config_->synchrony_tetrodes_[t]] / 1000.0 * high_synchrony_factor_){
//			nhigh ++;
//		}
//	}
//	return nhigh > 6;

	// whether have at least synchrony.factor X average spikes at all tetrodes
	return (high_synchrony_tetrode_spikes_ >= sync_spikes_window_ * high_synchrony_factor_);
}


void LFPBuffer::Log() {
	std::cout << log_string_stream_.str();
	log_stream << log_string_stream_.str();
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

void LFPBuffer::ResetAC(const int& reset_tetrode) {
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
	if (!valid)
		return nanf("");

	if (Utils::Math::Isnan(smallLED)){
		return bigLED;
	}
	else if (Utils::Math::Isnan(bigLED)){
		return smallLED;
	}
	else{
		return (smallLED + bigLED) / 2.0f;
	}
}

float SpatialInfo::x_pos() const{
	return AverageLEDs(x_small_LED_, x_big_LED_, valid);
}

float SpatialInfo::y_pos() const{
	return AverageLEDs(y_small_LED_, y_big_LED_, valid);
}

SpatialInfo::SpatialInfo(const float& xs, const float& ys, const float& xb, const float& yb, const unsigned long long& ts)
	: x_small_LED_(xs)
	, y_small_LED_(ys)
	, x_big_LED_(xb)
	, y_big_LED_(yb)
	, timestamp_(ts)
{ }

SpatialInfo::SpatialInfo()
: x_small_LED_(nanf(""))
, y_small_LED_(nanf(""))
, x_big_LED_(nanf(""))
, y_big_LED_(nanf(""))
, pkg_id_(0)
, valid(false)
{}

void SpatialInfo::Init(const float& xs, const float& ys, const float& xb, const float& yb, const unsigned long long& ts){
	x_small_LED_ = xs;
	y_small_LED_ = ys;
	x_big_LED_ = xb;
	y_big_LED_ = yb;
    timestamp_ = ts;
}

std::ostream& operator<<(std::ostream& out, const SpatialInfo& si)
{
    // const auto ts = std::chrono::microseconds(si.timestamp_);
    // display(out, ts, false);
    out << si.timestamp_ << ',' << si.x_big_LED_ << ',' << si.y_big_LED_ << ',' << si.x_small_LED_ << ',' << si.y_small_LED_;
    return out;
}

// multiple pools implementation
void LFPBuffer::AllocateWaveshapeMemory(Spike *spike) {
//	unsigned int nchan = tetr_info_->channels_number(spike->tetrode_);
//	PseudoMultidimensionalArrayPool<ws_type> * spikes_ws_pool_ = spikes_ws_pools_[nchan];

	if (spikes_ws_pool_->Empty()){
		Log("ERROR: waveshape empty pool");
		exit(87234);

//		Log("Expand spike ws pool for number of channels = ", nchan);
//		Log("	new pool size = ", spikes_ws_pool_->PoolSize() * 2);
//		spikes_ws_pool_->Expand();
	}
	AllocatePoolMemory<ws_type*>(&spike->waveshape, spikes_ws_pool_);
}

void LFPBuffer::FreeWaveshapeMemory(Spike* spike) {
//	unsigned int nchan = tetr_info_->channels_number(spike->tetrode_);
	FreetPoolMemory<ws_type*>(&spike->waveshape, spikes_ws_pool_);
}

void LFPBuffer::AllocateFinalWaveshapeMemory(Spike* spike) {
//	unsigned int nchan = tetr_info_->channels_number(spike->tetrode_);
//	PseudoMultidimensionalArrayPool<int> * spikes_ws_final_pool_ = spikes_ws_final_pools_[nchan];
	if (spikes_ws_final_pool_->Empty()){
		Log("ERROR: waveshape empty pool");
		exit(87235);
//		Log("Expand spike final ws pool for number of channels = ", nchan);
//		Log("	new pool size = ", spikes_ws_final_pool_->PoolSize() * 2);
//		spikes_ws_final_pool_->Expand();
	}
	AllocatePoolMemory<int*>(&spike->waveshape_final, spikes_ws_final_pool_);
}

void LFPBuffer::FreeFinalWaveshapeMemory(Spike* spike) {
//	unsigned int nchan = tetr_info_->channels_number(spike->tetrode_);
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
	if (last_pkg_id < REWIND_GUARD){
		Log("ERROR: buffer rewind happened before the guarded time. Increase buffer size or adjust other parameters (e.g. detection threshold");
		exit(249871);
	}

	// exchange head and tail1
	memcpy(tmp_spike_buf_, spike_buffer_ + spike_buf_pos - SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);
	// to reuse the same objects that are in the head
	memcpy(spike_buffer_ + spike_buf_pos - SPIKE_BUF_HEAD_LEN, spike_buffer_, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);
	memcpy(spike_buffer_, tmp_spike_buf_, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);

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
	spike_buf_pos_pred_start_ -= std::min(shift_new_start, (int)spike_buf_pos_pred_start_);

	for (size_t i=0; i < spike_buf_pos_clusts_.size(); ++i){
		spike_buf_pos_clusts_[i] -= std::min(shift_new_start, (int)spike_buf_pos_clusts_[i]);
	}

	Log("Spike buffer rewind at pos ", spike_buf_pos);
	Log("Last package id: ", last_pkg_id);

	spike_buf_pos = SPIKE_BUF_HEAD_LEN;
}

void LFPBuffer::add_data(const unsigned char* new_data, size_t data_size, int timestamp) {
#ifdef PIPELINE_THREAD
	std::lock_guard<std::mutex> lk(chunk_access_mtx_);
#endif

	if (chunk_buf_ptr_in_ + data_size >= chunk_buf_len_){
		Log("ERROR: input buffer overflow, cut the data, bytes: ", chunk_buf_ptr_in_ + data_size - chunk_buf_len_ );
		data_size = chunk_buf_len_ - chunk_buf_ptr_in_ - 1;
	}

	memcpy(chunk_buf_ + chunk_buf_ptr_in_, new_data, data_size);

    if (timestamp_buf_.size() > timestamp_buf_len)
        timestamp_buf_.clear(); // if no processor consumes timestamps this buffer may grow too large

    if (timestamp != std::numeric_limits<int>::min())
        timestamp_buf_.push_back(timestamp);

	chunk_buf_ptr_in_ += data_size;
}

void LFPBuffer::estimate_firing_rates() {
	// estimate firing rates => spike sampling rates if model has not been loaded AND PCA EXTRACTION STARTED
	if (!fr_estimated_ && last_pkg_id > FR_ESTIMATE_DELAY && spike_buf_no_rec > 0){
		std::stringstream ss;
		ss << "FR estimate delay over (" << FR_ESTIMATE_DELAY << "). Estimate firing rates => sampling rates and start spike collection";
		Log(ss.str());

		// estimate firing rates
		std::vector<unsigned int> spike_numbers_, spikes_discarded_, spikes_slow_;
		spike_numbers_.resize(tetr_info_->tetrodes_number());
		spikes_slow_.resize(tetr_info_->tetrodes_number());
		spikes_discarded_.resize(tetr_info_->tetrodes_number());
		const unsigned int frest_pos = spike_buf_no_rec;
		Log("Estimate FR from spikes in buffer until position ", frest_pos);
		// first spike pkg_id
		const unsigned int first_spike_pkg_id = spike_buffer_[0]->pkg_id_;
		for (unsigned int i=0; i < frest_pos; ++i){
			Spike *spike = spike_buffer_[i];
			if (spike->discarded_){
				spikes_discarded_[spike->tetrode_] ++;
				continue;
			}

			spike_numbers_[spike->tetrode_] ++;
		}

		double seconds_est = double(spike_buffer_[frest_pos - 1]->pkg_id_ - first_spike_pkg_id) / (double)SAMPLING_RATE;
		Log("Estimating firing rate in seconds: ", seconds_est);

		for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
			double firing_rate = spike_numbers_[t]  / seconds_est;
			std::stringstream ss;
			ss << "Estimated firing rate for tetrode #" << t << ": " << firing_rate << " spk / sec (" << spike_numbers_[t] << " spikes)\n";
			ss << "   with " << spikes_discarded_[t] << " spikes DISCARDED\n";
//			ss << "   with " << spikes_slow_[t] << " spikes SLOW\n";

			fr_estimates_.push_back(firing_rate);
			Log(ss.str());
		}

		// write sampling rates to file
		if (fr_save_){
			std::ofstream fsampling_rates(fr_path_);
			for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
				fsampling_rates << fr_estimates_[t] << "\n";//tetrode_sampling_rates_[t] << "\n";
			}
		}

		fr_estimated_ = true;

		for (size_t t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
			sync_spikes_window_ += fr_estimates_[config_->synchrony_tetrodes_[t]] * POP_VEC_WIN_LEN / 1000.0;
		}
	}
}

void LFPBuffer::AdvancePositionBufferPointer(){
	if (pos_buf_pos_ == POS_BUF_LEN - 1){
		// REWIND
		Log("Rewind pos buf pos at ", pos_buf_pos_);
		Log("	With last_pkg_id at ", last_pkg_id);

		// exchange head and tail1
		memcpy(tmp_pos_buf_, positions_buf_ + pos_buf_pos_ - POS_BUF_HEAD_LEN, sizeof(SpatialInfo)*POS_BUF_HEAD_LEN);
		// to reuse the same objects that are in the head
		//memcpy(positions_buf_ + pos_buf_pos_ - POS_BUF_HEAD_LEN, positions_buf_, sizeof(Spike*)*POS_BUF_HEAD_LEN);
		memset(positions_buf_, 0,  sizeof(SpatialInfo) * POS_BUF_LEN);
		memcpy(positions_buf_, tmp_pos_buf_, sizeof(SpatialInfo)*POS_BUF_HEAD_LEN);

		const int shift_new_start = pos_buf_pos_ - POS_BUF_HEAD_LEN;
		pos_buf_disp_pos_ -= std::min(shift_new_start, (int)pos_buf_disp_pos_);
		pos_buf_pos_speed_est -= std::min(shift_new_start, (int)pos_buf_pos_speed_est);
		pos_buf_pos_spike_speed_ -= std::min(shift_new_start, (int)pos_buf_pos_spike_speed_);
		pos_buf_pos_whl_writer_ -= std::min(shift_new_start, (int)pos_buf_pos_whl_writer_);
		pos_buf_spike_pos_ -= std::min(shift_new_start, (int)pos_buf_spike_pos_);
		pos_buf_trans_prob_est_ -= std::min(shift_new_start, (int)pos_buf_trans_prob_est_);

		pos_buf_pos_ = POS_BUF_HEAD_LEN;

		pos_rewind_shift += POS_BUF_LEN - POS_BUF_HEAD_LEN;
	}
	else{
		pos_buf_pos_ ++;
	}
}

const SpatialInfo& LFPBuffer::PositionAt(const unsigned int& pkg_id){
	return positions_buf_[PositionIndexByPacakgeId(pkg_id)];
}

const unsigned int LFPBuffer::PositionIndexByPacakgeId(const unsigned int& pkg_id){
	const unsigned int abs_index = pkg_id / POS_SAMPLING_RATE;
	return abs_index - pos_rewind_shift;
}

const unsigned int LFPBuffer::PacakgeIdByPositionIndex(const unsigned int& pos_index){
	return (pos_index + pos_rewind_shift) * POS_SAMPLING_RATE;
}

void LFPBuffer::AddWaveshapeCut(const unsigned int & tetr, const unsigned int & cell, WaveshapeCut cut){
	if (cells_[tetr].size() <= cell){
		cells_[tetr].resize(cell + 1);
	}
	cells_[tetr][cell].waveshape_cuts_.push_back(cut);

	//update cluster identities
	for (unsigned int i=0; i < spike_buf_pos; ++i){
		Spike *spike = spike_buffer_[i];
		if (spike->tetrode_ == (int)tetr && spike->cluster_id_ == (int)cell && cut.Contains(spike)){
			spike->cluster_id_ = -1;
		}
	}

	spike_buf_no_disp_pca = 0;
}

void LFPBuffer::DeleteWaveshapeCut(const unsigned int & tetr, const unsigned int & cell, const unsigned int & at){
	std::vector<WaveshapeCut>& cuts = cells_[tetr][cell].waveshape_cuts_;

	cuts.erase(cuts.begin() + at, cuts.begin() + at + 1);

	for (unsigned int i=0; i < spike_buf_pos; ++i){
		Spike *spike = spike_buffer_[i];
		if (cells_[tetr][cell].Contains(spike) && spike->cluster_id_ == -1){
			spike->cluster_id_ = cell;
		}
	}
}

void LFPBuffer::Log(std::string message, std::vector<unsigned int> array, bool print_order){
	Log(message);
	for (unsigned int i=0; i < array.size(); ++i){
		if (print_order)
			Log(Utils::Converter::int2str(array[i]) + " (" + Utils::Converter::int2str(i) + ")");
		else{
			Log(Utils::Converter::int2str(array[i]));
		}
	}
}

void LFPBuffer::calculateClusterNumberShifts(){
	// reset - because of possible trailing empty clusters
	for (unsigned int t=0; t < tetr_info_->tetrodes_number(); ++t){
		clusters_in_tetrode_[t] = 0;
	}

	unsigned int i=0;
    while (i < spike_buf_pos_speed_){
        Spike *spike = spike_buffer_[i++];

        unsigned int tetr = spike->tetrode_;
        int clust = spike->cluster_id_;

        if (clust > 0 &&  clust > (int)clusters_in_tetrode_[tetr]){
        	clusters_in_tetrode_[tetr] = clust;
        }
    }

    Log("Clusters per tetrodes:");
    unsigned int total_clusters = 0;
    for (unsigned int t=0; t < tetr_info_->tetrodes_number(); ++t){
    	std::stringstream ss;
    	ss << "Clusters in tetrode " << t << " : " << clusters_in_tetrode_[t];
    	Log(ss.str());

    	global_cluster_number_shfit_[t] = total_clusters;
    	total_clusters += clusters_in_tetrode_[t];
    }

    Log("Total clusters: ", total_clusters);
}

void LFPBuffer::dumpCluAndRes(bool recalculateClusterNumbers){
	if (recalculateClusterNumbers){
		calculateClusterNumberShifts();
	}

	if (clures_readonly_){
		Log("WARNING: CLU AND RES ARE READ-ONLY, CANNOT DUMP");
		return;
	}

	Log("START SAVING CLU/RES");
	Log("Global cluster number shifts: ", global_cluster_number_shfit_, true);

	std::ofstream res_global, clu_global;

	int current_session = 0;
//	res_global.open(buffer->config_->getString("out.path.base") + "all.res");
//	clu_global.open(buffer->config_->getString("out.path.base") + "all.clu");
	res_global.open(config_->spike_files_[current_session] + "res");
	clu_global.open(config_->spike_files_[current_session] + "clu");

	for (unsigned int i=0; i < spike_buf_pos; ++i){
		Spike *spike = spike_buffer_[i];

		// next session?
		if (current_session < (int)session_shifts_.size() - 1 && spike->pkg_id_ > session_shifts_[current_session + 1]){
			res_global.close();
			clu_global.close();
			current_session ++;
			res_global.open(config_->spike_files_[current_session] + "res");
			clu_global.open(config_->spike_files_[current_session] + "clu");
		}

		if (spike->cluster_id_ > 0){
			res_global << spike->pkg_id_ - session_shifts_[current_session] << "\n";
			clu_global << global_cluster_number_shfit_[spike->tetrode_] + spike->cluster_id_ << "\n";
		// IF NEED FULL DUMP AND CO
		}
//		else {
//			res_global << spike->pkg_id_ - session_shifts_[current_session] << "\n";
//			clu_global << "-" << spike->tetrode_ << "\n";
//		}
	}

	clu_global.close();
	res_global.close();
	Log("FINISHED SAVING CLU/RES");

	// write cluster shifts in every dump directory
	for (unsigned int s=0; s < config_->spike_files_.size(); ++s){
		std::ofstream cluster_shifts(config_->spike_files_[s] + std::string("cluster_shifts"));
		for (size_t t=0; t < global_cluster_number_shfit_.size(); ++t){
			cluster_shifts << global_cluster_number_shfit_[t] << "\n";
		}
	}
}

