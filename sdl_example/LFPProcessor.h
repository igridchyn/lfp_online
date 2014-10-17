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
    
    inline const int& NumColors() const { return num_colors_; }
};

//==========================================================================================


//==========================================================================================

class LFPProcessor{
    
protected:
    LFPBuffer* buffer = nullptr;
    virtual void Log(std::string message);
    virtual void Log(std::string message, int num);
    virtual void Log(std::string message, double num);

public:
    virtual std::string name();
    virtual void process() = 0;
    LFPProcessor(LFPBuffer *buf)
    :buffer(buf){}
	virtual ~LFPProcessor(){ buffer->Log(std::string("Destructor of") + name()); }
};

//====================================================================================================


//====================================================================================================

// tetrodes switch: implement separate interface or provide [dummy] implementation of SetDisplayTetrode() if not supported

class SDLControlInputProcessor : virtual public LFPProcessor{
public:
    SDLControlInputProcessor(LFPBuffer *buf);
    
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
    unsigned int *prev_spike_pos_ = nullptr;
    int *prev_max_val_ = nullptr;
    Spike **prev_spike_ = nullptr;
    
public:
    SpikeAlignmentProcessor(LFPBuffer* buffer);
    virtual void process();
};

//==========================================================================================

class WaveShapeReconstructionProcessor : public LFPProcessor{
    unsigned int mul;
    
    double *sin_table = nullptr;
    double *t_table = nullptr;
    int *it_table = nullptr;
    double **sztable = nullptr;
    
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
    FILE *f_ = nullptr;
    
public:
    FileOutputProcessor(LFPBuffer* buf);
    virtual void process();
    ~FileOutputProcessor();
};

//==========================================================================================






//==========================================================================================


#endif /* defined(__sdl_example__LFPProcessor__) */
