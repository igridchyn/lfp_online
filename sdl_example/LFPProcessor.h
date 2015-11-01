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


#ifdef _WIN32
	#include <windows.h>
	#undef max
	#undef min
	void usleep(__int64 usec);
#else
	#include "unistd.h"
#endif // _WIN32

#include <math.h>

#include <iostream>
#include <vector>
#include <thread>
#include <fstream>

#include <SDL2/SDL.h>
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif

#include "OnlineEstimator.h"
#include "LFPBuffer.h"



//==========================================================================================

class ColorPalette{
    int num_colors_;
    int *color_values_ = nullptr;
public:
    ColorPalette(int num_colors, int *color_values);
    int getR(int order) const;
    int getG(int order) const;
    int getB(int order) const;
    int getColor(int order) const;
    
    static const ColorPalette BrewerPalette12;
    static const ColorPalette MatlabJet256;
    
    static inline unsigned char getColorR(int color) { return (color & 0xFF0000) >> 16; }
    static inline unsigned char getColorG(int color){ return (color & 0x00FF00) >> 8; }
    static inline unsigned char getColorB(int color) { return color & 0x0000FF; }

    inline const int& NumColors() const { return num_colors_; }
};

//==========================================================================================


//==========================================================================================

class LFPONLINEAPI LFPProcessor{
    
protected:
    LFPBuffer* buffer = nullptr;
    const unsigned int processor_number_ = 0;

    std::vector<unsigned int> spike_buf_tetrodewise_ptrs_;

    virtual void Log(std::string message);
    virtual void Log(std::string message, int num);
    virtual void Log(std::string message, double num);
    virtual void Log(std::string message, unsigned int num);

public:
    virtual int getInt(std::string name);
    virtual std::string getString(std::string name);
    virtual bool getBool(std::string name);
    virtual float getFloat(std::string name);
    virtual std::string getOutPath(std::string name);

    virtual std::string name();
    virtual void process() = 0;
    virtual void process_tetrode(int tetrode) { process(); };
    LFPProcessor(LFPBuffer *buf, const unsigned int& processor_number = 0)
    : buffer(buf)
    , processor_number_(processor_number){ spike_buf_tetrodewise_ptrs_.resize(buffer->tetr_info_->tetrodes_number()); }
	virtual ~LFPProcessor(){ buffer->Log(std::string("Destructor of") + name()); }

	// methods for parallel execution
	virtual void desync() {};
	virtual void sync() {};
};

//====================================================================================================


//====================================================================================================

// tetrodes switch: implement separate interface or provide [dummy] implementation of SetDisplayTetrode() if not supported

class SDLControlInputProcessor : virtual public LFPProcessor{
public:
    SDLControlInputProcessor(LFPBuffer *buf, const unsigned int processor_number = 0);
    
    UserContext& user_context_;
    unsigned int last_proc_ua_id_ = 0;

    virtual void process_SDL_control_input(const SDL_Event& e) = 0;
    virtual void process() = 0;
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode) = 0;
	virtual ~SDLControlInputProcessor() {};
};

//====================================================================================================
class SDLControlInputMetaProcessor : public LFPProcessor{
    SDLControlInputProcessor* control_processor_ = nullptr;
    std::vector<SDLControlInputProcessor*> control_processors_;
    
    std::map<unsigned int, SDLControlInputProcessor*> cp_by_win_id_;

    // id of last package with which the input was obtained
    int last_input_pkg_id_ = 0;
    unsigned int last_pkg_id = 0;
    
    unsigned int calls_since_scan = 0;
    
    static const int input_scan_rate_ = 100;
    static const int INPUT_SCAN_RATE_CALLS = 100;
    
    void SwitchDisplayTetrode(const unsigned int& display_tetrode);
    
public:
    virtual std::string name();
    virtual void process();
    
    SDLControlInputMetaProcessor(LFPBuffer* buffer, std::vector<SDLControlInputProcessor *> control_processors);
};

//==========================================================================================

class SpikeAlignmentProcessor : public LFPProcessor{
    // for each tetrode
    unsigned int *prev_spike_pos_ = nullptr;
    int *prev_max_val_ = nullptr;
    Spike **prev_spike_ = nullptr;
    
    const unsigned int REFRACTORY_PERIOD;

    const unsigned int NOISE_WIN = 5;
    std::queue<Spike*> noise_detection_queue_;
    const unsigned int NNOISE = 10;
    std::ofstream noise_stream_;
    unsigned int last_noise_pkg_id_ = 0;

public:
    SpikeAlignmentProcessor(LFPBuffer* buffer);
    virtual void process();
    virtual void process_tetrode(int tetrode);
    virtual inline std::string name() { return "Spike Alignment"; }

    virtual void desync();
    virtual void sync();
};

//==========================================================================================

class WaveShapeReconstructionProcessor : public LFPProcessor{
    unsigned int mul;
    
    double *sin_table = nullptr;
    double *t_table = nullptr;
    int *it_table = nullptr;
    double **sztable = nullptr;
    
    void construct_lookup_table();
    ws_type optimized_value(int num_sampl,ws_type *sampl,int h);
    void load_restore_one_spike(Spike *spike);
    void find_one_peak(Spike* spike, int *ptmout,int peakp,int peakgit,int *ptmval);
    
    ws_type rec_tmp_[128][128];
    
    // cleanup waveshape after reconstruction
    bool cleanup_ws_ = false;

public:
    WaveShapeReconstructionProcessor(LFPBuffer* buffer);
    WaveShapeReconstructionProcessor(LFPBuffer* buffer, int mul);
    virtual void process();

    virtual inline std::string name() { return "Waveshape Reconstruction"; }
};


//==========================================================================================

class BinaryPopulationClassifierProcessor : public LFPProcessor{

public:
	// list of clusters to be used
	std::vector<std::vector<unsigned int> > clusters_used_;
	// whether cluster should be used for decoding - duplicates previous for efficiency
	std::vector< std::vector<bool> > use_cluster_;


	// distribution of number of spikes in window
	// [env][tetr][cluster][count]
	std::vector<std::vector<std::vector< std::vector<unsigned int> > > > spike_count_stats_;
	// number of spikes in current window
	// [tetr][cluster]
	std::vector< std::vector<unsigned int> > instant_counts_;

	const unsigned int WINDOW = 2400;
	const unsigned SAMPLE_END;
	float speed_limit_ = 2.0;
	const unsigned int MAX_CLUST = 50;
	const unsigned int MAX_SPIKE_COUNT = 20;

	// threshold to define in which environment is current window
	const unsigned int ENV_THOLD = 43 * 4;
	unsigned int last_pkg_id_ = 0;

	// -1 => still unknown
	int current_environment_ = 0;

	bool distribution_reported_ = false;

	const bool SAVE;

	const double SPEED_THRESHOLD_;

	std::vector<unsigned int> class_occurances_counts_;

public:
	BinaryPopulationClassifierProcessor(LFPBuffer *buf);
	virtual ~BinaryPopulationClassifierProcessor() {}

	virtual void process();
	virtual inline std::string name() { return "Binary Population Classifier"; }
};





//==========================================================================================


#endif /* defined(__sdl_example__LFPProcessor__) */
