//
//  LFPBuffer.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_LFPBuffer_h
#define sdl_example_LFPBuffer_h

#define _CHANNEL_NUM 64

#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <sys/stat.h>
#include <string>

#include "LFPOnline.h"
#include "TetrodesInfo.h"
#include "Spike.h"
#include "OnlineEstimator.h"
#include "Config.h"
#include "UserContext.h"
#include "Utils.h"

#include <armadillo>

#include <boost/filesystem.hpp>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#define M_PI 3.14159265358979323846
#endif

#define CHAR_SIGNAL
typedef char signal_type;

enum LFPPipelineStatus{
	PIPELINE_STATUS_READ_BIN,
	PIPELINE_STATUS_READ_FET,
	PIPELINE_STATUS_ONLINE,
	PIPELINE_STATUS_INPUT_OVER
};

enum LFPONLINE__ERROR_CODES{
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

class SpatialInfo{
public:
	// if only one LED pos is known, the other one will be equal to -1
	float x_small_LED_ = .0;
	float y_small_LED_ = .0;
	float x_big_LED_ = .0;
	float y_big_LED_ = .0;
	unsigned int pkg_id_ = 0;
	float speed_ = .0;
	// valid if at least one LED is known
	bool valid = false;

public:

	float x_pos();
	float y_pos();
};

class LFPONLINEAPI LFPBuffer{

public:
	LFPPipelineStatus pipeline_status_;

	// TODO use instead of string literals
	class POS_BUF_POINTER_NAMES{
	public:
		const std::string POS_BUF_POS = "pos";
		const std::string POS_BUF_SPEED_EST = "speed.est";
		const std::string POS_BUF_SPIKE_SPEED = "spike.speed";
		const std::string POS_BUF_SPIKE_POS = "spike.pos";
	};
	POS_BUF_POINTER_NAMES pos_buf_pointer_names_;

	Config *config_ = nullptr;
	UserContext user_context_;

    const unsigned int CHANNEL_NUM = 64;

    // Axona package configuration in bytes
    const int CHUNK_SIZE = 432;
    const int HEADER_LEN = 32;
    const int TAIL_LEN = 16;
    const int BLOCK_SIZE = 64;

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

    // which channel is at i-th position in the BIN chunk
	int *CH_MAP_INV;

    TetrodesInfo *tetr_info_ = nullptr;
    // alternative tetrode infos (e.g. for other models)
    std::vector<TetrodesInfo *> alt_tetr_infos_;

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

    // TODO: size ?
    std::vector<unsigned int> spike_buf_pos_clusts_;

    // TODO ? messaging between processors
    bool ac_reset_ = false;
    int ac_reset_tetrode_ = -1;
	int ac_reset_cluster_ = -1;

    // POSITION BUFFER
    // TODO rewind ??? [max = 8h]
    //const int POS_BUF_SIZE = 1 << 20;
    // 4 coords, pkg_id and speed magnitude
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
    // TODO !!! rewind
    unsigned int pos_buf_pos_whl_writer_ = 0;

    // pvw[tetrode][cluster] - number of spikes in window [last_pkg_id - POP_VEC_WIN_LEN, last_pkg_id]
    // initialized by GMM clustering processor
    // !!! the cluster number is shifted by one
    std::vector< std::vector<unsigned int> > population_vector_window_;
    unsigned int population_vector_total_spikes_ = 0;
    std::queue<Spike*> population_vector_stack_;

    // map of whether tetrode is used for high synchrony detection
    bool *is_high_synchrony_tetrode_;
    unsigned int high_synchrony_tetrode_spikes_ = 0;

    std::vector<std::vector<unsigned int>> swrs_;
    std::vector<int> swr_pointers_;

    // to be initialized when first pos package is detected
    int pos_first_pkg_ = -1;

private:
	std::ofstream log_stream;

public:
    bool is_valid_channel_[_CHANNEL_NUM];

    signal_type *signal_buf[_CHANNEL_NUM];
    int *filtered_signal_buf[_CHANNEL_NUM];
    int *power_buf[_CHANNEL_NUM];

    // ??? for all arrays ?
    unsigned int buf_pos = BUF_HEAD_LEN;
    unsigned int buf_pos_trig_ = BUF_HEAD_LEN;

    unsigned int last_pkg_id = 0;
    // for each tetrode
    int *last_spike_pos_ = nullptr;

    // if shift has happened, what was the previous zero level;
    int zero_level;

    unsigned char *chunk_ptr = nullptr;
    unsigned int num_chunks;

    std::vector<OnlineEstimator<float, float> > powerEstimators_;
    OnlineEstimator<float, float>* powerEstimatorsMap_[_CHANNEL_NUM];

    OnlineEstimator<float, float>* speedEstimator_;

    // for ISI estimation - previous spike
    // ??? initialize with fake spike to avoid checks
    unsigned int *previous_spikes_pkg_ids_;

    OnlineEstimator<float, float>** ISIEstimators_;

    arma::fmat cluster_spike_counts_;

    // TODO !WORKAOURD! implement exchange through interface between processors
    // ? predictions buffer ?
    // in case there are few processors, using processors are responsible for resizing
    std::vector<arma::fmat> last_predictions_;
    std::vector<unsigned int> last_preidction_window_ends_;

    std::vector<arma::mat> tps_;

	unsigned int input_duration_ = 0;

	// dimensionalities of spike feature representations for each tetrode
	// this should be abstracted from the wat the features are extracted (PCs etc.)
	std::vector<unsigned int> feature_space_dims_;

	// DEBUG
	// for performance evaluation
	time_t checkpoint_ = 0;
//	unsigned int target_pkg_id_ = 482412;
	unsigned int target_pkg_id_ = 127205;
	unsigned int target_buf_pos_ = 7543;

	int coord_shift_x_ = 0;
	int coord_shift_y_ = 0;

	const static unsigned int TETRODE_UNKNOWN = std::numeric_limits<unsigned int>::max();
	const static unsigned int CLUSTER_UNKNOWN = std::numeric_limits<unsigned int>::max();

	std::stringstream log_string_stream_;

	unsigned int WS_SHIFT = 100;

	const unsigned int SPEED_ESTIMATOR_WINDOW_;

    //====================================================================================================

    LFPBuffer(Config* config);
	~LFPBuffer();

    void Reset(Config* config);

    inline bool is_valid_channel(int channel_num) { return is_valid_channel_[channel_num]; }

	// Population window operations
    void RemoveSpikesOutsideWindow(const unsigned int& right_border);
    void UpdateWindowVector(Spike *spike);
	// required after cluster operations, for example
	void ResetPopulationWindow();

    bool IsHighSynchrony();
	bool IsHighSynchrony(double average_spikes_window);
	double AverageSynchronySpikesWindow();

    void AddSpike(Spike *spike);

    void Log();
    void Log(std::string message);
    void Log(std::string message, int num);
    void Log(std::string message, unsigned int num);
    void Log(std::string message, size_t num);
    void Log(std::string message, double num);

    const unsigned int& GetPosBufPointer(std::string name);

    // cause recalculation of autocorrelograms due to change in cluster(s) - from buffer start
    void ResetAC(const unsigned int& reset_tetrode, const int& reset_cluster);
    void ResetAC(const unsigned int& reset_tetrode);

    // DEBUG
    void CheckPkgIdAndReportTime(const unsigned int& pkg_id, const std::string msg, bool set_checkpoint = false);
    void CheckBufPosAndReportTime(const unsigned int& buf_pos, const std::string msg);
};

//==========================================================================================

#endif
