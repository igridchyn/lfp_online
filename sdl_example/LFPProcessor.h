//
//  LFPProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__LFPProcessor__
#define __sdl_example__LFPProcessor__

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include "mlpack/methods/gmm/gmm.hpp"

// object size:
class Spike{
public:
    static const int WL_LENGTH = 22;
    static const int WS_LENGTH_ALIGNED = 32;
    
    int pkg_id_;
    int **waveshape = NULL;
    int **waveshape_final = NULL;
    float **pc = NULL;
    
    int cluster_id_ = -1;
    
    int tetrode_;
    int num_channels_;
    
    Spike(int pkg_id, int tetrode);
    
    // workaround ? - has to be checked in every processor
    // TODO: list of spikes or new buffer
    bool discarded_ = false;
    
    // for next processors to know whether they can process this spike
    bool aligned_ = false;
};

class ColorPalette{
    int num_colors_;
    int *color_values_;
public:
    ColorPalette(int num_colors, int *color_values);
    int getR(int order);
    int getG(int order);
    int getB(int order);
};

class TetrodesInfo{
    
public:
    
    // number of tetrodes = channel groups
    int tetrodes_number;
    
    // number of channels in each group
    int *channels_numbers;
    
    // of size tetrodes_number, indices of channels in each group
    int **tetrode_channels;
    
    // group by electrode
    int *tetrode_by_channel;
    
    int number_of_channels(Spike* spike);
};

//==========================================================================================

template<class T>
class OnlineEstimator{
    static const int BUF_SIZE = 2 << 24;
    
    T buf[BUF_SIZE];
    unsigned int buf_pos = 0;
    unsigned int num_samples = 0;
    
    T sum = 0;
    T sumsq = 0;
    
public:
    //OnlineEstimator();
    void push(T value);
    T get_mean_estimate();
    T get_std_estimate();
};

//==========================================================================================

class LFPBuffer{
    
public:
    static const int CHANNEL_NUM = 64;
    static const int LFP_BUF_LEN = 1 << 17; // 11
    // TODO: large buffer now needed only for delayed spike registration
    //      STORE WAVESHAPE with prev_spike
    static const int BUF_HEAD_LEN = 1 << 15;
    
    static const int SPIKE_BUF_LEN = 1 << 18;
    static const int SPIKE_BUF_HEAD_LEN = 1 << 7; // = 128
    
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
    
    // spikes buffer
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
    
    
    // POSITION BUFFER
    static const int POS_BUF_SIZE = 1 << 16;
    unsigned short positions_buf_[POS_BUF_SIZE][4];
    unsigned int pos_buf_pos_ = 0;
    unsigned int pos_buf_disp_pos_ = 0;
    
    
    // TODO: GetNextSpike(const int& proc_id_) : return next unprocessed + increase counter
    // TODO: INIT SPIKES instead of creating new /deleting
    
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
    
    //====================================================================================================
    
    LFPBuffer(TetrodesInfo* tetr_info);
    
    inline bool is_valid_channel(int channel_num) { return is_valid_channel_[channel_num]; }
};

class LFPProcessor{
    
protected:
    LFPBuffer* buffer;
    
public:
    virtual void process() = 0;
    LFPProcessor(LFPBuffer *buf)
    :buffer(buf){}
};

//====================================================================================================

class SDLSingleWindowDisplay{
protected:
    SDL_Window *window_;
    SDL_Renderer *renderer_;
    SDL_Texture *texture_;
    
    const unsigned int window_width_;
    const unsigned int window_height_;
    
public:
    SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height);
};

//====================================================================================================

class SDLControlInputProcessor{
public:
    virtual void process_SDL_control_input(const SDL_Event& e) = 0;
};

class SDLControlInputMetaProcessor : public LFPProcessor{
    SDLControlInputProcessor* control_processor_;
    
    // id of last package with which the input was obtained
    int last_input_pkg_id_ = 0;
    static const int input_scan_rate_ = 1000;
    
public:
    virtual void process();
    SDLControlInputMetaProcessor(LFPBuffer* buffer, SDLControlInputProcessor* control_processor);
};

class SpikeDetectorProcessor : public LFPProcessor{
    // TODO: read from config
    
    static const int POWER_BUF_LEN = 2 << 10;
    static const int SAMPLING_RATE = 24000;
    static const int REFR_LEN = (int)SAMPLING_RATE / 1000;
    
    // int is not enough for convolution
    long long filter_int_ [ 2 << 7 ];
    
    float filter[ 2 << 7];
    int filter_len;

    float nstd_;
    int refractory_;
    
    // position of last processed position in filter array
    // after process() should be equal to buf_pos
    int filt_pos = 0;
    
    int powerBufPos = 0;
    float powerBuf[POWER_BUF_LEN];
    float powerSum;
    
    int last_processed_id;
    
    std::vector<Spike*> spikes;
    
public:
    SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float nstd, const int refractory);
    virtual void process();
};

class PackageExractorProcessor : public LFPProcessor{
public:
    virtual void process();
    PackageExractorProcessor(LFPBuffer *buffer)
    :LFPProcessor(buffer){}
};

class SDLSignalDisplayProcessor : public LFPProcessor, public SDLControlInputProcessor, public SDLSingleWindowDisplay{
    static const int SCREEN_HEIGHT = 600;
    static const int SCREEN_WIDTH = 1280;
    
    static const int DISP_FREQ = 30;
    
    // 11000
    static const int SHIFT = 1300;
    
    int plot_scale = 1; // 40
    int plot_hor_scale = 10;
    int target_channel_;
    
    int transform_to_y_coord(int voltage);
    void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);
    int current_x;
    
    // last disaplued position in buffer
    int last_disp_pos;
    
    // number of unrendered signal samples (subject to threshold)
    int unrendered = 0;
    
    // previous displayed value
    int val_prev = 0;
    
public:
    virtual void process();
    SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, int target_channel);
    
    virtual void process_SDL_control_input(const SDL_Event& e);
};

class SpikeAlignmentProcessor : public LFPProcessor{
    // for each tetrode
    unsigned int *prev_spike_pos_ = 0;
    int *prev_max_val_ = 0;
    Spike **prev_spike_ = NULL;
    
public:
    SpikeAlignmentProcessor(LFPBuffer* buffer);
    virtual void process();
};

class WaveShapeReconstructionProcessor : public LFPProcessor{
    unsigned int mul;
    
    double *sin_table;
    double *t_table;
    int *it_table;
    double **sztable;
    
    void construct_lookup_table();
    int optimized_value(int num_sampl,int *sampl,int h);
    void load_restore_one_spike(Spike *spike);
    void find_one_peak(Spike* spike, int *ptmout,int peakp,int peakgit,int *ptmval);
    
    int rec_tmp_[64][128];
    
public:
    WaveShapeReconstructionProcessor(LFPBuffer* buffer, int mul);
    virtual void process();
};

class PCAExtractionProcessor : public LFPProcessor{
    // projection matrix
    float ***prm;
    const unsigned int min_samples_;
    
    void tred(float **a,int n,float d[],float e[]);
    void tqli(float d[],float e[],int n,float **z);
    void eigenc(float **m,float ev[], int ftno);
    void final(float **cor,float mea[],int ftno, int num_obj,float **prm, int prno);
    
    void compute_pcs(Spike* spike);
    
    // TODO: use online estimators
    // [channel][ws1][ws2]
    int ***cor_;
    // [channel][ws1]
    int **mean_;
    
    // for PCA computation
    float **corf_;
    float *meanf_;
    
    // number of objects accumulated in means / cors for each tetrode
    unsigned int *num_spikes;
    
    // number of components per channel
    unsigned int num_pc_;
    
    // number of waveshape samples
    unsigned int waveshape_samples_;
    
    // transform matrices : from waveshape to PC
    // [channel][ws][pc]
    float ***pc_transform_;
    
    // WORKAROUND
    // TODO: recalc PCA periodically using online estimators
    bool *pca_done_;
    
public:
    PCAExtractionProcessor(LFPBuffer *buffer, const unsigned int& num_pc, const unsigned int& waveshape_samples, const unsigned int& min_samples);
    virtual void process();
};

class SDLPCADisplayProcessor : public LFPProcessor, public SDLSingleWindowDisplay{
    
    ColorPalette palette_;
    // TODO: display for multiple tetrodes with ability to switch
    int target_tetrode_;
    
public:
    SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width, const unsigned int window_height, int target_tetrode);
    virtual void process();
};

class FileOutputProcessor : public LFPProcessor{
    FILE *f_ = NULL;
    
public:
    FileOutputProcessor(LFPBuffer* buf);
    virtual void process();
    ~FileOutputProcessor();
};

// TODO: create abstract clustering class
class GMMClusteringProcessor : public LFPProcessor{
    unsigned int dimensionality_;
    unsigned int rate_;
    unsigned int max_clusters_;
    
    unsigned int total_observations_ = 0;
    
    int min_observations_;
    static const int classification_rate_ = 10;
    
    arma::mat observations_;
    arma::mat observations_train_;
    
    std::vector<Spike*> obs_spikes_;
    mlpack::gmm::GMM<> gmm_;
    bool gmm_fitted_ = false;
    unsigned int spikes_collected_ = 0;
    
    std::vector< arma::vec > means_;
    std::vector< arma::mat > covariances_;
    arma::vec weights_;
    
public:
    GMMClusteringProcessor(LFPBuffer* buf, const unsigned int& min_observations, const unsigned int& rate, const unsigned int& max_clusters);
    virtual void process();
};

//==========================================================================================

class LFPPipeline{
    std::vector<LFPProcessor*> processors;
    
public:
    inline void add_processor(LFPProcessor* processor) {processors.push_back(processor);}
    void process(unsigned char *data, int nchunks);
};

//==========================================================================================



#endif /* defined(__sdl_example__LFPProcessor__) */
