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

class Spike{
    static const int WL_LENGTH = 16;
    
public:
    int pkg_id;
    int waveshape[WL_LENGTH];
    
    Spike(int *buffer, int pkg_id, int channel);
};

class TetrodesInfo{
    
public:
    
    // number of tetrodes = channel groups
    int tetrodes_number;
    
    // number of channels in each group
    int *channels_numbers;
    
    // of size tetrodes_number, indices of channels in each group
    int **tetrode_channels;
};

class LFPBuffer{
    
public:
    static const int CHANNEL_NUM = 64;
    static const int LFP_BUF_LEN = 2048;
    
    // in bytes
    const int CHUNK_SIZE = 432;
    const int HEADER_LEN = 32;
    const int TAIL_LEN = 16;
    const int BLOCK_SIZE = 64;
    
    static const int CH_MAP[];
    
    // which channel is at i-th position in the BIN chunk
    static const int CH_MAP_INV[];
    
public:
    int signal_buf[CHANNEL_NUM][LFP_BUF_LEN];
    int filtered_signal_buf[CHANNEL_NUM][LFP_BUF_LEN];
    int power_buf[CHANNEL_NUM][LFP_BUF_LEN];
    
    // ??? for all arrays ?
    int buf_pos;
    int last_pkg_id;
    
    unsigned char *chunk_ptr;
    int num_chunks;
    
    //====================================================================================================
    
    inline int get_signal(int channel, int pkg_id);
};

class LFPProcessor{
    
protected:
    LFPBuffer* buffer;
    
public:
    virtual void process() = 0;
    LFPProcessor(LFPBuffer *buf)
    :buffer(buf){}
};

class SpikeDetectorProcessor : public LFPProcessor{
    // TODO: read from config
    
    static const int POWER_BUF_LEN = 2 << 10;
    static const int SAMPLING_RATE = 24000;
    static const int REFR_LEN = (int)SAMPLING_RATE / 1000;
    
    float filter[ 2 << 7];
    int filter_len;
    
    const int channel;
    const int detection_threshold;
    
    int powerBufPos = 0;
    float powerBuf[POWER_BUF_LEN];
    float powerSum;
    
    int last_processed_id;
    
    std::vector<Spike*> spikes;
    
public:
    SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const int channel, const float detection_threshold);
    virtual void process(LFPBuffer* buffer);
};

class PackageExractorProcessor : public LFPProcessor{
public:
    virtual void process();
    PackageExractorProcessor(LFPBuffer *buffer)
    :LFPProcessor(buffer){}
};

class SDLSignalDisplayProcessor : public LFPProcessor{
    static const int SCREEN_HEIGHT = 600;
    static const int SCREEN_WIDTH = 800;
    
    static const int DISP_FREQ = 30;
    
    static const int plot_hor_scale = 10;
    static const int plot_scale = 40;
    static const int SHIFT = 11000;
    
    SDL_Window *window_;
    SDL_Texture *texture_;
    SDL_Renderer *renderer_;
    
    int target_channel_;
    
    int transform_to_y_coord(int voltage);
    void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);    
    int current_x;
    
public:
    virtual void process();
    SDLSignalDisplayProcessor(LFPBuffer *buffer, SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture, int target_channel)
        :LFPProcessor(buffer)
        , window_(window)
        , texture_(texture)
        , renderer_(renderer)
        , target_channel_(target_channel)
        , current_x(0){}
};

//==========================================================================================

class LFPPipeline{
    std::vector<LFPProcessor*> processors;
    
public:
    inline void add_processor(LFPProcessor* processor) {processors.push_back(processor);}
    void process(unsigned char *data, int nchunks);
};

#endif /* defined(__sdl_example__LFPProcessor__) */
