//
//  LFPBuffer.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_LFPBuffer_h
#define sdl_example_LFPBuffer_h

// TODO !!! dynamic
#define _POS_BUF_SIZE 1<<20
#define _CHANNEL_NUM 64

#include <vector>
#include <stack>
#include <queue>
#include <sys/stat.h>

#include "LFPOnline.h"
#include "TetrodesInfo.h"
#include "Spike.h"
#include "OnlineEstimator.h"
#include "Config.h"

#include <armadillo>
#include <boost/filesystem.hpp>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#define M_PI 3.14159265358979323846
#endif

class LFPONLINEAPI LFPBuffer{
    
public:
	Config *config_ = NULL;

    const int CHANNEL_NUM = 64;
    //static const int LFP_BUF_LEN = 1 << 11; // 11 / 20
    const int LFP_BUF_LEN;
    // TODO: large buffer now needed only for delayed spike registration
    //      STORE WAVESHAPE with prev_spike
//    static const int BUF_HEAD_LEN = 1 << 8; // 11
    const int BUF_HEAD_LEN;
    
    const int SPIKE_BUF_LEN;
    const int SPIKE_BUF_HEAD_LEN;
    
    // Axona package configuration in bytes
    const int CHUNK_SIZE = 432;
    const int HEADER_LEN = 32;
    const int TAIL_LEN = 16;
    const int BLOCK_SIZE = 64;
    
    const unsigned int SAMPLING_RATE;

	int *CH_MAP; // = { 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55 };
    
    // which channel is at i-th position in the BIN chunk
	int *CH_MAP_INV;
    
    TetrodesInfo *tetr_info_ = NULL;
    
    // spikes buffer and POINTERS [all have to be reset at buffer rewind]
    Spike* *spike_buffer_ = NULL;
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

    // TODO ? messaging between processors
    bool ac_reset_ = false;
    int ac_reset_tetrode_ = -1;
	int ac_reset_cluster_ = -1;
    
    // POSITION BUFFER
    // TODO rewind ??? [max = 8h]
    //const int POS_BUF_SIZE = 1 << 20;
    // 4 coords, pkg_id and speed magnitude
    unsigned int positions_buf_[_POS_BUF_SIZE][6];
    // main poiter - where the next position will be put
    unsigned int pos_buf_pos_ = 0;
    // the last displayed position
    unsigned int pos_buf_disp_pos_ = 0;
    // TODO: ensure reset
    unsigned int pos_buf_spike_pos_ = 0;
    // last for which speed has been calculated
    unsigned int pos_buf_pos_spike_speed_ = 0;
    // last for speed estimatino
    unsigned int pos_buf_pos_speed_est = 0;

    // value of pos., meaning that position is unknown
    const int pos_unknown_;

    // TODO: GetNextSpike(const int& proc_id_) : return next unprocessed + increase counter
    // TODO: INIT SPIKES instead of creating new /deleting
    
    // length of a population vector
    const unsigned int POP_VEC_WIN_LEN;
    // pvw[tetrode][cluster] - number of spikes in window [last_pkg_id - POP_VEC_WIN_LEN, last_pkg_id]
    // initialized by GMM clustering processor
    std::vector< std::vector<unsigned int> > population_vector_window_;
    unsigned int population_vector_total_spikes_ = 0;
    std::queue<Spike*> population_vector_stack_;
    
    std::queue<std::vector<int>> swrs_;

private:
    bool is_valid_channel_[_CHANNEL_NUM];
    
public:
    short *signal_buf[_CHANNEL_NUM];
    int *filtered_signal_buf[_CHANNEL_NUM];
    int *power_buf[_CHANNEL_NUM];
    
    // ??? for all arrays ?
    int buf_pos = BUF_HEAD_LEN;
    int buf_pos_trig_ = BUF_HEAD_LEN;

    unsigned int last_pkg_id = 0;
    // for each tetrode
    int *last_spike_pos_ = NULL;
    
    // if shift has happened, what was the previous zero level;
    int zero_level;
    
    unsigned char *chunk_ptr = NULL;
    int num_chunks;
    
    OnlineEstimator<float>* powerEstimators_;
    OnlineEstimator<float>* powerEstimatorsMap_[_CHANNEL_NUM];
    
    OnlineEstimator<float>* speedEstimator_;
    
    arma::mat cluster_spike_counts_;

    // TODO !WORKAOURD! implement exchange through interface between processors
    // ? predictions buffer ?
    arma::mat last_prediction_;
    unsigned int last_preidction_window_end_;

    std::vector<arma::mat> tps_;

	std::ofstream log_stream;

    //====================================================================================================
    
    LFPBuffer(Config* config);
	~LFPBuffer();
    
    void Reset(Config* config);

    inline bool is_valid_channel(int channel_num) { return is_valid_channel_[channel_num]; }
    
    void RemoveSpikesOutsideWindow(const unsigned int& right_border);
    void UpdateWindowVector(Spike *spike);

    void AddSpike(Spike *spike);

    void Log(std::string message);
    void Log(std::string message, int num);
};

//==========================================================================================

class Utils{
public:
    static const char* const NUMBERS[];
    
    class Converter{
    public:
    	static std::string int2str(int a);
    };

    class Math{
    public:
        inline static double Gauss2D(double sigma, double x, double y) { return 1/(2 * M_PI * sqrt(sigma)) * exp(-0.5 * (pow(x, 2) + pow(y, 2)) / (sigma * sigma)); };
        static std::vector<int> GetRange(const unsigned int& from, const unsigned int& to);
        static std::vector<int> MergeRanges(const std::vector<int>& a1, const std::vector<int>& a2);
    };

    class Output{
    public:
    	static void printIntArray(int *array, const unsigned int num_el);
    };

    class FS{
    public:
    	static bool FileExists(const std::string& filename)
    	{
    	    struct stat buf;
    	    if (stat(filename.c_str(), &buf) != -1)
    	    {
    	        return true;
    	    }
    	    return false;
    	}

    	// create directories at all levels up to the file specified in the argument
    	static bool CreateDirectories(const std::string file_path){
    		std::string  path_dir = file_path.substr(0, file_path.find_last_of('/'));
    		std::cout << "Create directory " << path_dir << "\n";
    		if(!boost::filesystem::exists(path_dir)){
    			boost::filesystem::create_directories(path_dir);
    		}

			return true;
    	}
    };
};

#endif
