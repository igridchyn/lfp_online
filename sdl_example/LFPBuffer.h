//
//  LFPBuffer.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_LFPBuffer_h
#define sdl_example_LFPBuffer_h

#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <sys/stat.h>
#include <string>
#include <mutex>
#include <map>

#include "LFPOnline.h"
#include "TetrodesInfo.h"
#include "Spike.h"
#include "OnlineEstimator.h"
#include "Config.h"
#include "UserContext.h"
#include "Utils.h"
#include "PutativeCell.h"

#include <armadillo>

#include <boost/filesystem.hpp>

// !!! DON'T FORGET TO EXCLUDE BINARY READER !!!
//#define PIPELINE_THREAD

#define CHAR_SIGNAL
typedef char signal_type;

enum BinFileFormat{
	BFF_AXONA,
	BFF_MATRIX
};

enum LFPPipelineStatus {
	PIPELINE_STATUS_READ_BIN,
	PIPELINE_STATUS_READ_FET,
	PIPELINE_STATUS_ONLINE,
	PIPELINE_STATUS_INPUT_OVER
};

enum LFPONLINE__ERROR_CODES {
	LFPONLINE_ERROR_NO_BIN_FILE_PROVIDED,
	LFPONLINE_ERROR_BIN_FILE_DOESNT_EXIST_OR_UNAVAILABLE,
	LFPONLINE_ERROR_UNKNOWN_PROCESSOR,
	LFPONLINE_ERROR_MISSING_PARAMETER,
	LFPONLINE_ERROR_DEFAULT_OUTPATH_NOT_ALLOWED,
	LFPONLINE_ERROR_SPIKE_FILES_LIST_EMPTY,
	LFPONLINE_ERROR_FET_FILE_DOESNT_EXIST_OR_UNAVAILABLE,
	LFPONLINE_ERROR_UNKNOWN_POS_BUFFER_POINTER,
	LFPONLINE_WRONG_TETRODES_NUMBER,
	LFPONLINE_BAD_ALLOC,
	LFPONLINE_BAD_TETRODES_CONFIG,
	LFPONLINE_BUFFER_TOO_SHORT,
	LFPONLINE_REQUESTING_INVALID_CHANNEL
};

enum SHARED_VALUE {
	SHARED_VALUE_FILTER_LENGTH,
	SHARED_VALUE_LAST_TRIGGERED_SWR,
	SHARED_VALUE_LAST_TRIGGER_PKG,
	SHARED_VALUE_ARRAY_SIZE
};

class SpatialInfo {
public:
	// if only one LED pos is known, the other one will be equal to -1
	float x_small_LED_ = .0;
	float y_small_LED_ = .0;
	float x_big_LED_ = .0;
	float y_big_LED_ = .0;
	unsigned int pkg_id_ = 0;
	float speed_ = .0;
	// direction variance - to distinguish fast head rotation from running
	float dirvar_ = nanf("");
	// valid if at least one LED is known
	bool valid = false;

public:

	float x_pos() const;
	float y_pos() const;

	SpatialInfo();
	SpatialInfo(const float& xs, const float& ys, const float& xb, const float& yb);
	void Init(const float& xs, const float& ys, const float& xb, const float& yb);
};

template<class T>
class QueueInterface {
protected:
	T* pool_ = nullptr;
	unsigned int pool_size_ = 0;
	unsigned int free_pos_ = 0;
	unsigned int alloc_pos_ = 0;

public:

	virtual bool Empty() {
		return free_pos_ == alloc_pos_;
	}

	virtual T GetMemoryPtr() {
		T mem = pool_[alloc_pos_];
		alloc_pos_ = (alloc_pos_ + 1) % pool_size_;

		if (mem == nullptr){
			std::cout << "free = " << free_pos_ << ", alloc = " << alloc_pos_ << ", pool size = " << pool_size_ << "\n";
			throw std::string("ERROR: queue returned empty ptr");
		}


		return mem;
	}
	virtual void MemoryFreed(T mem) {
		if (mem == nullptr){
			throw std::string("nulltrp freed!");
		}
		pool_[free_pos_] = mem;
		free_pos_ = (free_pos_ + 1) % pool_size_;
	}

	QueueInterface(const unsigned int pool_size);
	virtual ~QueueInterface() {
		delete[] pool_;
	}

	// this need to be reviewed - either have a queue of pools or old / new pool with old freed after all pointers in it returned ...
//	virtual bool ExpandPool() {
//	    // allocate new pool
//	    T *new_pool = new T[pool_size_ * 2];
//	    memcpy(new_pool, pool_, pool_size_ * sizeof(T));
//	    pool_ = new_pool;
//	    alloc_pos_ = pool_size_;
//
//	    free_pos_ = pool_size_;
//
//	    pool_size_ *= 2;
//
//	    return true;
//	}
};

template<class T>
class LinearArrayPool: public virtual QueueInterface<T*> {
	T *array_;
	unsigned int dim_;
	unsigned int pool_size_;

public:
	LinearArrayPool(unsigned int dim, unsigned int pool_size);
	virtual ~LinearArrayPool() { delete[] array_; }
};

template<class T>
class PseudoMultidimensionalArrayPool: public virtual QueueInterface<T**> {
	T *array_;
	T **array_rows_;

	unsigned int dim1_;
	unsigned int dim2_;
	unsigned int pool_size_;

public:
	PseudoMultidimensionalArrayPool(unsigned int dim1, unsigned int dim2,
			unsigned int pool_size);
	virtual ~PseudoMultidimensionalArrayPool() { delete[] array_; delete[] array_rows_; }

//	void Expand();
	unsigned int PoolSize() { return pool_size_; }
};

class LFPONLINEAPI LFPBuffer : public virtual Utils::Logger {

public:
	LFPPipelineStatus pipeline_status_ = PIPELINE_STATUS_ONLINE;

	class POS_BUF_POINTER_NAMES {
	public:
		const std::string POS_BUF_POS = "pos";
		const std::string POS_BUF_SPEED_EST = "speed.est";
		const std::string POS_BUF_SPIKE_SPEED = "spike.speed";
		const std::string POS_BUF_SPIKE_POS = "spike.pos";
	};
	POS_BUF_POINTER_NAMES pos_buf_pointer_names_;

	Config *config_ = nullptr;
	UserContext user_context_;

	const unsigned int CHANNEL_NUM;

	// length of a population vector, in number of samples (length in seconds = POP_VEC_WIN_LEN / SAMPLING_RATE)
	const unsigned int POP_VEC_WIN_LEN;

	const unsigned int SAMPLING_RATE;

	// value of pos., meaning that position is unknown
	// ACCORDING TO THE AXONA CONVENTIONS
	const int pos_unknown_;

	const unsigned int SPIKE_BUF_LEN;
	const unsigned int SPIKE_BUF_HEAD_LEN;

	const unsigned int LFP_BUF_LEN;
	const unsigned int BUF_HEAD_LEN;

	float high_synchrony_factor_;

	const unsigned int POS_BUF_LEN;
	unsigned int POS_BUF_HEAD_LEN = 0;

	int *CH_MAP; // = { 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55 };

	TetrodesInfo *tetr_info_ = nullptr;
	// alternative tetrode infos (e.g. for other models)
	std::vector<TetrodesInfo *> alt_tetr_infos_;

	Spike *spike_pool_ = nullptr;

	Spike* *tmp_spike_buf_ = nullptr;
	// spikes buffer and POINTERS [all have to be reset at buffer rewind]
	Spike* *spike_buffer_ = nullptr;
	// position, at which next spike will be put
	unsigned int spike_buf_pos;
	// position of first spike, not populated with original waveshape data (due to signal lag)
	unsigned int spike_buf_nows_pos;
	// position of first spike without reconstructed waveshape
	unsigned int spike_buf_no_rec;
	// position of first non-displayed PCA
	unsigned int spike_buf_no_disp_pca;
	// last unprocessed with PCA
	unsigned int spike_buf_pos_unproc_;
	// first non-outputted
	unsigned int spike_buf_pos_out;
	//  first non-clustered
	unsigned int spike_buf_pos_clust_;
	// drawing spikes in pos
	unsigned int spike_buf_pos_draw_xy;
	// spikes with speed estimate
	unsigned int spike_buf_pos_speed_;
	// pointers to the first spike in the population time window, for each tetrode
	unsigned int spike_buf_pos_pop_vec_;
	// place field processor
	unsigned int spike_buf_pos_pf_;
	// autocorrelogram
	unsigned int spike_buf_pos_auto_;
	// LPT triggger
	unsigned int spike_buf_pos_lpt_;
	// fet writer
	unsigned int spike_buf_pos_fet_writer_;
	// ws display
	unsigned int spike_buf_pos_ws_disp_;
	// collected by feature extractor but doesn't have features yet
	unsigned int spike_buf_pos_featext_collected_;
	// binary population classifier
	unsigned int spike_buf_pos_binary_classifier_;
	// before starting detection - to process all new spikes afterwards (esp. for parallel mode)
	unsigned int spike_buf_pos_predetect_;
	// first spike participating in current prediction
	unsigned int spike_buf_pos_pred_start_;

	std::vector<unsigned int> spike_buf_pos_clusts_;

	bool ac_reset_ = false;
	int ac_reset_tetrode_ = -1;
	int ac_reset_cluster_ = -1;

	bool clu_reset_ = false;

	// POSITION BUFFER
	SpatialInfo *positions_buf_;
	// main poiter - where the next position will be put
	unsigned int pos_buf_pos_ = 0;
	// the last displayed position
	unsigned int pos_buf_disp_pos_ = 0;
	unsigned int pos_buf_spike_pos_ = 0;
	// last for which speed has been calculated
	unsigned int pos_buf_pos_spike_speed_ = 0;
	// last for speed estimatino
	unsigned int pos_buf_pos_speed_est = 0;
	// whl writer (within SpikeWriter)
	unsigned int pos_buf_pos_whl_writer_ = 0;
	// trans prob estimation
	unsigned int pos_buf_trans_prob_est_ = 0;

	// pvw[tetrode][cluster] - number of spikes in window [last_pkg_id - POP_VEC_WIN_LEN, last_pkg_id]
	// initialized by GMM clustering processor
	// !!! the cluster number is shifted by one
	std::vector<std::vector<unsigned int> > population_vector_window_;
	unsigned int population_vector_total_spikes_ = 0;
	std::queue<Spike*> population_vector_stack_;

	// map of whether tetrode is used for high synchrony detection
	bool *is_high_synchrony_tetrode_;
	unsigned int high_synchrony_tetrode_spikes_ = 0;

	std::vector<std::vector<unsigned int>> swrs_;
	std::vector<int> swr_pointers_;

private:
	std::ofstream log_stream;

public:
	bool *is_valid_channel_;

	signal_type **signal_buf;
	std::vector<ws_type *> filtered_signal_buf;
	std::vector<unsigned int *> power_buf;

	// ??? for all arrays ?
	unsigned int buf_pos;
	unsigned int buf_pos_trig_;
	unsigned int filt_pos_;

	unsigned int last_pkg_id = 0;
	// for each tetrode
	int *last_spike_pos_ = nullptr;

	// if shift has happened, what was the previous zero level;
	int zero_level;

	std::mutex chunk_access_mtx_;
	unsigned char *chunk_buf_ = nullptr;

	size_t chunk_buf_len_ = 0;
	size_t chunk_buf_ptr_in_ = 0;

	std::vector<OnlineEstimator<float, float>* > powerEstimators_;
	std::vector<OnlineEstimator<float, float>* > powerEstimatorsMap_;

	OnlineEstimator<float, float>* speedEstimator_;

	arma::fmat cluster_spike_counts_;

	// ? predictions buffer ?
	// in case there are few processors, using processors are responsible for resizing
	std::vector<arma::fmat> last_predictions_;
	std::vector<unsigned int> last_preidction_window_ends_;

	std::vector<arma::fmat> tps_;

	unsigned int input_duration_ = 0;

	// dimensionalities of spike feature representations for each tetrode
	// this should be abstracted from the wat the features are extracted (PCs etc.)
	std::vector<unsigned int> feature_space_dims_;

	// DEBUG
	// for performance evaluation
	time_t checkpoint_ = 0;
	unsigned int target_pkg_id_ = 0;
	unsigned int target_buf_pos_ = 0;

	float coord_shift_x_ = .0;
	float coord_shift_y_ = .0;

	static const unsigned int TETRODE_UNKNOWN = 1000000;
	static const unsigned int CLUSTER_UNKNOWN = 1000000;

	std::stringstream log_string_stream_;

	unsigned int WS_SHIFT = 100;

	const unsigned int SPEED_ESTIMATOR_WINDOW_;

	bool fr_estimated_ = false;
	std::vector<double> fr_estimates_;
//	double synchrony_tetrodes_firing_rate_ = .0;

	PseudoMultidimensionalArrayPool<ws_type> *spikes_ws_pool_;
	PseudoMultidimensionalArrayPool<int> *spikes_ws_final_pool_;

//	std::vector<PseudoMultidimensionalArrayPool<ws_type> * > spikes_ws_pools_;
//	std::vector<PseudoMultidimensionalArrayPool<int> * > spikes_ws_final_pools_;

	LinearArrayPool<float> *spike_features_pool_;
	LinearArrayPool<float *> *spike_extra_features_ptr_pool_;

	const unsigned int spike_waveshape_pool_size_;

	const unsigned int FR_ESTIMATE_DELAY;

	std::string timestamp_ = "";

	const unsigned int REWIND_GUARD = 0;
	SpatialInfo* tmp_pos_buf_ = nullptr;
	unsigned int pos_rewind_shift = 0;
	const unsigned int POS_SAMPLING_RATE;

	bool processing_over_ = false;

	// average number of spikes in the synchrony window
	double sync_spikes_window_ = .0;


	// sampling rates path / save / load
	std::string fr_path_ = "";
	const bool fr_save_ = false;
	const bool fr_load_ = false;

	bool adjust_synchrony_rate_ = true;

	Utils::NewtonSolver *synchronyThresholdAdapter_;

	std::vector< std::vector <PutativeCell> > cells_;

	// shifts in pkg id if multiple files opened (fet + spk + whl + clu etc.)
	std::vector<unsigned int> session_shifts_;

	double TARGET_SYNC_RATE;

	BinFileFormat bin_file_format_;

	std::vector<unsigned int> all_sessions_;

	bool clures_readonly_ = true;

	std::vector< std::vector<float> > cluster_firing_rates_;

	// for sharing values accross processors
	std::map<int, int> shared_values_int_;

	//====================================================================================================

	LFPBuffer(Config* config);
	virtual ~LFPBuffer();

	void Reset(Config* config);

	inline bool is_valid_channel(int channel_num) {
		return is_valid_channel_[channel_num];
	}

	// Population window operations
	void RemoveSpikesOutsideWindow(const unsigned int& right_border);
	void UpdateWindowVector(Spike *spike);
	// required after cluster operations, for example
	void ResetPopulationWindow();

	bool IsHighSynchrony();

	void AddSpike(bool rewind = true);
	void Rewind();

	virtual void Log();
	virtual void Log(std::string message);
	virtual void Log(std::string message, int num);
	virtual void Log(std::string message, unsigned int num);
	virtual void Log(std::string message, std::vector<unsigned int> num, bool print_order = false);
#ifndef _WIN32
	virtual void Log(std::string message, size_t num);
#endif
	virtual void Log(std::string message, double num);

	const unsigned int& GetSpikeBufPointer(std::string name);
	const unsigned int& GetPosBufPointer(std::string name);

	// cause recalculation of autocorrelograms due to change in cluster(s) - from buffer start
	void ResetAC(const unsigned int& reset_tetrode, const int& reset_cluster);
	void ResetAC(const int& reset_tetrode);

	// DEBUG
	bool CheckPkgIdAndReportTime(const unsigned int pkg_id1, const unsigned int pkg_id2,
				const std::string msg, bool set_checkpoint = false);
	bool CheckPkgIdAndReportTime(const unsigned int& pkg_id,
			const std::string msg, bool set_checkpoint = false);
	void CheckBufPosAndReportTime(const unsigned int& buf_pos,
			const std::string msg);

	template<class T>
	void AllocatePoolMemory(T **ponter, QueueInterface<T*> *queue);
	template<class T>
	void FreetPoolMemory(T **ponter, QueueInterface<T*> *queue);

	void AllocateWaveshapeMemory(Spike* spike);
	void FreeWaveshapeMemory(Spike* spike);

	void AllocateFinalWaveshapeMemory(Spike* spike);
	void FreeFinalWaveshapeMemory(Spike* spike);

	void AllocateFeaturesMemory(Spike* spike);
	void FreeFeaturesMemory(Spike* spike);

	void AllocateExtraFeaturePointerMemory(Spike *spike);
	void FreeExtraFeaturePointerMemory(Spike *spike);

	// DEBUG
	Spike *head_start_, *tail_start_;

	// just add the data and move along
	void add_data(const unsigned char* new_data, size_t data_size);

	void estimate_firing_rates();

	void AdvancePositionBufferPointer();

	const SpatialInfo& PositionAt(const unsigned int& pkg_id);
	const unsigned int PositionIndexByPacakgeId(const unsigned int& pkg_id);
	const unsigned int PacakgeIdByPositionIndex(const unsigned int& pos_index);

	void AddWaveshapeCut(const unsigned int & tetr, const unsigned int & cell, WaveshapeCut cut);
	void DeleteWaveshapeCut(const unsigned int & tetr, const unsigned int & cell, const unsigned int & at);
	int AssignCluster(Spike *spike);

	std::vector<unsigned int> clusters_in_tetrode_;
	std::vector<unsigned int> global_cluster_number_shfit_;
	void calculateClusterNumberShifts();
	void dumpCluAndRes(bool recalculateClusterNumbers);
};

template<class T>
inline void LFPBuffer::AllocatePoolMemory(T** pointer,
		QueueInterface<T*>* queue) {
	if (*pointer != nullptr) {
		Log("ERROR: pointer requesting memory is non-zero");
		throw std::string("ERROR: pointer requesting memory is non-zero");
	}

	if (queue->Empty()) {
		Log("ERROR: pool empty");
		throw std::string("ERROR: pool empty");
	}

	*pointer = queue->GetMemoryPtr();
}

template<class T>
inline void LFPBuffer::FreetPoolMemory(T** pointer, QueueInterface<T*>* queue) {
	if (*pointer == nullptr)
		return;

	queue->MemoryFreed(*pointer);
	*pointer = nullptr;
}

template<class T>
inline QueueInterface<T>::QueueInterface(const unsigned int pool_size) :
		pool_size_(pool_size + 1) {
	// 1 dummy element to distinguish completely full and empty pool
	pool_ = new T[pool_size + 1];
}

//==========================================================================================

#endif
