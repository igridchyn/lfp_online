//
//  LFPBuffer.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_LFPBuffer_h
#define sdl_example_LFPBuffer_h

#include <vector>

#include "TetrodesInfo.h"
#include "Spike.h"
#include "OnlineEstimator.h"

class LFPBuffer{
    
public:
    static const int CHANNEL_NUM = 64;
    static const int LFP_BUF_LEN = 1 << 20; // 11
    // TODO: large buffer now needed only for delayed spike registration
    //      STORE WAVESHAPE with prev_spike
    static const int BUF_HEAD_LEN = 1 << 15;
    
    static const int SPIKE_BUF_LEN = 1 << 24;
    static const int SPIKE_BUF_HEAD_LEN = 1 << 14; // = 128
    
    // in bytes
    const int CHUNK_SIZE = 432;
    const int HEADER_LEN = 32;
    const int TAIL_LEN = 16;
    const int BLOCK_SIZE = 64;
    
    static const int CH_MAP[];
    
    // which channel is at i-th position in the BIN chunk
    static const int CH_MAP_INV[];
    
    // TODO: move to context class
    TetrodesInfo *tetr_info_;
    
    // spikes buffer and POINTERS [all have to be reset at buffer rewind]
    Spike* spike_buffer_[SPIKE_BUF_LEN];
    // position, at which next spike will be put
    unsigned int spike_buf_pos = SPIKE_BUF_HEAD_LEN;
    // position of first spike, not populated with original waveshape data (due to signal lag)
    unsigned int spike_buf_nows_pos = SPIKE_BUF_HEAD_LEN;
    // position of first spike without reconstructed waveshape
    unsigned int spike_buf_no_rec = SPIKE_BUF_HEAD_LEN;
    // position of first non-displayed PCA
    unsigned int spike_buf_no_disp_pca = SPIKE_BUF_HEAD_LEN;
    // last unprocessed with PCA
    unsigned int spike_buf_pos_unproc_ = SPIKE_BUF_HEAD_LEN;
    // first non-outputted
    unsigned int spike_buf_pos_out = SPIKE_BUF_HEAD_LEN;
    //  first non-clustered
    // TODO: rewind
    unsigned int spike_buf_pos_clust_ = SPIKE_BUF_HEAD_LEN;
    // drawing spikes in pos
    unsigned int spike_buf_pos_draw_xy = SPIKE_BUF_HEAD_LEN;
    // spikes with speed estimate
    unsigned int spike_buf_pos_speed_ = SPIKE_BUF_HEAD_LEN;
    // pointer to the first spike in the population time window
    unsigned int spike_buf_pos_pop_vec_ = SPIKE_BUF_HEAD_LEN;
    
    // POSITION BUFFER
    static const int POS_BUF_SIZE = 1 << 20;
    // 4 coords, pkg_id and speed magnitude
    unsigned int positions_buf_[POS_BUF_SIZE][6];
    // main poiter - where the next position will be put
    unsigned int pos_buf_pos_ = 0;
    // the last displayed position
    unsigned int pos_buf_disp_pos_ = 0;
    // TODO: ensure reset
    unsigned int pos_buf_spike_pos_ = 0;
    // last for which speed has been calculated
    unsigned int pos_buf_pos_spike_speed_ = 0;

    // TODO: GetNextSpike(const int& proc_id_) : return next unprocessed + increase counter
    // TODO: INIT SPIKES instead of creating new /deleting
    
    // length of a population vector
    const unsigned int POP_VEC_WIN_LEN;
    // pvw[tetrode][cluster] - number of spikes in window [last_pkg_id - POP_VEC_WIN_LEN, last_pkg_id]
    // initialized by GMM clustering processor
    std::vector< std::vector<unsigned int> > population_vector_window_;
    unsigned int population_vector_total_spikes_ = 0;
    
private:
    bool is_valid_channel_[CHANNEL_NUM];
    
public:
    short signal_buf[CHANNEL_NUM][LFP_BUF_LEN];
    int filtered_signal_buf[CHANNEL_NUM][LFP_BUF_LEN];
    int power_buf[CHANNEL_NUM][LFP_BUF_LEN];
    
    // ??? for all arrays ?
    int buf_pos;
    int last_pkg_id = 0;
    // for each tetrode
    int *last_spike_pos_;
    
    // if shift has happened, what was the previous zero level;
    int zero_level;
    
    unsigned char *chunk_ptr;
    int num_chunks;
    
    OnlineEstimator<float>* powerEstimators_;
    OnlineEstimator<float>* powerEstimatorsMap_[CHANNEL_NUM];
    
    OnlineEstimator<float>* speedEstimator_;
    
    //====================================================================================================
    
    LFPBuffer(TetrodesInfo* tetr_info, const unsigned int& pop_vec_win_len);
    
    inline bool is_valid_channel(int channel_num) { return is_valid_channel_[channel_num]; }
    
    void UpdateWindowVector(Spike *spike);
};

//==========================================================================================

class Utils{
public:
    static const char* const NUMBERS[];
    
    class Math{
    public:
        inline static double Gauss2D(double sigma, double x, double y) { return 1/(2 * M_PI * sqrt(sigma)) * exp(-0.5 * (pow(x, 2) + pow(y, 2)) / (sigma * sigma)); };
    };
};

#endif
