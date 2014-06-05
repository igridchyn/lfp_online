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

//==========================================================================================


//==========================================================================================

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
    int last_pkg_id = 0;
    
    unsigned int calls_since_scan = 0;
    
    static const int input_scan_rate_ = 1000;
    static const int INPUT_SCAN_RATE_CALLS = 1000;
    
    void SwitchDisplayTetrode(const unsigned int& display_tetrode);
    
public:
    virtual void process();
    
    SDLControlInputMetaProcessor(LFPBuffer* buffer, std::vector<SDLControlInputProcessor *> control_processors);
};

//====================================================================================================


class PackageExractorProcessor : public LFPProcessor{
public:
    virtual void process();
    PackageExractorProcessor(LFPBuffer *buffer)
    :LFPProcessor(buffer){}
};

//==========================================================================================

class SpikeAlignmentProcessor : public LFPProcessor{
    // for each tetrode
    unsigned int *prev_spike_pos_ = 0;
    int *prev_max_val_ = 0;
    Spike **prev_spike_ = NULL;
    
public:
    SpikeAlignmentProcessor(LFPBuffer* buffer);
    virtual void process();
};

//==========================================================================================

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

//==========================================================================================

class FileOutputProcessor : public LFPProcessor{
    FILE *f_ = NULL;
    
public:
    FileOutputProcessor(LFPBuffer* buf);
    virtual void process();
    ~FileOutputProcessor();
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


//==========================================================================================

class Utils{
public:
    static const char* const NUMBERS[];
    
    class Math{
    public:
        inline static double Gauss2D(double sigma, double x, double y) { return 1/(2 * M_PI * sqrt(sigma)) * exp(-0.5 * (pow(x, 2) + pow(y, 2)) / (sigma * sigma)); };
    };
};

//==========================================================================================


#endif /* defined(__sdl_example__LFPProcessor__) */
