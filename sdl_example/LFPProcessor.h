//
//  LFPProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__LFPProcessor__
#define __sdl_example__LFPProcessor__

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include "mlpack/methods/gmm/gmm.hpp"
#include "OnlineEstimator.h"
#include <thread>

// NOTES:
// spike identification is based on unique number assigned to each detected spike
// it is assumed that fraction of spikes discaeded after initial power threshold + refractory detection is negligible

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
    
    // coords
    float x, y;
};

class ColorPalette{
    int num_colors_;
    int *color_values_;
public:
    ColorPalette(int num_colors, int *color_values);
    int getR(int order) const;
    int getG(int order) const;
    int getB(int order) const;
    int getColor(int order) const;
    
    static const ColorPalette BrewerPalette12;
    static const ColorPalette MatlabJet256;
    
    inline const int& NumColors() const { return num_colors_; }
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



//==========================================================================================

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
    
    // drawing spikes in pos
    unsigned int spike_buf_pos_draw_xy = SPIKE_BUF_HEAD_LEN;
    
    
    // POSITION BUFFER
    static const int POS_BUF_SIZE = 1 << 20;
    // 4 coords and pkg_id
    unsigned short positions_buf_[POS_BUF_SIZE][5];
    
    unsigned int pos_buf_pos_ = 0;
    unsigned int pos_buf_disp_pos_ = 0;
    // TODO: ensure reset
    unsigned int pos_buf_spike_pos_ = 0;
    
    
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
    
    ColorPalette palette_;
    
    void FillRect(const int x, const int y, const int cluster, const unsigned int w = 4, const unsigned int h = 4);
    
public:
    SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height);
    virtual void ReinitScreen();
};

//====================================================================================================

// tetrodes switch: implement separate interface or provide [dummy] implementation of SetDisplayTetrode() if not supported

class SDLControlInputProcessor : public LFPProcessor{
public:
    SDLControlInputProcessor(LFPBuffer *buf);
    
    virtual void process_SDL_control_input(const SDL_Event& e) = 0;
    virtual void process() = 0;
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode) = 0;
};

//====================================================================================================
class SDLControlInputMetaProcessor : public LFPProcessor{
    SDLControlInputProcessor* control_processor_;
    std::vector<SDLControlInputProcessor*> control_processors_;
    
    // id of last package with which the input was obtained
    int last_input_pkg_id_ = 0;
    static const int input_scan_rate_ = 1000;
    
    void SwitchDisplayTetrode(const unsigned int& display_tetrode);
    
public:
    virtual void process();
    
    SDLControlInputMetaProcessor(LFPBuffer* buffer, std::vector<SDLControlInputProcessor *> control_processors);
};

//====================================================================================================
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
    int det_pos = 0;
    
    int powerBufPos = 0;
    float powerBuf[POWER_BUF_LEN];
    float powerSum;
    
    int last_processed_id;
    
    int *thresholds_;
    
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

class SDLSignalDisplayProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay{
    static const int SCREEN_HEIGHT = 600;
    static const int SCREEN_WIDTH = 1280;
    
    static const int DISP_FREQ = 30;
    
    // 11000
    static const int SHIFT = 3000; // 1300
    
    int plot_scale = 40; // 40 - 1
    int plot_hor_scale = 10; // controlled
    
    //
    unsigned int displayed_channels_number_;
    unsigned int *displayed_channels_;
    
    // previous displayed coordinate (y)
    int *prev_vals_;
    
    int transform_to_y_coord(int voltage);
    void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);
    int current_x;
    
    // last disaplued position in buffer
    int last_disp_pos;
    
    // number of unrendered signal samples (subject to threshold)
    int unrendered = 0;
    
public:
    virtual void process();
    virtual void SetDisplayTetrode(const int& display_tetrode);
    
    SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, unsigned int displayed_channels_number, unsigned int *displayed_channels);
    
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
    
    // ??? workaround to avoid overflow
    float scale_ = 100.0f;
    
    // TODO: use online estimators
    // [channel][ws1][ws2]
    int ***cor_;
    // [channel][ws1]
    int **mean_;
    int ** sumsq_;
    
    // for PCA computation
    float **corf_;
    float *meanf_;
    
    float **stdf_;
    
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
    
    bool save_transform_ = false;
    bool load_transform_ = true;
    std::string pc_path_ = "/Users/igridchyn/data/bindata/jc103/pca/pc_";
    
public:
    PCAExtractionProcessor(LFPBuffer *buffer, const unsigned int& num_pc, const unsigned int& waveshape_samples, const unsigned int& min_samples, const bool load_transform, const bool save_transform);
    virtual void process();
};

class SDLPCADisplayProcessor : public SDLSingleWindowDisplay, public SDLControlInputProcessor{
    
    ColorPalette palette_;
    // TODO: display for multiple tetrodes with ability to switch
    int target_tetrode_;
    
    // displayed components, can be changed by the control keys
    unsigned int comp1_ = 0;
    unsigned int comp2_ = 4; // 4 - 2nd channel, PC1; 1 - 1st channel, PC2;
    unsigned int nchan_;
    
    // diplay non-assigned spikes
    bool display_unclassified_;
    
public:
    SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width, const unsigned int window_height, int target_tetrode, bool display_unclassified);
    
    // LFPProcessor
    virtual void process();
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode) { target_tetrode_ = display_tetrode; ReinitScreen(); buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN; }
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
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
    int min_observations_;
    
    bool save_clustering_ = false;
    bool load_clustering_ = true;
    
    // classify every .. spikes (to reduce computations overhead)
    static const int classification_rate_ = 10;
    
    std::vector<unsigned int> total_observations_;
    std::vector<arma::mat> observations_;
    std::vector<arma::mat> observations_train_;
    
    std::vector<std::vector<Spike*> > obs_spikes_;
    
    std::vector< mlpack::gmm::GMM<> > gmm_;
    
    // whether thread clustering job is over and can be joined
    bool *gmm_fitted_;
    std::vector<unsigned int> spikes_collected_;
    
    std::vector<std::vector< arma::vec > > means_;
    std::vector<std::vector< arma::mat > > covariances_;
    
    std::vector<arma::vec> weights_;
    
    // array of jobs [one per tetrode] and their start flags
    std::vector<std::thread*> clustering_jobs_;
    std::vector<bool> clustering_job_running_;

    mlpack::gmm::GMM<> fit_gmm(arma::mat observations_train, const unsigned int& max_clusters);

    std::string gmm_path_basename_ = "/Users/igridchyn/data/bindata/jc103/clustering/gmm_";
    void saveGMM(mlpack::gmm::GMM<> gmm, const unsigned int tetrode);

    void fit_gmm_thread(const unsigned int& tetr);
    void gmm_task(int *ptr);

public:
    GMMClusteringProcessor(LFPBuffer* buf, const unsigned int& min_observations, const unsigned int& rate, const unsigned int& max_clusters, const bool load_model, const bool save_model);

    // LFPProcessor
    virtual void process();

    void JoinGMMTasks();
};

//==========================================================================================

class LFPPipeline{
    std::vector<LFPProcessor*> processors;
    
public:
    inline void add_processor(LFPProcessor* processor) {processors.push_back(processor);}
    void process(unsigned char *data, int nchunks);
    
    LFPProcessor *get_processor(const unsigned int& index);
    
    std::vector<SDLControlInputProcessor *> GetSDLControlInputProcessors();
};

//==========================================================================================

class FetReaderProcessor : public LFPProcessor{
    std::ifstream fet_file_;
    
public:
    FetReaderProcessor(LFPBuffer *buf, std::string fet_path);
    virtual void process();
};

class FrequencyPowerBandProcessor : public LFPProcessor, public SDLSingleWindowDisplay{
    static const int FACTOR = 4;
    static const int BUF_LEN = 20000 * FACTOR;
    static const int ANAL_RATE = 5000;
    
    unsigned int last_performed_an = 0;
    
public:
    FrequencyPowerBandProcessor(LFPBuffer *buf, std::string window_name, const unsigned int window_width, const unsigned int window_height)
    : LFPProcessor(buf)
    , SDLSingleWindowDisplay(window_name, window_width, window_height){ }
    virtual void process();
};

class SDLWaveshapeDisplayProcessor : public SDLSingleWindowDisplay, public SDLControlInputProcessor {
    unsigned int buf_pointer_ = LFPBuffer::SPIKE_BUF_HEAD_LEN;
    unsigned last_disp_pkg_id_ = 0;
    
    unsigned int targ_tetrode_ = 0;
    int disp_cluster_ = 0;
    
    unsigned int tetrode_total_spikes_ = 0;
    
    static const unsigned int DISPLAY_RATE = 10;
    
public:
    SDLWaveshapeDisplayProcessor(LFPBuffer *buf, const std::string& window_name, const unsigned int& window_width, const unsigned int& window_height)
    : SDLControlInputProcessor(buf)
    , SDLSingleWindowDisplay(window_name, window_width, window_height){ }
    
    // LFPProcessor
    virtual void process();
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
};

//==========================================================================================

class Utils{
public:
    static const char* const NUMBERS[];
};

//==========================================================================================

class PlaceField{
    arma::mat place_field_;
    
    double sigma_;
    double bin_size_;
    // how many bins around spikes to take into account
    int spread_;

public:
    PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);
    // PlaceField doesn't know about its identity and doesn't check spikes
    void AddSpike(Spike *spike);

    inline const double& operator()(unsigned int x, unsigned int y) const { return place_field_(x, y); }

    inline size_t Width() const { return place_field_.n_cols; }
    inline size_t Height() const { return place_field_.n_rows; }
    
    inline const double Max() const { return place_field_.max(); }
};

//==========================================================================================

class PlaceFieldProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay {
    unsigned int spike_buf_pos_;
    unsigned int pos_buf_pos_;
    
    arma::mat occupancy_;
    
    std::vector< std::vector< PlaceField > > place_fields_;

    double bin_size_;
    double sigma_;
    unsigned int nbins_;
    double spread_;

    unsigned int display_tetrode_ = 0;
    unsigned int display_cluster_ = 0;

    //================================

    void drawPlaceField();
    void AddPos(int x, int y);

public:

    PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);

    // LFPProcessor
    virtual void process();

    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
};

#endif /* defined(__sdl_example__LFPProcessor__) */
