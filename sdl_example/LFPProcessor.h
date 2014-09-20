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

#include "OnlineEstimator.h"
#include "LFPBuffer.h"



//==========================================================================================

class ColorPalette{
    int num_colors_;
    int *color_values_ = NULL;
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

//==========================================================================================


//==========================================================================================

class LFPProcessor{
    
protected:
    LFPBuffer* buffer = NULL;
    virtual void Log(std::string message);

public:
    virtual std::string name();
    virtual void process() = 0;
    LFPProcessor(LFPBuffer *buf)
    :buffer(buf){}
	virtual ~LFPProcessor(){ buffer->Log(std::string("Destructor of") + name()); }
};

//====================================================================================================

class SDLSingleWindowDisplay{
protected:
    SDL_Window *window_ = NULL;
	SDL_Renderer *renderer_ = NULL;
	SDL_Texture *texture_ = NULL;
    
    const unsigned int window_width_;
    const unsigned int window_height_;
    
    ColorPalette palette_;
    
	std::string name_;

    void FillRect(const int x, const int y, const int cluster, const unsigned int w = 4, const unsigned int h = 4);
    
public:
    SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height);
    virtual void ReinitScreen();
    virtual unsigned int GetWindowID();

	virtual ~SDLSingleWindowDisplay();
};

//====================================================================================================

// tetrodes switch: implement separate interface or provide [dummy] implementation of SetDisplayTetrode() if not supported

class SDLControlInputProcessor : virtual public LFPProcessor{
public:
    SDLControlInputProcessor(LFPBuffer *buf);
    
    virtual void process_SDL_control_input(const SDL_Event& e) = 0;
    virtual void process() = 0;
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode) = 0;
	virtual ~SDLControlInputProcessor() {};
};

//====================================================================================================
class SDLControlInputMetaProcessor : public LFPProcessor{
    SDLControlInputProcessor* control_processor_ = NULL;
    std::vector<SDLControlInputProcessor*> control_processors_;
    
    std::map<unsigned int, SDLControlInputProcessor*> cp_by_win_id_;

    // id of last package with which the input was obtained
    int last_input_pkg_id_ = 0;
    int last_pkg_id = 0;
    
    unsigned int calls_since_scan = 0;
    
    static const int input_scan_rate_ = 100;
    static const int INPUT_SCAN_RATE_CALLS = 100;
    
    void SwitchDisplayTetrode(const unsigned int& display_tetrode);
    
public:
    virtual std::string name();
    virtual void process();
    
    SDLControlInputMetaProcessor(LFPBuffer* buffer, std::vector<SDLControlInputProcessor *> control_processors);
};

//====================================================================================================


class PackageExractorProcessor : public LFPProcessor{
	const float SCALE;

public:
	virtual std::string name();
    virtual void process();
    PackageExractorProcessor(LFPBuffer *buffer);
};

//==========================================================================================

class SpikeAlignmentProcessor : public LFPProcessor{
    // for each tetrode
    unsigned int *prev_spike_pos_ = NULL;
    int *prev_max_val_ = NULL;
    Spike **prev_spike_ = NULL;
    
public:
    SpikeAlignmentProcessor(LFPBuffer* buffer);
    virtual void process();
};

//==========================================================================================

class WaveShapeReconstructionProcessor : public LFPProcessor{
    unsigned int mul;
    
    double *sin_table = NULL;
    double *t_table = NULL;
    int *it_table = NULL;
    double **sztable = NULL;
    
    void construct_lookup_table();
    int optimized_value(int num_sampl,int *sampl,int h);
    void load_restore_one_spike(Spike *spike);
    void find_one_peak(Spike* spike, int *ptmout,int peakp,int peakgit,int *ptmval);
    
    int rec_tmp_[64][128];
    
    // cleanup waveshape after reconstruction
    bool cleanup_ws_ = false;

public:
    WaveShapeReconstructionProcessor(LFPBuffer* buffer);
    WaveShapeReconstructionProcessor(LFPBuffer* buffer, int mul);
    virtual void process();
};

//==========================================================================================

class FileOutputProcessor : public LFPProcessor{
    FILE *f_ = NULL;
    
public:
    FileOutputProcessor(LFPBuffer* buf);
    virtual void process();
    ~FileOutputProcessor();
};

//==========================================================================================

class FrequencyPowerBandProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor{
    const int FACTOR;
    const int BUF_LEN;
    const int ANAL_RATE;
    
    unsigned int last_performed_an = 0;
    unsigned int channel_ = 0;
    
public:
    FrequencyPowerBandProcessor(LFPBuffer *buf);
    FrequencyPowerBandProcessor(LFPBuffer *buf, std::string window_name, const unsigned int window_width, const unsigned int window_height);
    virtual void process();

    // SDLControlInputProcessor
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

	virtual ~FrequencyPowerBandProcessor();
};




//==========================================================================================


#endif /* defined(__sdl_example__LFPProcessor__) */
